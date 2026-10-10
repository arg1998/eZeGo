// The dev loop. Watches the C++ sources and the build files, rebuilds on save, and reloads the
// browser only when the build succeeds. A failed build leaves the last good one on the page and
// shows the compiler errors over it until the next good build.
import { createReadStream, existsSync, watch } from "node:fs";
import path from "node:path";
import { createServer, type Plugin } from "vite";
import { buildDir, cmakeBuild, configure, diagnostics, emsdkEnv, fingerprint, outputs, publish, root, seconds } from "./lib";

// What the page knows about the builds. `key` identifies the served build by its content (the page
// reloads when it changes, across server restarts too); `id` counts builds for people to read.
interface Status {
  key: string;
  id: number;
  ok: boolean;
  building: boolean;
  ms: number;
  log: string;
}

const env = await emsdkEnv();
await configure("debug", env);

// The page loads from here, not from the build tree: files land here only after a good build.
const serveDir = path.join(root, "build", "serve");
let served = await fingerprint(serveDir);
const status: Status = { key: served ?? "", id: served ? 1 : 0, ok: true, building: false, ms: 0, log: "" };

const server = await createServer({ configFile: path.join(root, "vite.config.ts"), plugins: [serveBuild()] });
await server.listen();
server.printUrls();

let building = false;
let again = false;

async function rebuild() {
  if (building) {
    again = true;
    return;
  }
  building = true;
  status.building = true;
  send();

  const result = await cmakeBuild("debug", env);
  status.building = false;
  status.ms = result.ms;
  if (result.ok) {
    status.ok = true;
    status.log = "";
    const fresh = await fingerprint(path.join(buildDir("debug"), "out"));
    if (fresh !== served) {
      await publish("debug", serveDir);
      served = fresh;
      status.key = fresh ?? "";
      status.id += 1;
      say(`build ${status.id} in ${seconds(result.ms)}, reloading the page`);
    } else {
      say(`no change (${seconds(result.ms)})`);
    }
  } else {
    status.ok = false;
    status.log = diagnostics(result.log, { color: false });
    console.log(diagnostics(result.log, { color: true }));
    say(status.id > 0 ? `build failed; the page keeps build ${status.id}` : "build failed");
  }
  send();

  building = false;
  if (again) {
    again = false;
    void rebuild();
  }
}

let timer: ReturnType<typeof setTimeout> | undefined;
function schedule() {
  clearTimeout(timer);
  timer = setTimeout(rebuild, 50);  // one build for an editor's burst of writes
}

watch(path.join(root, "src"), { recursive: true }, schedule);
watch(path.join(root, "assets"), { recursive: true }, schedule);
watch(root, (_, file) => {
  if (file === "CMakeLists.txt" || file === "versions.json") schedule();
});
void rebuild();

function send() {
  server.ws.send("design:status", status);
}

function say(text: string) {
  const time = new Date().toLocaleTimeString([], { hour12: false });
  console.log(`\x1b[2m${time}\x1b[0m ${text}`);
}

// Serves the published build at /app/ without caching, and the build state at /__design/status.
function serveBuild(): Plugin {
  return {
    name: "design-serve-build",
    configureServer(dev) {
      dev.middlewares.use("/__design/status", (_req, res) => {
        res.setHeader("Content-Type", "application/json");
        res.setHeader("Cache-Control", "no-store");
        res.end(JSON.stringify(status));
      });
      dev.middlewares.use("/app", (req, res, next) => {
        const name = path.basename((req.url ?? "").split("?")[0]);
        const file = path.join(serveDir, name);
        if (!outputs.includes(name) || !existsSync(file)) return next();
        res.setHeader("Content-Type", name.endsWith(".wasm") ? "application/wasm" : "text/javascript");
        res.setHeader("Cache-Control", "no-store");
        createReadStream(file).pipe(res);
      });
    },
  };
}
