// Windows side of the subprocess runner: CreateProcess with one inherited pipe for stdout and
// stderr, a polling read loop with a deadline, TerminateProcess on timeout.
// Not yet compiled on Windows (no Windows machine in the loop so far).
#include "support/subprocess.hpp"

#include <chrono>
#include <windows.h>

namespace ez::test {

namespace {

// Quotes one argument per the rules CommandLineToArgvW and the MSVC runtime use.
void append_quoted(std::string& cmd, const std::string& arg) {
    if (!arg.empty() && arg.find_first_of(" \t\n\v\"") == std::string::npos) {
        cmd += arg;
        return;
    }
    cmd += '"';
    for (usize i = 0; i < arg.size(); ++i) {
        usize backslashes = 0;
        while (i < arg.size() && arg[i] == '\\') {
            ++i;
            ++backslashes;
        }
        if (i == arg.size()) {
            cmd.append(backslashes * 2, '\\');
            break;
        }
        if (arg[i] == '"') {
            cmd.append(backslashes * 2 + 1, '\\');
        } else {
            cmd.append(backslashes, '\\');
        }
        cmd += arg[i];
    }
    cmd += '"';
}

}  // namespace

ProcessResult run_process(const std::string& program, const std::vector<std::string>& args,
                          const std::vector<std::string>& env, u32 timeout_ms) {
    ProcessResult result;

    std::string cmd;
    append_quoted(cmd, program);
    for (const std::string& a : args) {
        cmd += ' ';
        append_quoted(cmd, a);
    }

    // Environment block: the inherited variables, then the extra ones; later entries win.
    std::string block;
    if (char* inherited = GetEnvironmentStringsA()) {
        for (const char* e = inherited; *e != '\0'; e += strlen(e) + 1) {
            block.append(e);
            block.push_back('\0');
        }
        FreeEnvironmentStringsA(inherited);
    }
    for (const std::string& e : env) {
        block.append(e);
        block.push_back('\0');
    }
    block.push_back('\0');

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE read_end = nullptr;
    HANDLE write_end = nullptr;
    if (!CreatePipe(&read_end, &write_end, &sa, 0)) {
        result.output = "CreatePipe failed";
        return result;
    }
    SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_end;
    si.hStdError = write_end;
    PROCESS_INFORMATION pi{};
    const BOOL ok =
        CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, block.data(), nullptr, &si, &pi);
    CloseHandle(write_end);
    if (!ok) {
        CloseHandle(read_end);
        result.output = "CreateProcess failed for " + program;
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    char buf[4096];
    for (;;) {
        DWORD available = 0;
        if (!PeekNamedPipe(read_end, nullptr, 0, nullptr, &available, nullptr)) {
            break;  // the child closed its side
        }
        if (available > 0) {
            DWORD n = 0;
            if (ReadFile(read_end, buf, sizeof(buf), &n, nullptr) && n > 0) {
                result.output.append(buf, n);
            }
            continue;
        }
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
            // Drain anything written just before exit.
            DWORD n = 0;
            while (PeekNamedPipe(read_end, nullptr, 0, nullptr, &available, nullptr) && available > 0 &&
                   ReadFile(read_end, buf, sizeof(buf), &n, nullptr) && n > 0) {
                result.output.append(buf, n);
            }
            break;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            TerminateProcess(pi.hProcess, 1);
            result.timed_out = true;
            break;
        }
        Sleep(1);
    }
    CloseHandle(read_end);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    // A crash ends the process with an NTSTATUS code such as 0xC0000005; treat those as "not exited".
    if (!result.timed_out && code < 0xC0000000u) {
        result.exited = true;
        result.exit_code = static_cast<i32>(code);
    }
    return result;
}

}  // namespace ez::test
