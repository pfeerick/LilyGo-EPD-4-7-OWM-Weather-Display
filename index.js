import { execSync, spawnSync } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";

// Load simulator/.env if present (gitignored credentials file)
const dotenvPath = join(import.meta.dir, "simulator", ".env");
if (existsSync(dotenvPath)) {
  for (const line of readFileSync(dotenvPath, "utf8").split("\n")) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith("#")) continue;
    const eq = trimmed.indexOf("=");
    if (eq < 1) continue;
    const key = trimmed.slice(0, eq).trim();
    const val = trimmed.slice(eq + 1).trim();
    if (!(key in process.env)) process.env[key] = val;
  }
}

function git(args) {
  try {
    return execSync(`git ${args}`, { encoding: "utf8" }).trim();
  } catch {
    return "unknown";
  }
}
const sha = git("rev-parse --short HEAD");
const branch = git("rev-parse --abbrev-ref HEAD");
const dirty = git("status --porcelain") !== "";
const buildTime = `${new Date().toISOString().slice(0, 16).replace("T", " ")} UTC`;

const port = parseInt(process.env.PORT ?? "3000", 10);
const webDir = join(import.meta.dir, "web");

const mock = {
  __SSID__: "MyHomeWiFi",
  __PASSWORD__: "",
  __APIKEY__: "your_api_key_here",
  __SERVER__: "api.openweathermap.org",
  __CITY__: "Bath",
  __LATITUDE__: "51.38",
  __LONGITUDE__: "-2.36",
  __HEM_NORTH__: "selected",
  __HEM_SOUTH__: "",
  __UNITS_M__: "selected",
  __UNITS_I__: "",
  __LANG_EN__: "selected",
  __LANG_AR__: "",
  __LANG_CZ__: "",
  __LANG_EL__: "",
  __LANG_FA__: "",
  __LANG_FR__: "",
  __LANG_GL__: "",
  __LANG_HU__: "",
  __LANG_JA__: "",
  __LANG_KR__: "",
  __LANG_LA__: "",
  __LANG_LT__: "",
  __LANG_MK__: "",
  __LANG_SK__: "",
  __LANG_SL__: "",
  __LANG_VI__: "",
  __TIMEZONE__: "GMT0BST,M3.5.0/01,M10.5.0/02",
  __NTPSERVER__: "pool.ntp.org",
  __GMTOFFSET__: "0",
  __DSTOFFSET__: "3600",
  __SLEEPDURATION__: "30",
  __WAKEUPHOUR__: "7",
  __SLEEPHOUR__: "23",
  __BUILD__: `${branch}@${sha}${dirty ? "*" : ""} &bull; ${buildTime} (dev server)`,
};

// Path to the compiled simulator binary
const SIMULATOR_EXE = join(import.meta.dir, "simulator", "build", "simulator.exe");
const EPD_FB_SIZE = (960 / 2) * 540; // 259,200 bytes

// Cache the framebuffer for REFRESH_TTL ms so every browser poll doesn't re-fetch OWM.
const REFRESH_TTL = parseInt(process.env.REFRESH_TTL ?? "300000", 10); // default 5 min
let cachedFb = null;
let cacheExpiry = 0;

function runSimulator() {
  if (!existsSync(SIMULATOR_EXE)) {
    return { error: `Simulator not built. Run: cd simulator && cmake -B build && cmake --build build` };
  }

  const result = spawnSync(SIMULATOR_EXE, [], {
    maxBuffer: EPD_FB_SIZE + 64 * 1024,
    timeout: 30_000,
    env: process.env,
  });

  if (result.status !== 0) {
    const stderr = result.stderr?.toString() ?? "(no stderr)";
    console.error("[simulator] exit", result.status, "\n", stderr);
    return { error: `Simulator exited with status ${result.status}`, detail: stderr };
  }

  const buf = result.stdout;
  if (!buf || buf.length !== EPD_FB_SIZE) {
    return { error: `Expected ${EPD_FB_SIZE} bytes, got ${buf?.length ?? 0}` };
  }

  return { fb: buf };
}

