# eZeGo — Windowing

> **Status:** Proposed 2026-10-09. Implemented so far: the GLFW to SDL3 swap and the custom title bar, inside the app's placeholder shell (§12). Everything else here is the target design and is not built. Decisions are *proposed* until confirmed. Decision IDs (`W-n`) are indexed in [`README.md`](./README.md).
> **Companions:** [`ui-system.md`](./ui-system.md) U-1 to U-7 · [`application-architecture.md`](./application-architecture.md) §4 · [`code-organization.md`](./code-organization.md) §4 · [`threading-and-timing.md`](./threading-and-timing.md) · [`cvars.md`](./cvars.md) §1, CV-9 · [`testing.md`](./testing.md) T-5, T-11
> **Scope:** native windows and what sits between them and the UI: the window library, the custom chrome, the modules that own each library, the frame handoff, docking and overlay rules, extra native windows, layout persistence, taskbar and display integration. The widget vocabulary stays with U-8; the look of the chrome is a UX decision and is not designed here.

---

## 1. Decisions

| ID | Decision |
|---|---|
| **W-1** | **SDL3 replaces GLFW** as the windowing library. Only the `window` module sees it, enforced by the build graph the way U-1 enforces ImGui. |
| **W-2** | **Every eZeGo window is borderless and draws its own chrome**: title, menu bar, fullscreen, minimize, maximize, close. The OS still moves, snaps, resizes, maximizes and minimizes the window, because the chrome is declared to the OS through its hit-test channel (§2). The chrome is the same on Windows, macOS and Linux. `window.os_decorations` restores native frames as a debugging fallback only. |
| **W-3** | **One library per module.** `window` owns SDL3 and the native window, its events, the hit test and the presentation surface. `render` owns the GPU and draws packets. `ui` owns Dear ImGui, the chrome, docking, overlays and layout persistence. `app` decides which windows exist and what is in them. No module sees two of SDL3, ImGui and OpenGL (§3). |
| **W-4** | **Input becomes eZeGo data at the window boundary.** `window` publishes POD events; `ui` feeds them to ImGui; the engine, keybindings and the scenario recorder consume the same stream. The upstream ImGui platform backend is not used. |
| **W-5** | **The UI reaches the GPU as a packet.** `ui` converts ImGui draw data and texture requests into a POD UI packet in `render`'s vocabulary; `render` draws it. The upstream ImGui renderer backend is not used. Vertices and indices are referenced, not copied; commands are translated. |
| **W-6** | **Native windows are created by `app` through `window`, never by `ui`.** `ui` attaches one ImGui context per native window. ImGui's multi-viewport feature stays off: nothing is ever torn out of a native window into a new one. |
| **W-7** | **Panels dock and never float.** A panel may leave its dock node only while the left mouse button is held during a drag; on release outside a dock target it returns to the node it came from, or to the central node when that node is gone. |
| **W-8** | **Overlays float and never dock**: the command palette, modal dialogs and toasts. They are drawn on top of the dockspace of the window they belong to and are not part of the layout. |
| **W-9** | **Layout is view state, persisted per user and per window role, never a cvar and never undoable.** Three things are saved: native geometry, ImGui's dock tree, and panel visibility. The default layout is code; reset deletes the saved files and rebuilds it. Panels have stable ids independent of their titles; the file carries a version. |
| **W-10** | **Fullscreen means a borderless window covering one chosen display**, never an exclusive mode. Restoring a window clamps it onto a display that exists. |
| **W-11** | **Main thread only, and no window operation may disturb the show output.** Every `window`, `ui` and `render` call happens on the display thread. The hit-test callback is allocation-free and answers from data written by the previous frame. |
| **W-12** | **Taskbar and dock integration goes through `window`**: attention request, progress state, icon, title. The chrome is custom; the OS surfaces that show the window are not. |
| **W-13** | **Known costs are recorded, not hidden** (§10): no accessibility tree, a different menu-bar convention on macOS, no window shadow under Wayland compositors that expect the client to draw it, and double-click on the title bar handled by the OS only where the OS does it. |

