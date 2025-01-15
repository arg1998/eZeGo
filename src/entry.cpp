#include "core/definitions.hpp"
#include "core/logger/logger.hpp"
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