#include "core/definitions.hpp"
#include "core/logger/logger.hpp"
#include "core/platform/platform.hpp"
#include "application/application.hpp"
#include "application/fonts/fonts.hpp"
#include "core/actions/commands.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <tracy/Tracy.hpp>

#include "core/actions/state.hpp"

void PLATFORM_MAIN() {
    initApplication();
    applicationLoadFonts();

    u64 frame_count = 0;
    b8 first_time = true;
    ImGuiID root_dock_node = 0;
    ImGuiID extra_page_root_dock_node = 0;
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    auto commandRegistry = getCommandRegistry();

    auto main_page_commands = commandRegistry->registerCommandContext("main_page");

    InputEventCode add_event = InputEventCode(ImGuiKey_KeypadAdd);
    InputEventCode undo_event = InputEventCode(ImGuiMod_Ctrl | ImGuiKey_Z);
    InputEventCode redo_event = InputEventCode(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z);

    main_page_commands->bindCommand(add_event);
    main_page_commands->bindCommand(undo_event);
    main_page_commands->bindCommand(redo_event);

    StateHistoryContext stateCtx(64);
    TrackableState<s32> mValue(0);
    bool extraPageMounted = false;
    bool isWindowFocused = false;

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
            if (ImGui::Begin("Home")) {
                static float myValue = 0.0f;  // Your state variable
                if (ImGui::SliderFloat("My Slider", &myValue, 0.0f, 100.0f)) {
                    // This is called continuously while dragging
                    EZ_LOG_INFO("Dragging Slider");
                }

                // Check if the user has finished interacting
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    // Notify your state manager here
                    EZ_LOG_WARN("Slider Deactivated");
                }

                static int currentItem = 0;
                const char* items[] = {"Item 1", "Item 2", "Item 3"};
                if (ImGui::BeginCombo("My Combo", items[currentItem])) {
                    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
                        bool isSelected = (currentItem == i);
                        if (ImGui::Selectable(items[i], isSelected)) {
                            currentItem = i;  // Update selection
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    
                if(ImGui::IsItemDeactivatedAfterEdit()){
                    EZ_LOG_WARN("ComboBox Closed");

                }
                    ImGui::EndCombo();
                    EZ_LOG_INFO("Combobox Open");
                }


                // Detect when the combo box closes
                // if (!ImGui::IsPopupOpen("My Combo")) {
                //     // Notify state manager (selection finalized)
                //     EZ_LOG_WARN("ComboBox Closed");
                // }


            }
            ImGui::End();

            if (ImGui::Begin("Design")) {
            }
            ImGui::End();

            if (ImGui::Begin("Edit")) {
            }
            ImGui::End();

            if (ImGui::Begin("Extra")) {
                isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy);
                if (isWindowFocused && !extraPageMounted) {
                    EZ_LOG_DEBUG("Extra Page ACTIVATED");
                    extraPageMounted = true;

                } else if (!isWindowFocused && extraPageMounted) {
                    EZ_LOG_WARN("Extra Page DEACTIVATED");
                    extraPageMounted = false;
                }

                ImGuiID id2 = ImGui::GetID("EXTRA_PAGE_ROOT_DOCK_NODE");
                ImGui::DockSpace(extra_page_root_dock_node);
                ImGui::Begin("Dummy");
                if (ImGui::Button("Increment by 1") || main_page_commands->isCommandCommitted(add_event)) {
                    Action valueUpdateAction = mValue.setState(mValue.getState() + 1);
                    stateCtx.add(valueUpdateAction);
                    EZ_LOG_INFO("ADD");
                }

                if (main_page_commands->isCommandCommitted(undo_event)) {
                    stateCtx.undoAction();
                    EZ_LOG_INFO("UNDO");
                }

                if (main_page_commands->isCommandInitated(redo_event)) {
                    stateCtx.redoAction();
                    EZ_LOG_INFO("REDO");
                }

                ImGui::Text("Value : %d", mValue.getState());
                ImGui::SameLine();
                ImGui::Text("   |   Press \"Ctrl + Z\" to Undo & \"Ctrl + Shift + Z\" to Redo the operations");

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