**Why.** The chrome is a product decision taken on 2026-10-09: eZeGo's top bar must look and behave the same on all three operating systems, and the OS must decorate nothing. What makes that feel native is not the drawing but telling the OS which pixels are the caption and which are the edges, so that the OS runs its own move and resize loops, snapping and all. GLFW 3.4 has no such channel; SDL3 has one call implemented on Win32, Cocoa, X11 and Wayland. The module split follows philosophy §2.4: the seam is rigid, the implementation behind it is replaceable, and a library that is seen by one module can be swapped by touching that module.

**What this costs.** Our own input glue and our own UI draw path instead of two upstream backend files, a few hundred lines owned by us. The per-platform polish that SDL3 does not provide (§10) is ours too.

---

## 2. The OS boundary

A borderless window is easy to create. The OS behaviors Amir wants to keep each need the window to answer one question: *is this point a caption, an edge, or content?*

| Platform | Channel | What it gives |
|---|---|---|
| Windows | `WM_NCHITTEST` answered with `HTCAPTION` / `HTTOP` / … | Native move, Aero Snap, shake, edge resize, double-click to maximize, snap on drag to edges |
| macOS | `NSWindow.movableByWindowBackground` toggled by the hit test | Native move; resize through `NSWindowStyleMaskResizable` edges |
| Linux X11 | `_NET_WM_MOVERESIZE` client message to the window manager | Native move and resize, window-manager tiling |
| Linux Wayland | `xdg_toplevel.move` / `xdg_toplevel.resize` with the pointer serial | Compositor-driven move and resize, tiling |

**GLFW 3.4** exposes none of them. Its undecorated Windows window is a `WS_POPUP` without `WS_THICKFRAME`, so edge resize and snap are lost and would need a subclassed window procedure; on Wayland the pointer serial is internal to GLFW. The ImGui GLFW backend also disables multi-viewport under Wayland.

**SDL3** (`SDL_SetWindowHitTest`) implements all four. Verified in its source on 2026-10-09: the Win32 handler maps the callback to the native hit codes and handles `WM_NCCALCSIZE` for borderless windows; Cocoa toggles background dragging; X11 sends `_NET_WM_MOVERESIZE` for move and for each resize direction; Wayland forwards to the compositor. SDL3 also provides `SDL_FlashWindow`, `SDL_SetWindowProgressState` / `SDL_SetWindowProgressValue` (since 3.4.0), display enumeration with per-display scale, `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`, system cursors, clipboard, and text-input control. The vendored ImGui ships an SDL3 backend; the placeholder shell uses it until W-4 is built.

The pin is SDL **3.4.18** (`release-3.4.18`), the newest release on 2026-10-09; zlib license. The dependency is built from source with our options: static library, video and events only; audio, GPU, render, camera, joystick, haptic, HIDAPI, sensor, power, dialog and tray subsystems off; X11 and Wayland both on; libdecor off because every window is borderless. The X11 extensions XScrnSaver, XTest and XShape are off: the window does not use them, and SDL stops the configure when their headers are missing.

---

## 3. Modules and the frame

| Module | Layer | Owns | Never sees |
|---|---|---|---|
| `window` | 2 | SDL3. Native windows, displays, DPI, the event pump and the POD event queue, the hit-test callback and the chrome map, native actions (minimize, maximize, fullscreen, attention, progress, title, cursor, clipboard, text input), the OpenGL surface: context, make-current, present, proc addresses | ImGui, widgets, layouts |
| `render` | 2 | The GPU. Per-window surfaces, the UI draw path, the texture slot table. Later: scene packets, viewports | ImGui, SDL3 |
| `ui` | 2 | Dear ImGui. One context per native window, the chrome, the dockspace and the dock rules, overlays, the panel registry and visibility, layout persistence, the input glue and the packet export | SDL3, OpenGL |
| `app` | 3 | Which windows exist and their roles, the panels and their content, the menus, the commands, the frame loop | SDL3, ImGui, OpenGL (U-1) |

