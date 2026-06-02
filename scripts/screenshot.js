// Renders the weather display via the WASM simulator and saves a PNG.
// Usage: bun run screenshot [output-path]
// Output defaults to simulator-screenshot.png in the current directory

import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { dirname, join } from "node:path";
import { deflateSync } from "node:zlib";

const ROOT = join(import.meta.dir, "..");
const DEFAULT_OUT = join(process.cwd(), "simulator-screenshot.png");
const outPath = process.argv[2] ?? DEFAULT_OUT;
const EPD_W = 960;
const EPD_H = 540;

// --- Load simulator/.env ---
const dotenvPath = join(ROOT, "simulator", ".env");
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

const apikey = process.env.OWM_APIKEY ?? "";
const lat = process.env.OWM_LAT ?? "";
const lon = process.env.OWM_LON ?? "";
const city = process.env.OWM_CITY ?? "";
const unitsCode = process.env.OWM_UNITS ?? "M";
const lang = (process.env.OWM_LANG ?? "EN").toLowerCase();
const units = { M: "metric", I: "imperial" }[unitsCode] ?? "metric";
const gmtOffsetSec = parseInt(process.env.OWM_GMT_OFFSET_SEC ?? "0", 10);
const dstOffsetSec = parseInt(process.env.OWM_DST_OFFSET_SEC ?? "0", 10);
const owmServer = process.env.OWM_SERVER ?? "api.openweathermap.org";

if (!apikey) {
  console.error("Error: OWM_APIKEY not set. Copy simulator/.env.example to simulator/.env and fill in your key.");
  process.exit(1);
}

// --- Load WASM module (Node-compatible Emscripten glue) ---
const wasmDir = join(ROOT, "simulator", "wasm");
if (!existsSync(join(wasmDir, "simulator.wasm"))) {
  console.error("Error: simulator/wasm/simulator.wasm not found.");
  console.error("Download from the latest release or rebuild with emcmake cmake.");
  process.exit(1);
}

const require = createRequire(import.meta.url);
const SimulatorModule = require(join(wasmDir, "simulator.js"));
console.log("Loading WASM module...");
const wasm = await SimulatorModule({
  locateFile: (f) => join(wasmDir, f),
  // Suppress Emscripten's default stdout/stderr noise
  print: () => {},
  printErr: () => {},
});

// --- Initialise and configure ---
wasm.ccall("wasm_init", null, [], []);
// Use "Simulator" as city name to match the browser's default (Show city checkbox unchecked)
wasm.ccall(
  "wasm_set_config",
  null,
  ["string", "string", "string", "number", "number", "number"],
  ["Simulator", unitsCode, lang.toUpperCase(), parseFloat(lat), gmtOffsetSec, dstOffsetSec],
);

// --- Fetch OWM data ---
const owmUrl =
  `https://${owmServer}/data/3.0/onecall?lat=${lat}&lon=${lon}&appid=${apikey}` +
  `&mode=json&units=${units}&lang=${lang}&exclude=minutely,alerts`;

console.log(`Fetching OWM data for ${city || `${lat},${lon}`}...`);
const owmRes = await fetch(owmUrl);
if (!owmRes.ok) {
  console.error(`OWM API error: ${owmRes.status} ${owmRes.statusText}`);
  const body = await owmRes.text();
  console.error(body.slice(0, 400));
  process.exit(1);
}
const owmJson = await owmRes.text();

// --- Render ---
console.log("Rendering...");
const enc = new TextEncoder().encode(owmJson);
const ptr = wasm._malloc(enc.length);
wasm.HEAPU8.set(enc, ptr);
const ok = wasm.ccall("wasm_render", "number", ["number", "number"], [ptr, enc.length]);
wasm._free(ptr);
if (ok < 0) {
  console.error("wasm_render failed — check OWM JSON format (stderr above).");
  process.exit(1);
}

// --- Read framebuffer (4bpp packed grayscale) ---
const fbPtr = wasm.ccall("wasm_get_framebuffer", "number", [], []);
const fbSize = wasm.ccall("wasm_fb_size", "number", [], []);
const fb = new Uint8Array(wasm.HEAPU8.buffer, fbPtr, fbSize).slice();

// --- Unpack nibbles → 8-bit grayscale ---
// odd x → upper nibble (byte >> 4), even x → lower nibble (byte & 0xF)
// matches the pixel packing in epd_draw_pixel and display.html renderFramebuffer
const gray = new Uint8Array(EPD_W * EPD_H);
for (let y = 0; y < EPD_H; y++) {
  const rowBase = y * (EPD_W / 2);
  for (let x = 0; x < EPD_W; x++) {
    const byte = fb[rowBase + Math.floor(x / 2)];
    const nibble = x % 2 === 1 ? (byte >> 4) & 0xf : byte & 0xf;
    gray[y * EPD_W + x] = Math.round((nibble / 15) * 255);
  }
}

// --- Encode as PNG (no external deps) ---
// CRC32 lookup table
const crcTable = new Uint32Array(256);
for (let n = 0; n < 256; n++) {
  let c = n;
  for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
  crcTable[n] = c;
}
function crc32(buf) {
  let c = 0xffffffff;
  for (let i = 0; i < buf.length; i++) c = crcTable[(c ^ buf[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

function chunk(type, data) {
  const typeBytes = Buffer.from(type, "ascii");
  const lenBuf = Buffer.allocUnsafe(4);
  lenBuf.writeUInt32BE(data.length, 0);
  const crcInput = Buffer.concat([typeBytes, data]);
  const crcBuf = Buffer.allocUnsafe(4);
  crcBuf.writeUInt32BE(crc32(crcInput), 0);
  return Buffer.concat([lenBuf, typeBytes, data, crcBuf]);
}

// IHDR: 960×540, 8-bit grayscale
const ihdrData = Buffer.allocUnsafe(13);
ihdrData.writeUInt32BE(EPD_W, 0);
ihdrData.writeUInt32BE(EPD_H, 4);
ihdrData[8] = 8; // bit depth
ihdrData[9] = 0; // color type: grayscale
ihdrData[10] = 0; // compression
ihdrData[11] = 0; // filter
ihdrData[12] = 0; // interlace

// Raw scanlines: filter byte (0=None) + row pixels
const raw = Buffer.allocUnsafe(EPD_H * (1 + EPD_W));
for (let y = 0; y < EPD_H; y++) {
  const rowOffset = y * (1 + EPD_W);
  raw[rowOffset] = 0;
  raw.set(gray.subarray(y * EPD_W, (y + 1) * EPD_W), rowOffset + 1);
}

const png = Buffer.concat([
  Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), // PNG signature
  chunk("IHDR", ihdrData),
  chunk("IDAT", deflateSync(raw, { level: 6 })),
  chunk("IEND", Buffer.alloc(0)),
]);

// --- Write output ---
mkdirSync(dirname(outPath), { recursive: true });
writeFileSync(outPath, png);
console.log(`Saved ${outPath} (${EPD_W}×${EPD_H}, ${png.length} bytes)`);
