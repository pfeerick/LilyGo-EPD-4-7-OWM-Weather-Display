import { execSync } from "node:child_process";
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
const SIMULATOR_WASM_DIR = join(import.meta.dir, "simulator", "wasm");

const mock = {
  __SSID__: "MyHomeWiFi",
  __PASSWORD__: "",
  __APIKEY__: "your_api_key_here",
  __SERVER__: "api.openweathermap.org",
  __CITY__: "Bath",
  __LATITUDE__: "51.38",
  __LONGITUDE__: "-2.36",
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

const server = Bun.serve({
  port,
  async fetch(req) {
    const { pathname } = new URL(req.url);

    if (req.method === "GET" && pathname === "/") {
      let html = readFileSync(join(webDir, "config.html"), "utf8");
      for (const [token, value] of Object.entries(mock)) {
        html = html.replaceAll(token, value);
      }
      html = html.replace("</nav>", `  <a href="/display">Display</a>\n</nav>`);
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
      html = html.replace("</nav>", `  <a href="/display">Display</a>\n</nav>`);
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

    if (req.method === "GET" && pathname === "/owm") {
      const apikey = process.env.OWM_APIKEY ?? "";
      const lat = process.env.OWM_LAT ?? "";
      const lon = process.env.OWM_LON ?? "";
      const units = { M: "metric", I: "imperial" }[process.env.OWM_UNITS ?? "M"] ?? "metric";
      const lang = (process.env.OWM_LANG ?? "EN").toLowerCase();
      const server = process.env.OWM_SERVER ?? "api.openweathermap.org";
      const owmUrl =
        `https://${server}/data/3.0/onecall?lat=${lat}&lon=${lon}&appid=${apikey}` +
        `&mode=json&units=${units}&lang=${lang}&exclude=minutely,alerts`;
      const owmRes = await fetch(owmUrl);
      const body = await owmRes.arrayBuffer();
      return new Response(body, {
        status: owmRes.status,
        headers: { "Content-Type": "application/json" },
      });
    }

    if (req.method === "GET" && pathname === "/owm-config") {
      return new Response(
        JSON.stringify({
          city: process.env.OWM_CITY ?? "",
          units: process.env.OWM_UNITS ?? "M",
          lang: process.env.OWM_LANG ?? "EN",
          latitude: parseFloat(process.env.OWM_LAT ?? "0"),
          gmtOffsetSec: parseInt(process.env.OWM_GMT_OFFSET_SEC ?? "0", 10),
          dstOffsetSec: parseInt(process.env.OWM_DST_OFFSET_SEC ?? "0", 10),
        }),
        { headers: { "Content-Type": "application/json" } },
      );
    }

    if (req.method === "GET" && pathname.startsWith("/wasm/")) {
      const file = pathname.slice(6);
      const wasmPath = join(SIMULATOR_WASM_DIR, file);
      if (!existsSync(wasmPath)) return new Response("Not Found", { status: 404 });
      const ct = file.endsWith(".wasm") ? "application/wasm" : "application/javascript";
      return new Response(Bun.file(wasmPath), { headers: { "Content-Type": ct } });
    }

    return new Response("Not Found", { status: 404 });
  },
});

const wasmAvailable = existsSync(join(SIMULATOR_WASM_DIR, "simulator.wasm"));
console.log(`Preview server running at http://localhost:${server.port}`);
console.log(`Serving web/ from: ${webDir}`);
console.log(`Simulator (WASM): ${wasmAvailable ? SIMULATOR_WASM_DIR : "(not built — run emcmake cmake)"}`);

process.on("SIGINT", () => {
  console.log("\nShutting down...");
  server.stop();
  process.exit(0);
});