`window` is not part of `platform` (layer 0). `platform` must stay headless: the logger and cvars use it at init, the tests and the headless runner link it. A module that links a video subsystem belongs with the other layer-2 services. The platform abstraction table in [`application-architecture.md`](./application-architecture.md) §4 keeps its "Windowing" row as a concept; the code is this module.

Dependency edges, all within layer 2 and acyclic: `render → window` (surface), `ui → window` (events, chrome map, native actions), `ui → render` (the packet types, later viewport textures). `app` depends on all three.

### 3.1 Opening a window and handing it to ImGui

Two calls from `app`, never one from `ui`:

1. `window::create(desc)`: role, size, position, display, resizable, minimum size, and the graphics API the window will be drawn with, which SDL needs at creation time. Returns a handle; the window owns its surface. `render::create_surface(handle)` builds the GL objects for it.
2. `ui::attach(handle, desc)`: the panels this window may show, the default layout recipe, the menus and the commands. `ui` creates the ImGui context, loads the saved layout or builds the default, and from then on provides the chrome map for that window.

`app` reads the saved geometry (`ui::load_geometry(role)`) before step 1 so the window opens where it was closed.

### 3.2 The frame

```text
main thread, each frame
  window::pump()                   SDL events -> per-window POD queues; geometry, DPI, focus updated;
                                   the hit-test callback answers the OS from last frame's chrome map
  for each attached window w:
      ui::begin_frame(w, events)   feed w's events into w's ImGui context; NewFrame
      ui draws the chrome band     menus from app's menu data, title, min / max / close
      app draws panels             ui::begin_panel(id) ... ui::end_panel(), inside the dockspace
      ui::end_frame(w) -> packet   Render(); textures and draw lists translated; chrome map, cursor
                                   and text-input state handed to window
      render::begin(w)             make current, viewport, clear
      render::draw_ui(w, packet)   texture ops first, then the lists
      render::end(w)               present
```

Panels are immediate mode (U-2): `app` calls `begin_panel` every frame; `ui` decides whether the panel is open and where it is docked.

---

## 4. The chrome

A band at the top of every window, drawn by `ui` as an ImGui window with no decoration, pinned at the viewport top, full width, below nothing. Its height is a style constant in points, so DPI scaling applies like any other UI metric.

| Region | Drawn by | Hit-test answer |
|---|---|---|
| Menus (left) | ImGui menu bar from the menu data `app` passes at attach | Content (excluded) |
| Title (center) | Text | Caption |
| Fullscreen, minimize, maximize or restore, close (right) | Canvas glyphs, no icon font needed; hover highlight, close in red; in fullscreen only exit-fullscreen and close | Content (excluded) |
| Everything else in the band | Background | Caption |
| The outer `border` points of the window, when resizable and neither maximized nor fullscreen | Nothing | Resize edges and corners |

**The chrome map** is what crosses the `ui → window` seam: a fixed-capacity POD of caption rects, exclusion rects, a border width and a corner size, in window points. `ui` writes it at the end of every frame; `window` stores it per window and answers the OS from it whenever asked, with no allocation and no ImGui. Edges are tested first, then exclusions, then captions, so a button at the very top still leaves a resize band above it, as native frames do.

**Actions** are direct calls from `ui` into `window`: minimize, maximize or restore, and close. Close does not destroy anything; it queues a `WindowClose` event so that `app` treats the button, the OS close command and a keyboard shortcut the same way.

**Double-click** on the caption maximizes or restores. Windows does it natively after the hit test. Under X11 and Wayland the press goes to the window manager, which starts a move, so the application never sees a click. SDL reports that press as a hit-test event, and the first one arms a double-click at its position. While armed, the hit test answers Normal there, so the second press reaches the application as an ordinary click and toggles maximize without starting a move; maximizing in the middle of a compositor move leaves a window that is marked maximized but sized short of the work area. SDL 3.4.18 needs a small patch for this (`third_party/patches/sdl3/`): Wayland must report the press as X11 does, and both must ask the hit test again at the press rather than reuse the answer from the last pointer motion. macOS does not zoom on a movable background (§10).

