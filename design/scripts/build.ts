// An optimized build for sharing: dist/ is a static site. It needs a web server (browsers do not
// load WebAssembly modules from file://); `bun run preview` serves it.
import path from "node:path";
import { build as viteBuild } from "vite";
import { cmakeBuild, configure, diagnostics, emsdkEnv, fail, publish, root, seconds } from "./lib";

const env = await emsdkEnv();
await configure("release", env);
const result = await cmakeBuild("release", env);
if (!result.ok) {
  console.log(diagnostics(result.log, { color: true }));
  fail("The C++ build failed.");
}
await viteBuild({ configFile: path.join(root, "vite.config.ts") });
await publish("release", path.join(root, "dist", "app"));
console.log(`\ndist/ is ready (C++ build ${seconds(result.ms)}).`);
