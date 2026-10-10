// The page around the canvas. It loads the current build, reloads when a new build lands, and
// shows the build state. Nothing here is part of the design: everything on the canvas is C++.

interface Status {
  key: string;
  id: number;
  ok: boolean;
  building: boolean;
  ms: number;
  log: string;
}

type CreateModule = (options: Record<string, unknown>) => Promise<unknown>;

const canvas = document.querySelector<HTMLCanvasElement>("#canvas")!;
const pill = document.querySelector<HTMLDivElement>("#pill")!;
const errors = document.querySelector<HTMLPreElement>("#errors")!;
canvas.addEventListener("contextmenu", (event) => event.preventDefault());

// A crash in the design (a failed IM_ASSERT, an exception in main) shows on the page, not only in
// the console.
const crashed = (what: unknown) => showErrors(`The design stopped: ${what}\nThe browser console has the details.`);
window.addEventListener("error", (event) => crashed(event.error ?? event.message));
window.addEventListener("unhandledrejection", (event) => crashed(event.reason));

// Development serves the build at /app/ (scripts/dev.ts); dist/ carries it in app/ next to the page.
const base = new URL(import.meta.env.DEV ? "/app/" : "app/", document.baseURI);

async function start(key?: string) {
  const query = key ? `?v=${key}` : "";
  const module = await import(/* @vite-ignore */ new URL(`design.js${query}`, base).href);
  const create = module.default as CreateModule;
  await create({
    canvas,
    locateFile: (file: string) => new URL(`${file}${query}`, base).href,
    onAbort: crashed,
  });
  canvas.focus();
}

let fadeTimer: ReturnType<typeof setTimeout> | undefined;

function showPill(state: "building" | "ok" | "failed", text: string, fade: boolean) {
  clearTimeout(fadeTimer);
  pill.hidden = false;
  pill.classList.remove("fade");
  pill.dataset.state = state;
  pill.textContent = text;
  if (fade) fadeTimer = setTimeout(() => pill.classList.add("fade"), 2000);
}

function showErrors(text: string) {
  errors.textContent = text;
  errors.hidden = false;
}

function seconds(ms: number) {
  return `${(ms / 1000).toFixed(1)} s`;
}

function render(status: Status, reloaded: boolean) {
  if (status.building) {
    showPill("building", "building…", false);
  } else if (!status.ok) {
    showPill("failed", status.id > 0 ? `build failed · showing build ${status.id}` : "build failed", false);
    showErrors(status.log);
  } else {
    errors.hidden = true;
    if (reloaded) showPill("ok", `build ${status.id} · ${seconds(status.ms)}`, true);
    else if (!pill.hidden) showPill("ok", status.id > 0 ? `build ${status.id}` : "ok", true);
  }
}

if (import.meta.hot) {
  const reloaded = sessionStorage.getItem("design:reloaded") !== null;
  sessionStorage.removeItem("design:reloaded");

  // Listen before anything can fail, so a build that crashes is replaced by the next good one.
  let loaded = "";
  import.meta.hot.on("design:status", (status: Status) => {
    render(status, false);
    if (!status.ok || status.building || !status.key || status.key === loaded) return;
    if (!loaded) {
      loaded = status.key;
      start(loaded).catch(crashed);
      return;
    }
    sessionStorage.setItem("design:reloaded", "1");
    location.reload();
  });

  const initial: Status = await fetch("/__design/status", { cache: "no-store" }).then((r) => r.json());
  render(initial, reloaded);
  if (initial.key && !loaded) {
    loaded = initial.key;
    start(loaded).catch(crashed);
  } else if (!initial.key) {
    showPill("building", "waiting for the first build…", false);
  }
} else {
  start().catch(crashed);
}
