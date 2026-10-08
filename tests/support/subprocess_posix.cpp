// POSIX side of the subprocess runner: posix_spawn, one pipe for stdout and stderr, poll with a
// deadline, SIGKILL on timeout.
#include "support/subprocess.hpp"

#include <poll.h>
#include <spawn.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;  // NOLINT(readability-identifier-naming) POSIX name

namespace ez::test {

ProcessResult run_process(const std::string& program, const std::vector<std::string>& args,
                          const std::vector<std::string>& env, u32 timeout_ms) {
    ProcessResult result;

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(program.c_str()));
    for (const std::string& a : args) {
        argv.push_back(const_cast<char*>(a.c_str()));
    }
    argv.push_back(nullptr);

    std::vector<char*> envp;
    for (char** e = environ; *e != nullptr; ++e) {
        envp.push_back(*e);
    }
    for (const std::string& e : env) {
        envp.push_back(const_cast<char*>(e.c_str()));  // later entries win with glibc and macOS
    }
    envp.push_back(nullptr);

    int fds[2];
    if (pipe(fds) != 0) {
        result.output = std::string("pipe failed: ") + std::strerror(errno);
        return result;
    }
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, fds[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, fds[0]);

    pid_t pid = 0;
    const int rc = posix_spawn(&pid, program.c_str(), &actions, nullptr, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    close(fds[1]);
    if (rc != 0) {
        close(fds[0]);
        result.output = std::string("posix_spawn failed: ") + std::strerror(rc);
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    char buf[4096];
    for (;;) {
        const auto left =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (left.count() <= 0) {
            kill(pid, SIGKILL);
            result.timed_out = true;
            break;
        }
        pollfd p{fds[0], POLLIN, 0};
        const int ready = poll(&p, 1, static_cast<int>(left.count()));
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready <= 0) {
            continue;  // timeout is handled at the top of the loop
        }
        const ssize_t n = read(fds[0], buf, sizeof(buf));
        if (n > 0) {
            result.output.append(buf, static_cast<usize>(n));
        } else if (n == 0 || errno != EINTR) {
            break;  // end of output: the child closed its side
        }
    }
    close(fds[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (WIFEXITED(status)) {
        result.exited = true;
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.signal = WTERMSIG(status);
    }
    return result;
}

}  // namespace ez::test
