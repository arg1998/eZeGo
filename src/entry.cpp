#include "core/definitions.hpp"
#include "core/logger/logger.hpp"
#include "core/assertion/assertions.hpp"
#include "core/serial/SerialInterface.hpp"
#include "core/platform/platform.hpp"
#include "application/application.hpp"

#include <imgui.h>
#include <tracy/Tracy.hpp>
#include <IconsFontAwesome6.h>

void PLATFORM_MAIN() {
    initApplication();
    applicationLoadFonts();

    u64 frame_count = 0;
    while (!shouldApplicationClose()) {
        ZoneScopedN("Main Loop");

        applicationProcessInput();
        applicationBeginFrame();

    SerialPortConfig config;
    config.port_name = "/dev/tty.usbmodem11303"; // Replace with your device path (e.g., /dev/tty.usbserial-XXXXX)
    config.baud_rate = 115200;
    config.character_size = 8;
    config.stop_bits = 0; // One stop bit
    config.parity = 0;    // No parity

    // TODO: Farid added for testing
    initiliazeSerialIO();
    openIOService(config);
    runBenchmarkIO(config, 512, 1);
    shutdownSerialIO();

    PlatformState *plat_state;
        {
            ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
            ImGui::Begin("New Project");
            if (ImGui::Button("Log Message", ImVec2(150, 60))) {
                EZ_LOG_INFO("Button was pressed");
            }

            ImGui::Text("%s among %d items", ICON_FA_ARROW_DOWN , 42);
            ImGui::Button(ICON_FA_ARROW_DOWN " Search");
            ImGui::End();
            ImGui::ShowDemoWindow();
            ImGui::ShowMetricsWindow();
        }

        applicationRenderFrame();
        FrameMark;
        frame_count++;
    }

    shutdownPlatform();
}