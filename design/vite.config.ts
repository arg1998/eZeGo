import { fileURLToPath } from "node:url";
import { defineConfig } from "vite";

// The page shell lives in web/; the C++ build is served next to it at app/ (scripts/dev.ts in
// development, copied into dist/app/ by scripts/build.ts).
export default defineConfig({
  root: fileURLToPath(new URL("web", import.meta.url)),
  base: "./",
  clearScreen: false,
  build: {
    outDir: fileURLToPath(new URL("dist", import.meta.url)),
    emptyOutDir: true,
  },
});
