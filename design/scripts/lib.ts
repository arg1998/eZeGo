// Shared by the setup, dev and build scripts: paths, the pinned versions, the Emscripten
// environment, and the CMake steps.
import { existsSync } from "node:fs";
import { copyFile, mkdir, readFile } from "node:fs/promises";
import path from "node:path";

export const root = path.resolve(import.meta.dir, "..");
export const emsdkDir = path.join(root, ".emsdk");
export const depsDir = path.join(root, ".deps");

export type Mode = "debug" | "release";
export const buildDir = (mode: Mode) => path.join(root, "build", mode);

// What the link step writes and the page loads. Both must come from the same build.
export const outputs = ["design.js", "design.wasm"];

export interface Dependency {
  version: string;
  url: string;
  sha256: string;
}

export interface Versions {
  emsdk: string;
  imgui: Dependency;
  sdl3: Dependency;
}

export async function versions(): Promise<Versions> {
  return JSON.parse(await readFile(path.join(root, "versions.json"), "utf8"));
}

export function fail(message: string): never {
  console.error(`\n${message}`);
  process.exit(1);
}

export function seconds(ms: number): string {
  return `${(ms / 1000).toFixed(1)} s`;
}

export function stripAnsi(text: string): string {
  return text.replace(/\x1b\[[0-9;]*m/g, "");
}

// The compiler's messages from a build log, without Ninja's progress lines and command lines, and
// with paths relative to design/. Colors are kept for the terminal and dropped for the page.
export function diagnostics(log: string, options: { color: boolean }): string {
  const noise = [/^\[\d+\/\d+\]/, /^FAILED: /, /^ninja: /, /\/em(\+\+|cc)\s/];
  const kept = log.split("\n").filter((line) => !noise.some((pattern) => pattern.test(stripAnsi(line))));
  const text = kept.join("\n").replaceAll(`${root}/`, "").trim();
  return options.color ? text : stripAnsi(text);
}

// Runs a command with its output on the terminal; stops the script if it fails.
export async function run(cmd: string[], options: { cwd?: string; env?: Record<string, string> } = {}) {
  const proc = Bun.spawn(cmd, { cwd: options.cwd ?? root, env: options.env, stdio: ["inherit", "inherit", "inherit"] });
  if ((await proc.exited) !== 0) fail(`Failed: ${cmd.join(" ")}`);
}

// The Emscripten version installed in .emsdk/, or undefined.
export async function installedEmscripten(): Promise<string | undefined> {
  const file = path.join(emsdkDir, "upstream", "emscripten", "emscripten-version.txt");
  if (!existsSync(file)) return undefined;
  return (await readFile(file, "utf8")).trim().replace(/"/g, "");
}

// The environment emsdk_env.sh sets up, without touching the caller's shell.
export async function emsdkEnv(): Promise<Record<string, string>> {
  const pinned = (await versions()).emsdk;
  const installed = await installedEmscripten();
  if (installed === undefined) fail("Emscripten is not installed here yet. Run: bun run setup");
  if (installed !== pinned) fail(`Emscripten ${installed} is installed but versions.json pins ${pinned}. Run: bun run setup`);

  const script = `source "${emsdkDir}/emsdk_env.sh" >/dev/null 2>&1 && env -0`;
  const proc = Bun.spawnSync(["bash", "-c", script], { cwd: root });
  if (proc.exitCode !== 0) fail("Could not load .emsdk/emsdk_env.sh. Run: bun run setup");
  const env: Record<string, string> = {};
  for (const entry of proc.stdout.toString().split("\0")) {
    const eq = entry.indexOf("=");
    if (eq > 0) env[entry.slice(0, eq)] = entry.slice(eq + 1);
  }
  return env;
}

// Configures the build tree once; later builds re-run CMake by themselves when CMakeLists.txt or
// versions.json change.
export async function configure(mode: Mode, env: Record<string, string>) {
  const dir = buildDir(mode);
  if (existsSync(path.join(dir, "build.ninja"))) return;
  const type = mode === "debug" ? "Debug" : "Release";
  await run(["emcmake", "cmake", "-S", root, "-B", dir, "-G", "Ninja", `-DCMAKE_BUILD_TYPE=${type}`], { env });
}

export interface BuildResult {
  ok: boolean;
  log: string;
  ms: number;
}

// Builds and captures the output, so the caller decides what to show.
export async function cmakeBuild(mode: Mode, env: Record<string, string>): Promise<BuildResult> {
  const start = performance.now();
  const proc = Bun.spawn(["cmake", "--build", buildDir(mode)], { cwd: root, env, stdout: "pipe", stderr: "pipe" });
  const [out, err] = await Promise.all([new Response(proc.stdout).text(), new Response(proc.stderr).text()]);
  const ok = (await proc.exited) === 0;
  return { ok, log: `${out}${err}`.trim(), ms: performance.now() - start };
}

// A cheap identity of the outputs in a directory; undefined when they are not all there.
export async function fingerprint(dir: string): Promise<string | undefined> {
  const parts: string[] = [];
  for (const name of outputs) {
    const file = Bun.file(path.join(dir, name));
    if (!(await file.exists())) return undefined;
    parts.push(Bun.hash(await file.arrayBuffer()).toString(16));
  }
  return parts.join("-");
}

export async function publish(mode: Mode, dest: string) {
  await mkdir(dest, { recursive: true });
  for (const name of outputs) await copyFile(path.join(buildDir(mode), "out", name), path.join(dest, name));
}
