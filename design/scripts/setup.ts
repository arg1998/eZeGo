// Installs everything the design sandbox needs, inside this folder only:
//   node_modules/  Vite                         (bun install)
//   .emsdk/        the Emscripten SDK           (version pinned in versions.json)
//   .deps/         Dear ImGui and SDL3 sources  (pinned and checked by SHA-256)
// Nothing outside design/ changes: no shell profile, no global install. Safe to run again; it only
// does what is missing or out of date.
import { existsSync } from "node:fs";
import { mkdir, readFile, rm, writeFile } from "node:fs/promises";
import path from "node:path";
import { type Dependency, depsDir, emsdkDir, fail, installedEmscripten, root, run, versions } from "./lib";

const pinned = await versions();

step("Vite");
await run(["bun", "install"], { cwd: root });

step(`Emscripten ${pinned.emsdk}`);
if (!existsSync(path.join(emsdkDir, "emsdk"))) {
  await run(["git", "clone", "--depth", "1", "https://github.com/emscripten-core/emsdk.git", emsdkDir]);
}
const installed = await installedEmscripten();
if (installed === pinned.emsdk) {
  console.log(`Emscripten ${installed} is installed.`);
} else {
  // A release newer than the clone is only known to a newer emsdk.
  await run(["git", "-C", emsdkDir, "pull", "--ff-only"]);
  await run(["./emsdk", "install", pinned.emsdk], { cwd: emsdkDir });
  await run(["./emsdk", "activate", pinned.emsdk], { cwd: emsdkDir });
  console.log("\nIgnore emsdk's PATH advice above: the scripts here load .emsdk/ by themselves.");
}

await fetchDependency("imgui", pinned.imgui);
await fetchDependency("sdl3", pinned.sdl3);

console.log("\nReady. Start the dev loop with: bun run dev");

function step(title: string) {
  console.log(`\n== ${title}`);
}

// Downloads a source archive, checks it, and unpacks it into .deps/<name>/ with a stamp file that
// CMake checks against versions.json.
async function fetchDependency(name: string, dependency: Dependency) {
  const dir = path.join(depsDir, name);
  const stamp = path.join(dir, ".sha256");
  step(`${name} ${dependency.version}`);
  if (existsSync(stamp) && (await readFile(stamp, "utf8")).trim() === dependency.sha256) {
    console.log("Up to date.");
    return;
  }

  const response = await fetch(dependency.url);
  if (!response.ok) fail(`Download failed (${response.status}): ${dependency.url}`);
  const bytes = new Uint8Array(await response.arrayBuffer());
  const sha256 = new Bun.CryptoHasher("sha256").update(bytes).digest("hex");
  if (sha256 !== dependency.sha256) fail(`${name}: SHA-256 is ${sha256}, versions.json expects ${dependency.sha256}`);

  await rm(dir, { recursive: true, force: true });
  await mkdir(dir, { recursive: true });
  const archive = path.join(depsDir, `${name}.tar.gz`);
  await Bun.write(archive, bytes);
  await run(["tar", "-xzf", archive, "-C", dir, "--strip-components=1"]);
  await rm(archive);
  await writeFile(stamp, `${dependency.sha256}\n`);
  console.log(`Unpacked into .deps/${name}/`);
}
