#include "core/definitions.hpp"
#include "core/logger/logger.hpp"
#include "core/platform/platform.hpp"
#include "application/application.hpp"
#include "application/fonts/fonts.hpp"
#include "core/actions/commands.hpp"
#include "core/actions/actions.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <tracy/Tracy.hpp>

void executeMyValue(const ActionRecord *action) {
    (*action->payload.intChangeValue.target) = action->payload.intChangeValue.current_value;
}

void undoMyValue(const ActionRecord *action) {
    (*action->payload.intChangeValue.target) = action->payload.intChangeValue.prev_value;
}

void PLATFORM_MAIN() {
    initApplication();
    initActionManager();
    applicationLoadFonts();

    u64 frame_count = 0;
    b8 first_time = true;
    ImGuiID root_dock_node = 0;
    ImGuiID extra_page_root_dock_node = 0;
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    auto commandRegistry = getCommandRegistry();

    auto main_page_commands = commandRegistry->registerCommandContext("main_page");

    InputEventCode add_event = InputEventCode(ImGuiKey_KeypadAdd);
    InputEventCode undo_event = InputEventCode(ImGuiMod_Ctrl | ImGuiKey_Z);
    InputEventCode redo_event = InputEventCode(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z);

    main_page_commands->bindCommand(add_event);
    main_page_commands->bindCommand(undo_event);
    main_page_commands->bindCommand(redo_event);

    int my_value = 0;

    registerActionHandler(ActionType::ActionType_ChangeIntValue, executeMyValue, undoMyValue);

    while (!shouldApplicationClose()) {
        ZoneScopedN("Main Loop");

        applicationProcessInput();
        applicationBeginFrame();

        ImGui::DockSpaceOverViewport(root_dock_node, viewport);  // TODO(Argosta): decide where this goes (applicationBeginFrame() Maybe?)

        if (first_time) {
            first_time = false;
            root_dock_node = ImGui::GetID("ROOT_DOCK_NODE");
            extra_page_root_dock_node = ImGui::GetID("EXTRA_PAGE_ROOT_DOCK_NODE");

            // Begin DockBuilder context
            ImGui::DockBuilderRemoveNode(root_dock_node);                             // Clear any previous layout
            ImGui::DockBuilderAddNode(root_dock_node, ImGuiDockNodeFlags_DockSpace);  // Add the dockspace
            ImGui::DockBuilderSetNodeSize(root_dock_node, ImGui::GetMainViewport()->Size);

            ImGuiID home_page_dock_id, design_page_dock_id, editor_page_dock_id;

            ImGui::DockBuilderDockWindow("Home", root_dock_node);
            ImGui::DockBuilderDockWindow("Design", root_dock_node);
            ImGui::DockBuilderDockWindow("Edit", root_dock_node);
            ImGui::DockBuilderDockWindow("Extra", root_dock_node);
            ImGui::DockBuilderFinish(root_dock_node);

            // ------------------------ Extra page layout
            ImGui::DockBuilderRemoveNode(extra_page_root_dock_node);
            ImGui::DockBuilderAddNode(extra_page_root_dock_node);

            ImGuiID left, bottom_left, top_left, right;
            ImGui::DockBuilderSplitNode(extra_page_root_dock_node, ImGuiDir::ImGuiDir_Right, 0.3f, &right, &left);
            ImGui::DockBuilderSplitNode(left, ImGuiDir::ImGuiDir_Down, 0.4f, &bottom_left, &top_left);

            ImGui::DockBuilderDockWindow("Dummy", top_left);
            ImGui::DockBuilderDockWindow("Dear ImGui Metrics/Debugger", bottom_left);
            ImGui::DockBuilderDockWindow("Dear ImGui Demo", right);

            ImGui::DockBuilderFinish(extra_page_root_dock_node);
        }

        {
            if (ImGui::Begin("Home")) {}
            ImGui::End();

            if (ImGui::Begin("Design")) {}
            ImGui::End();

            if (ImGui::Begin("Edit")) {}
            ImGui::End();

            if (ImGui::Begin("Extra")) {
                ImGuiID id2 = ImGui::GetID("EXTRA_PAGE_ROOT_DOCK_NODE");
                ImGui::DockSpace(extra_page_root_dock_node);
                ImGui::Begin("Dummy");

                if (ImGui::Button("Increment by 1") || main_page_commands->isCommandCommitted(add_event)) {
                    ActionRecord action;
                    action.type = ActionType::ActionType_ChangeIntValue;
                    action.payload.intChangeValue.target = &my_value;
                    action.payload.intChangeValue.prev_value = my_value;
                    my_value++;
                    action.payload.intChangeValue.current_value = my_value;
                    addAction(&action);

                    EZ_LOG_INFO("ADD");
                }

                if (main_page_commands->isCommandCommitted(undo_event)) {
                    undoAction();
                    EZ_LOG_INFO("UNDO");
                }

                if (main_page_commands->isCommandInitated(redo_event)) {
                    redoAction();
                    EZ_LOG_INFO("REDO");
                }

                ImGui::Text("Value : %d", my_value); ImGui::SameLine(); ImGui::Text("   |   Press \"Ctrl + Z\" to Undo & \"Ctrl + Shift + Z\" to Redo the operations");

                ImGui::End();
                ImGui::ShowDemoWindow();
                ImGui::ShowMetricsWindow();
            }
            ImGui::End();
        }

        applicationRenderFrame();
        FrameMark;
        frame_count++;
    }

    shutdownPlatform();
}