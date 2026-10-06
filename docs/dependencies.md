# Dependencies

Everything third-party is listed in **`dependencies.json`**. There are no git submodules.

```bash
./ez deps            # sync third_party/_src/ to the manifest (shallow, pinned)
./ez deps --check    # report only
./ez deps --only=imgui --force     # re-fetch one, discarding local edits
```

Each dependency is a git checkout holding exactly one commit (`git fetch --depth 1 <commit>`).
Configure and build never touch the network: configure only checks the stamp files and stops
with the command to run if anything is out of date.

## Fields

| Field | Meaning |
|---|---|
| `version`, `ref` | for humans: the release and the tag or branch |
| `commit` | **the pin**; git verifies content against it |
| `license` | must allow static linking into an MIT app |
| `used_by` | `app`, `profile`, `tests`, `tools`: lets a build skip what it does not need |
| `why` | mandatory justification |
| `prebuilt` | tools only: per-OS download URL + SHA-256 |

## Bumping a pin

1. Find the new commit: `git ls-remote --tags <url> <tag> <tag>^{}` (use the `^{}` line for
   annotated tags; it is the commit).
2. Edit `version`, `ref`, `commit` (and `prebuilt` hashes for tools) in `dependencies.json`.
3. `./ez deps`, then `./ez check`.
4. For tools: `./ez tools --force`.

If a host refuses fetch-by-commit, the script fetches `ref` instead and **fails** unless it
resolves to exactly the pinned commit. A moved tag can never slip in silently.

## How a dependency is built

`third_party/<name>.cmake` is our build description (B-8): which sources, which options, warnings
off, `-O2` even in debug. GLFW and Tracy go through their own CMake with our options; ImGui is
compiled from a source list we own.

## Adding a dependency

1. Add an entry to `dependencies.json` (pin a release tag's commit; fill in `license` and `why`).
2. Write `third_party/<name>.cmake` that defines the target and calls `ez_third_party(<target>)`.
3. `include()` it from the root `CMakeLists.txt` and link it only from the module that needs it.
4. `./ez deps && ./ez check`.

## Patches

Last resort. Put `*.patch` files in `third_party/patches/<name>/`; `deps` applies them in order.