const server = Bun.serve({
  port,
  async fetch(req) {
    const { pathname } = new URL(req.url);

    if (req.method === "GET" && pathname === "/") {
      let html = readFileSync(join(webDir, "config.html"), "utf8");
      for (const [token, value] of Object.entries(mock)) {
        html = html.replaceAll(token, value);
      }
      return new Response(html, { headers: { "Content-Type": "text/html; charset=utf-8" } });
    }

    if (req.method === "POST" && pathname === "/save") {
      const body = new URLSearchParams(await req.text());
      const parsed = Object.fromEntries(body.entries());
      console.log("POST /save (preview mode):", parsed);
      return new Response(
        "<html><body style='font-family:sans-serif;max-width:400px;margin:40px auto'>" +
          "<h2>Saved!</h2><p>Preview mode &mdash; no actual save performed.</p>" +
          "<p><a href='/'>Back</a></p></body></html>",
        { headers: { "Content-Type": "text/html; charset=utf-8" } },
      );
    }

    if (req.method === "GET" && pathname === "/update") {
      const devScript = `
    <script>
      // Dev-server preview: simulate upload progress with a fake XHR
      window.addEventListener("DOMContentLoaded", () => {
        const OrigXHR = window.XMLHttpRequest;
        function FakeXHR() {
          this.upload = { _listeners: {}, addEventListener(e, fn) { this._listeners[e] = fn; } };
          this._listeners = {};
        }
        FakeXHR.prototype.open = function(m, u) { this._url = u; };
        FakeXHR.prototype.setRequestHeader = function() {};
        FakeXHR.prototype.addEventListener = function(e, fn) { this._listeners[e] = fn; };
        FakeXHR.prototype.send = function(fd) {
          const file = fd.get("firmware");
          const size = file ? file.size : 1024 * 1024;
          const uploadMs = 4000;
          const flashMs = 1500;
          const interval = 100;
          let elapsed = 0;
          const tick = setInterval(() => {
            elapsed += interval;
            const pct = Math.min(elapsed / uploadMs, 1);
            const fn = this.upload._listeners["progress"];
            if (fn) fn({ lengthComputable: true, loaded: Math.round(pct * size), total: size });
            if (elapsed >= uploadMs) {
              clearInterval(tick);
              setTimeout(() => {
                this.status = 200;
                const loadFn = this._listeners["load"];
                if (loadFn) loadFn();
              }, flashMs);
            }
          }, interval);
        };
        window.XMLHttpRequest = FakeXHR;
      });
    </script>`;
      let html = readFileSync(join(webDir, "update.html"), "utf8").replace("</head>", `${devScript}\n  </head>`);
      html = html.replace("__BUILD__", mock.__BUILD__);
      return new Response(html, { headers: { "Content-Type": "text/html; charset=utf-8" } });
    }

    if (req.method === "POST" && pathname === "/update") {
      const formData = await req.formData();
      const file = formData.get("firmware");
      console.log(`POST /update (preview mode): filename=${file?.name} size=${file?.size}`);
      return new Response("OK", { status: 200 });
    }

    if (req.method === "GET" && pathname === "/display") {
      const html = readFileSync(join(webDir, "display.html"), "utf8");
      return new Response(html, { headers: { "Content-Type": "text/html; charset=utf-8" } });
    }

    if (req.method === "GET" && pathname === "/framebuffer") {
      const now = Date.now();
      if (!cachedFb || now > cacheExpiry || req.headers.get("cache-control") === "no-cache") {
        console.log("[simulator] running...");
        const { fb, error, detail } = runSimulator();
        if (error) {
          console.error("[simulator]", error);
          return new Response(JSON.stringify({ error, detail }), {
            status: 500,
            headers: { "Content-Type": "application/json" },
          });
        }
        cachedFb = fb;
        cacheExpiry = now + REFRESH_TTL;
        console.log(`[simulator] done — cached for ${REFRESH_TTL / 1000}s`);
      }
      return new Response(cachedFb, {
        headers: { "Content-Type": "application/octet-stream", "Cache-Control": "no-store" },
      });
    }

    // Bust the cache and re-run the simulator immediately
    if (req.method === "POST" && pathname === "/framebuffer/refresh") {
      cachedFb = null;
      cacheExpiry = 0;
      return new Response("ok", { headers: { "Content-Type": "text/plain" } });
    }


    return new Response("Not Found", { status: 404 });
  },
});

console.log(`Preview server running at http://localhost:${server.port}`);
console.log(`Serving web/ from: ${webDir}`);
console.log(`Simulator: ${existsSync(SIMULATOR_EXE) ? SIMULATOR_EXE : "(not built yet)"}`);

process.on("SIGINT", () => {
  console.log("\nShutting down...");
  server.stop();
  process.exit(0);
});