**Overlapping windows.** An ImGui window drawn over the band, such as a floating panel or a popup, is added to the exclusions every frame, so a click on it reaches it instead of moving the window.

**Fallback.** With `window.os_decorations = true` the window is created with native decorations, the band shows only the menus and the title, the buttons are not drawn and the chrome map is empty. It exists to compare behaviors while bringing up a platform and is `Advanced` tier.

---

## 5. Inside the frame

### 5.1 Panels and docking (W-7)

The dockspace fills the window below the chrome. Panels are ImGui windows with a **stable id** (`Title###panel.<id>`), so a title can change and keep its slot. A panel is registered once per window role with its id, title, default visibility and whether it can be closed; a closed panel is hidden, not destroyed, and reopened from the View menu or the palette.

ImGui moves a tab by undocking it first, so the docking-branch flag that forbids undocking also forbids rearranging. The rule is therefore implemented by `ui` instead of by a flag: after a panel's `End`, if the panel is floating and the left button is not held, it is docked back into the node it last lived in; if that node no longer exists, into the central node. A panel that appears for the first time in a saved layout (new in a newer version) has no node and lands in the central node by the same rule.

The **default layout** is a recipe: a central panel and an ordered list of placements, each docking a panel into a split of the remaining area by side and ratio, or as a tab beside the previous placement. `ui` builds it with the dock builder when no saved dock tree exists for the role, or after a reset.

### 5.2 Overlays (W-8)

| Kind | Behavior | Implementation |
|---|---|---|
| **Command palette** | Opens on a shortcut, lists the window's commands and panel toggles with filtering, runs one on Enter, closes on Escape or focus loss; movable and resizable; cannot dock | ImGui window with `NoDocking`, position remembered for the session |
| **Modal dialog** | Blocks and dims the window beneath; one at a time; `app` draws the content | ImGui modal popup |
| **Toast** | Non-blocking message that fades after a few seconds, stacked at the top right | Foreground draw list |

Overlays are never part of the saved layout.

---

## 6. Multiple windows (W-6)

Every native window has a **role** (`main`, `presenter`, …): the name of its layout files and the key `app` uses to decide its panels and menus. A role may have at most one window at a time.

`ui` keeps one ImGui context per attached window and switches context in `begin_frame`. Each context has its own font atlas and its own texture slots; each window has its own GL context and surface. Nothing is shared between windows at the GPU level in this phase, which keeps the texture slot table per surface and avoids share lists; a shared atlas is a later optimization (§13).

Keyboard shortcuts belong to the focused window: a command registered for a role fires in the window of that role when it has focus. The palette opens in the focused window.

Closing a secondary window saves its layout and destroys its context, surface and native window, in that order. Closing the main window quits.

The presenter window is the first use: `app` creates it on a chosen display, attaches the presenter layout, and the user drags it to a second monitor like any other OS window.

---

## 7. Persistence (W-9)

| What | Where it comes from | Stored as |
|---|---|---|
| Native geometry: position, size in points, maximized, display | `window::state()` at save time | `<role>.layout` |
| Panel visibility, layout file version | `ui` | `<role>.layout` |
| Dock tree and panel placement | `ImGui::SaveIniSettingsToMemory` with `io.IniFilename = nullptr` | `<role>.imgui` |

Files live in `<config dir>/layouts/`, beside `settings.cfg` ([`cvars.md`](./cvars.md) §6.4), one pair per role. The `.layout` file is flat `key = value` text like the settings file, with a `version` line first.

**When.** A layout change marks the role dirty; the files are written one second after the last change and at detach. Writing is atomic (temp file and rename), like CV-9. In this phase the write happens on the main thread: the files are small and the write is a few microseconds on a local disk. Moving it to the cvars' writer is a follow-up (§13).

**Reset** (`ui::reset_layout(w)`): the dock tree is discarded, every panel returns to its default visibility, the default recipe is rebuilt, and the files are rewritten. Native geometry is not touched by a reset; the window stays where it is.

**Never undoable.** No layout change emits a command into the action system. The state module, when it exists, does not see layout at all.

