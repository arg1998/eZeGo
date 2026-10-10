# eZeGo design sandbox

Dear ImGui running in the browser, for designing eZeGo's interface: visual language, layout, interaction, animation, theming. Nothing here runs the product. This is the reference the C++ application is built from, written with the same library the application uses, so nothing in a design depends on something ImGui cannot do.

This folder is a standalone codebase. It shares no files, dependencies or scripts with the rest of the repository, and nothing outside it refers to it. Deleting the folder removes everything it installed.

## Setup

Requires [bun](https://bun.sh), CMake 3.28+, Ninja, git, Python 3 (for emsdk), and bash. Linux and macOS.

```sh
cd design
bun run setup
```

Setup installs into this folder only, and running it again only does what is missing or out of date:

| Folder | What | Pinned in |
|---|---|---|
| `node_modules/` | Vite | `package.json` |
| `.emsdk/` | The Emscripten SDK, about 1.7 GB | `versions.json` |
| `.deps/` | Dear ImGui and SDL3 sources, checked by SHA-256 | `versions.json` |

It changes nothing in your shell: the scripts load `.emsdk/` by themselves. To do the Emscripten part by hand instead, these are the commands setup runs:

```sh
git clone --depth 1 https://github.com/emscripten-core/emsdk.git .emsdk
cd .emsdk && ./emsdk install 6.0.10 && ./emsdk activate 6.0.10
```

## Commands

| Command | What it does |
|---|---|
| `bun run dev` | Builds, serves at http://localhost:5173, and rebuilds whenever a file in `src/` or `assets/` (or `CMakeLists.txt`, `versions.json`) is saved. |
| `bun run build` | Optimized build into `dist/`, a static site to share. |
| `bun run preview` | Serves `dist/` at http://localhost:4173. Browsers will not load WebAssembly from `file://`. |
| `bun run clean` | Deletes `build/` and `dist/`. The next build starts from scratch, about 10 s. |

## How the reload works

1. `bun run dev` watches `src/` and `assets/`. On save it runs an incremental build: the changed files plus a link, about 1 to 2 s.
2. **Success:** the new `.js` and `.wasm` are copied to `build/serve/` and the page reloads itself. A pill in the corner shows the build number and how long it took.
3. **Failure:** nothing is copied. The page keeps running the last good build and shows the compiler errors over it until the next good build. The terminal shows the same errors in color.
4. **After a reload the page comes back where it was.** Dear ImGui's settings (window positions, docking) and the design state are kept in the browser's `localStorage` and restored on start. They are saved when they change and again just before the page unloads.
5. **Crashes show on the page too.** A design that crashes on start, such as a failed `IM_ASSERT`, shows its error; saving a fix reloads it.

A build is identified by a hash of its files, so restarting `bun run dev` never confuses an open page.

## Layout

```
src/
  main.cpp       the host: SDL3 window on the page's canvas, WebGL2, ImGui, the frame loop
  host/          the browser: persistence in localStorage
  style/         tokens (the named values) and the theme that maps them onto ImGui
  state/         the design's one state struct, saved across reloads
  screens/       complete designs of the window's interior, one per file, listed in screens.cpp
  tools/         the design tools window (the ` key): screens, ImGui's demo, metrics, style editor
assets/fonts/    Open Sans, copied from the application; bundled into the build at /fonts
web/             the page around the canvas: loading, reload, build status. Not part of any design.
scripts/         setup, the dev loop, the release build
```

The layers depend downward only: screens use widgets (a `widgets/` folder arrives with the first custom widget), widgets use the style, and everything reads the state. The host owns the frame and calls the screen and the tools.

- **A token** is a named value: a color, a distance, a radius, a font size. Change one in `style/tokens.cpp` and everything restyles. The Tokens screen draws all of them. The starting values are the application's theme, copied value for value; slots the application left pure red are the `unset` token.
- **A screen** is a function that draws the whole interior of the window. To add one, write it in `screens/<name>.cpp`, declare it in `screens.hpp`, and add a row to the table in `screens.cpp`. New files are picked up without touching CMake.
- **State** changes are saved automatically: the host compares the state before and after each frame. To persist a new field, add it to `State` and, if it is a bool, one row to the table in `state.cpp`.

## Conventions

The code uses the application's naming and formatting (`.clang-format`, `.clang-tidy`), so designs can move into the application with little change. Namespaces follow folders: `design::style`, `design::screens`, and so on.

For editor support, open `design/` as its own VS Code window. Its settings use Emscripten's bundled clangd, which understands Emscripten's newer standard library; the system clangd does not.

## Limits of the browser

The canvas is the window's interior, which is what this sandbox is for. Not covered: the native title bar and its drag and snap behaviour, extra native windows, and the shortcuts the browser keeps for itself (Ctrl+W, Ctrl+T, Ctrl+N).
