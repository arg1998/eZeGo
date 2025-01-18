#include "core/definitions.hpp"
#include "core/logger/logger.hpp"
#include "core/platform/platform.hpp"
#include "application/application.hpp"
#include "application/fonts/Fonts.hpp"



#include <imgui.h>
#include <tracy/Tracy.hpp>

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
            ImGui::Begin(ICON_LC_PLUS "New Project");
            if (ImGui::Button("Log Message", ImVec2(150, 60))) {
                EZ_LOG_INFO("Button was pressed");
            }

            ImGui::Text("%s among %d items", ICON_LC_AIRPLAY , 42);
            ImGui::Text(ICON_MD_ACCOUNT_CIRCLE ICON_LC_AIRPLAY  ICON_MD_STAR);
            ImGui::Button(ICON_MD_ADD_ALERT " Search");
            
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