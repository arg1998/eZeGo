#pragma once

#include "core/definitions.hpp"

#include <string>

//--------------------------------------------------------------------------------------------
//                                   forward declrations
//--------------------------------------------------------------------------------------------
struct GLFWwindow;

//--------------------------------------------------------------------------------------------
//                                      data structures
//--------------------------------------------------------------------------------------------

/// @brief Window State
enum ezWindowState {
    // when the application is on fullscreen mode and no window decoration is visible.
    // User cannot move, resize, close, or do anything with the window
    EZ_WINDOW_FULL_SCREEN,

    // when the application is maximized but he window decoration is visible.
    // User can minimize, reset the size (by moving the window), and close the window
    EZ_WINDOW_MAXIMIZED,
    // when the application is not visible and is iconified on the desktop toolbar
    EZ_WINDOW_MINIMIZED,

    // when window is neither maximized nor minimized.
    // User can move, resize, minimize, maximize, and close the window.
    EZ_WINDOW_DYNAMIC,

    // When user is closing the window (close message from OS is sent to the appliction)
    EZ_WINDOW_USER_CLOSING,

    // Window is closed, clean it up!
    EZ_WINDOW_CLOSED
};


/// @brief Data needed to create a window
struct ezWindow {
    u32 width;
    u32 height;
    ezWindowState state;
    std::string windowTitle;
    ezWindow* parentWindow;
    ezWindow* childWindow;
    GLFWwindow* window;
};

//--------------------------------------------------------------------------------------------
//                                           API
//--------------------------------------------------------------------------------------------

void initApplication();

void applicationBeginFrame();

void applicationEndFrame();

void applicationRenderFrame();

bool shouldApplicationClose();

void applicationProcessInput();
