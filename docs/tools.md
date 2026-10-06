# Dev tools

Programs we *run* during development (not code linked into the app), pinned like dependencies.
Today that is Tracy: the profiler GUI, `tracy-capture` and `tracy-csvexport`.

```bash
./ez tools                         # install whatever is missing (asks prebuilt vs source at a terminal)
./ez tools --tools=prebuilt        # verified upstream binary
./ez tools --tools=source          # build from the pinned source
./ez tools --force                 # reinstall
cmake -P scripts/tools.cmake --print=tracy:profiler      # path of one binary
```

## Where they go

Per user, keyed by version, shared by every clone and worktree:

| OS | Location |
|---|---|
| Linux | `~/.cache/ezego-dev/tools/tracy-0.14.1/` (or `$XDG_CACHE_HOME`) |
| macOS | `~/Library/Caches/ezego-dev/tools/tracy-0.14.1/` |
| Windows | `%LOCALAPPDATA%\ezego-dev\tools\tracy-0.14.1\` |

The `-dev` suffix is deliberate: the plain `ezego` directory belongs to the application itself.
Nothing in the development workflow may read or write it.

Override with `EZ_TOOLS_DIR` (tools only) or `EZ_CACHE_DIR` (everything: tools, downloads, Tracy's
own build cache).

## Prebuilt or source

- A **prebuilt** is used only if `dependencies.json` lists it for your OS with a SHA-256, and the
  download matches it. "Trusted" means *the hash matches*, not the website.
- A hash mismatch deletes the file. In `auto` mode the tool is then built from source; with
  `--tools=prebuilt` the command fails.
- A **source** build uses the same pinned commit as the profiler client in the `profile` build,
  so GUI and app always speak the same protocol. On Linux it needs
  `libcurl4-openssl-dev libfreetype-dev libegl-dev libffi-dev` (doctor reports them).

## Provenance

Every install dir has a `PROVENANCE.json` (outside the repo, never committed):

```json
{ "tool": "tracy", "version": "0.14.1", "commit": "30997d5c…", "origin": "prebuilt",
  "source": "https://…/linux-0.14.1.zip", "sha256": "4f5757…", "host": "linux-x86_64",
  "installed": "2026-10-06T01:39:59Z", "binaries": { "profiler": "tracy-profiler", … } }
```

`./ez doctor` prints it as `tracy 0.14.1  prebuilt, verified` or `built from source 30997d5ca6`.
Nothing else reads the origin: both kinds are interchangeable.

## Adding a tool

Admission rule: only tools whose **exact version matters**. Write `tools/<name>.cmake` with
`TOOL_DEP`, `tool_binaries()`, `tool_install_prebuilt()`, `tool_build_source()` (see
`tools/tracy.cmake`) and add the entry to `dependencies.json` with `"used_by": ["tools"]`.
