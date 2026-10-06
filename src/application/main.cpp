// The application's platform-independent main. The process entry point (main/WinMain) lives in
// core/platform/entry.cpp and calls PLATFORM_MAIN.

#include "application.hpp"
#include "core/assertion/assertions.hpp"
#include "core/logger/logger.hpp"
#include "core/platform/platform.hpp"
#include "core/profiler/profiler.hpp"

#include <imgui.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__clang__)
    #define EZ_COMPILER_STRING "clang " __clang_version__
#elif defined(_MSC_VER)
    #define EZ_COMPILER_STRING "msvc"
#else
    #define EZ_COMPILER_STRING "gcc " __VERSION__
#endif

s32 PLATFORM_MAIN(s32 argc, char** argv) {
    // Command line: --version (headless, used by the smoke test), --quit-after <seconds>
    f64 quit_after_s = 0.0;
    for (s32 i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("eZeGo %s (%s, %s, %s)\n", EZ_VERSION_STR, EZ_MODE_NAME, getPlatformOsTypeString(), EZ_COMPILER_STRING);
            return 0;
        }
        if (strcmp(argv[i], "--quit-after") == 0 && i + 1 < argc) {
            quit_after_s = atof(argv[++i]);
        }
    }

    // Order matters: profiler -> logger -> platform -> application (reverse on shutdown).
    startProfiler();
    initLoggingSystem();
    initPlatform("eZeGo");
    EZ_LOG_INFO("eZeGo %s starting (%s build, %s, %s)", EZ_VERSION_STR, EZ_MODE_NAME, getPlatformOsTypeString(), EZ_COMPILER_STRING);

    if (!initApplication()) {
        EZ_LOG_ERROR("Application failed to initialize");
        shutdownPlatform();
        shutdownLoggingSystem();
        stopProfiler();
        return 1;
    }
    applicationLoadFonts();

    const u64 start_ns = platformGetClockTickNs();
    u64 last_ns = start_ns;
    u64 frame_count = 0;
    f64 frame_ms_smoothed = 0.0;
    b8 show_demo = false;

    while (!shouldApplicationClose()) {
        EZ_PROFILE_ZONE("Main Loop");
        const u64 now_ns = platformGetClockTickNs();
        const f64 frame_ms = (f64)(now_ns - last_ns) / 1.0e6;
        last_ns = now_ns;
        frame_ms_smoothed += (frame_ms - frame_ms_smoothed) * 0.05;
        EZ_PROFILE_PLOT("frame ms", frame_ms);

        if (quit_after_s > 0.0 && (f64)(now_ns - start_ns) / 1.0e9 > quit_after_s) {
            applicationRequestClose();
        }

        applicationProcessInput();
        applicationBeginFrame();

        ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Hello, eZeGo")) {
            ImGui::Text("Hello, world!");
            ImGui::Separator();
            ImGui::Text("version   %s", EZ_VERSION_STR);
            ImGui::Text("build     %s", EZ_MODE_NAME);
            ImGui::Text("platform  %s", getPlatformOsTypeString());
            ImGui::Text("compiler  %s", EZ_COMPILER_STRING);
            ImGui::Text("asserts   %s", EZ_CONFIG_ASSERTION_ENABLED ? "on" : "off");
            ImGui::Text("profiler  %s", EZ_PROFILER ? (profilerIsConnected() ? "Tracy connected" : "waiting for Tracy") : "off");
            ImGui::Separator();
            ImGui::Text("frame     %.2f ms (%.0f FPS)", frame_ms_smoothed, frame_ms_smoothed > 0 ? 1000.0 / frame_ms_smoothed : 0.0);
            ImGui::Checkbox("Show Dear ImGui demo", &show_demo);
        }
        ImGui::End();
        if (show_demo) {
            ImGui::ShowDemoWindow(&show_demo);
        }

        applicationRenderFrame();
        EZ_PROFILE_FRAME();
        ++frame_count;
    }

    EZ_LOG_INFO("Shutting down after %llu frames", frame_count);
    (void)frame_count;   // EZ_LOG_INFO compiles out in release
    shutdownApplication();
    shutdownPlatform();
    shutdownLoggingSystem();
    stopProfiler();
    return 0;
}
