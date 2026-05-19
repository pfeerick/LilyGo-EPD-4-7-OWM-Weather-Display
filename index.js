import { readFileSync } from "node:fs";
import { join } from "node:path";

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

    return new Response("Not Found", { status: 404 });
  },
});

console.log(`Preview server running at http://localhost:${server.port}`);
console.log(`Serving web/ from: ${webDir}`);

process.on("SIGINT", () => {
  console.log("\nShutting down...");
  server.stop();
  process.exit(0);
});