**Restore** clamps the saved rectangle onto a display that exists; a window saved on an unplugged monitor opens on the primary display, maximized state preserved. A saved maximized window opens maximized without passing through its restored size.

---

## 8. Input, DPI, cursors, clipboard, text input (W-4)

**Events** are one POD `Event` per occurrence with the window handle, a type and a payload: window close, resized, moved, focus, minimized, maximized, restored, scale changed, mouse enter and leave, mouse move with a touch flag, mouse button with click count, wheel, key with modifiers and repeat flag, UTF-8 text, quit, display change. The queue is cleared by `pump` and holds the frame's events until the next `pump`. `Key` is eZeGo's own enum, layout-translated like ImGui's, mapped from SDL keycodes with a scancode fallback. Drop files and gamepads are deferred.

**DPI.** Window size and mouse positions are in points; the framebuffer size is in pixels; `window::state()` reports both and the display scale. `ui` sets ImGui's display size, framebuffer scale and `FontScaleDpi` from them and rescales the style from a kept base copy when the scale changes. ImGui 1.92's dynamic fonts make a scale change a per-frame matter, not an atlas rebuild.

**Cursors.** ImGui's requested cursor at the end of a frame becomes `window::set_cursor` with eZeGo's cursor enum; `window` maps it to SDL system cursors.

**Clipboard and text input.** ImGui's clipboard callbacks call `window`. SDL3 delivers text events only while text input is started, so `ui` forwards `WantTextInput` and the input rectangle to `window` every frame.

---

## 9. Taskbar and displays (W-10, W-12)

| Need | `window` API | SDL3 |
|---|---|---|
| Get the user's attention | `request_attention(w)` | `SDL_FlashWindow` until focused |
| Show progress on the taskbar button or dock icon | `set_progress(w, state, value)` | `SDL_SetWindowProgressState/Value` (3.4+): Windows taskbar, macOS dock, Linux launchers that support it |
| Title in the taskbar and task switcher | `set_title(w, text)` | `SDL_SetWindowTitle`; the chrome draws the same text |
| Icon | deferred: the app icon asset does not exist yet | `SDL_SetWindowIcon`; on Linux the `.desktop` file |
| Displays | `display_count()`, `display(i)`: id, name, bounds, work area, scale, primary | `SDL_GetDisplays` and friends; hotplug as events |
| Fullscreen on a display | `set_fullscreen(w, on, display)` | Borderless desktop fullscreen (`SDL_SetWindowFullscreenMode(w, NULL)`), never an exclusive mode; on macOS this is Spaces fullscreen |

---

## 10. Platform notes and known gaps (W-13)

| Platform | Works through SDL3 | Ours to add, in `window`, by platform file (CO-4) | Status |
|---|---|---|---|
| **Windows** | Move, Aero Snap, shake, edge resize, double-click to maximize, taskbar flash and progress | Windows 11 **Snap Layouts** flyout when hovering our maximize button: needs `HTMAXBUTTON` from `WM_NCHITTEST`, which SDL does not return; a `SDL_SetWindowsMessageHook` of a few lines | Not written; untested |
| **macOS** | Move by hit test, resize through the resizable mask, Spaces fullscreen, dock bounce and progress | **Rounded corners and shadow** on a borderless window: configure the `NSWindow` as titled with a transparent title bar, full-size content view and hidden traffic lights (the approach Electron uses); **double-click to zoom**; a minimal **native menu** for Quit, Hide and the Window list, since the in-window menu bar replaces the system one | Not written; untested |
| **Linux X11** | Move and resize through the window manager, tiling | Double-click to maximize, from SDL's hit-test event | Done in the placeholder shell |
| **Linux Wayland** | Move and resize through the compositor, tiling, fractional scale | **No shadow**: compositors expect clients to draw their own; **double-click to maximize**, through the SDL patch (§4); a compositor may refuse a move while maximized | Double-click done in the placeholder shell; shadow is a gap |
| All | | **Accessibility**: ImGui exposes no accessibility tree, and custom chrome removes even the native buttons from screen readers. Recorded as a cost of the product decision. | Accepted cost |

---

## 11. Rules and costs

- **Allocation.** `window` allocates nothing after init except SDL's own event queue. `ui` keeps per-window command and texture-request arrays whose capacity is retained between frames; after the first frames of a layout no allocation happens on the frame path. Vertices and indices are referenced from ImGui's buffers, never copied.
- **Hit test.** The callback runs whenever the OS asks, possibly many times per frame; it reads a fixed-size map and does rect tests only.
- **Resize.** On Windows a native drag or resize runs inside an OS modal loop. SDL delivers resize events from inside it, so a frame can be drawn there; the hard guarantee that the show output never stalls comes from the output thread of [`threading-and-timing.md`](./threading-and-timing.md), not from the display thread.
- **Tests.** The hit-test map and the layout file are pure, so they get unit tests when the modules are built. Dock recipes are tested against a headless ImGui context. The GPU smoke test (T-7) opens real windows.

---

## 12. Implementation

**Done on 2026-10-09: the SDL3 swap and the custom title bar.** SDL 3.4.18 replaces GLFW. The app's placeholder shell, which already owned the window library and Dear ImGui, now creates one borderless SDL3 window and draws the title bar of §4 without menus: the title, fullscreen, minimize, maximize or restore, and close. A double-click on the title bar maximizes or restores on Windows natively, and on X11 and Wayland from SDL's hit-test event. The shell writes the chrome map every frame and answers SDL's hit test from it, so the OS moves, snaps and resizes the window. ImGui's own SDL3 and OpenGL 3 backends are used. Multi-viewport is off (W-6). Tested by Amir on KDE Plasma under Wayland on 2026-10-09: the window, its title bar and buttons work, and a double-click on the title bar maximizes and restores. Under X11 the app was only started. Windows and macOS are untested.

**Not built: the rest of this document.** The `window`, `render` and `ui` modules (W-3 to W-5), menus in the title bar, panels and the docking rules (W-7), overlays (W-8), layout persistence (W-9), the choice of display for fullscreen (W-10), taskbar integration (W-12), the cvars named here, and the platform additions of §10. Until the modules exist, the placeholder shell is the one place that sees SDL3 and ImGui, as it was for GLFW; it is the standing exception to W-1 and U-1.

**Phase 2, per platform, as Amir tests there.** The Windows Snap Layouts hook, the macOS window configuration and native menu, the app icon.

---

## 13. Open questions

- **Named layouts** (Design, Program, Show): several defaults per role, each resettable. Cheap once reset exists; waits for the UX.
- **Per-project layouts**: whether a show file can carry a layout. Deferred with CV-13.
- **Double-click on the caption under macOS**: whether Cocoa's hit-test events can drive it as on X11 and Wayland. The SDL patch could be dropped if SDL adopts the same behavior; SDL's contribution policy (its `CLAUDE.md`) refuses AI-written changes, so a report upstream would come from Amir.
- **The chrome's look**: height, glyphs, icon, title placement are UX decisions (U-8); the implementation uses placeholders.
- **Audio through SDL3**: SDL3 has an audio subsystem; it is off in the pin and the audio library question in [`philosophy.md`](./philosophy.md) §6 stays open.

---

## 14. Earlier statements this proposal changes

| Earlier statement | Where | Changed to |
|---|---|---|
| "Custom window chrome" is an explicit non-goal; OS-native frames ship first | [`philosophy.md`](./philosophy.md) §5 | Custom chrome on every platform from the first window (W-2) |
| `Window`/`Display` over GLFW; "custom chrome or an SDL3 swap is a one-module change" | [`application-architecture.md`](./application-architecture.md) §4 | SDL3 (W-1); the one-module change happened |
| `window`: "Windows, displays, input, through GLFW" | [`code-organization.md`](./code-organization.md) §4 | Through SDL3 |
| ImGui "with docking and multi-viewport"; the placeholder shell enables viewports | `dependencies.json`, `app` | Docking only; multi-viewport off (W-6) |
