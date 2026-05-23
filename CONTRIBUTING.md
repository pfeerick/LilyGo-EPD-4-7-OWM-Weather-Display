# Contributing

## Code style

Two formatters are enforced by CI:

- **C/C++**: [clang-format](https://clang.llvm.org/docs/ClangFormat.html) with the Google base style at 120-column line width (see [.clang-format](.clang-format))
- **Web files**: [Biome](https://biomejs.dev/) (see [biome.json](biome.json))

Run clang-format on changed C/C++ files before submitting, and `bun run format` (or `bunx biome format --write`) on any changed web files.

## Commit style

Use [Conventional Commits](https://www.conventionalcommits.org):

```
<type>(<optional scope>): <short summary>
```

Allowed types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `perf`, `build`, `ci`, `chore`

## Pull requests

Open a PR against `main`. Keep changes focused — one logical change per PR makes review easier.

---

## Building from source

To compile you will need the following libraries, compatible versions of which will be installed automatically by PlatformIO:

- <https://github.com/vroland/epdiy> (display driver — replaced the frozen LilyGo-EPD47 fork)
- <https://github.com/bblanchon/ArduinoJson>

The setup portal HTML lives in `web/config.html`. On each PlatformIO build, `scripts/embed_html.py` automatically regenerates `include/config_html.h` (the embedded PROGMEM copy) — no separate filesystem upload is needed.

## Previewing the web UI

You can iterate on the setup portal page without flashing firmware using the included Bun preview server. The required Bun version is pinned in `.bun-version`. Install [Bun](https://bun.sh) if you do not have it already:

```sh
# Install Bun
curl -fsSL https://bun.sh/install | bash          # Linux / macOS
powershell -c "irm bun.sh/install.ps1 | iex"      # Windows (PowerShell)
```

```sh
# Install dependencies and start the preview server
bun install
bun run dev
```

Then open **http://localhost:3000** in a browser. The page is served from `web/config.html` with placeholder values filled from the mock config in `index.js`. `POST /save` is stubbed — it logs the submitted values and returns a confirmation page without restarting anything.

After editing `web/config.html`, refresh the browser to see changes immediately. The next `pio run` will regenerate `include/config_html.h` automatically.

## Simulating the display

The `simulator/` directory contains a native Windows build that runs the real rendering code (`DisplayWeather()`) against live OWM data and serves the resulting e-ink framebuffer to a browser canvas — no hardware required.

### Prerequisites

- [CMake](https://cmake.org/) ≥ 3.20 — `scoop install cmake`
- [Ninja](https://ninja-build.org/) — `scoop install ninja`
- GCC/G++ — `scoop install gcc`
- An [OpenWeatherMap](https://openweathermap.org/) API key with One Call API 3.0 access

### Build

```powershell
cd simulator
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output is `simulator/build/simulator.exe`.

### Run

Copy the credentials template and fill in your values:

```powershell
Copy-Item simulator\.env.example simulator\.env
# then edit simulator\.env
```

`simulator/.env` is gitignored. The Bun server loads it automatically. Then start it as usual:

```powershell
bun run dev
```

Open **http://localhost:3000/display** in a browser to see the rendered display. The framebuffer is cached for 5 minutes by default; click **Re-fetch OWM** to force a refresh, or set `REFRESH_TTL` (milliseconds) in the environment to change the cache duration.

The simulator writes 259,200 raw bytes (960 × 540 px, 4bpp packed grayscale) to stdout and exits. The Bun server spawns it on demand and converts the framebuffer to a PNG for the browser canvas.

### How it tracks `main.cpp`

The simulator `#include`s `src/main.cpp` directly — all rendering changes are picked up automatically on the next rebuild. Hardware-only functions (`BeginSleep`, `StartWiFi`, `DecodeWeather`, etc.) are compiled out with `#ifndef PC_SIMULATOR_BUILD` guards and replaced by stubs in `simulator/main_pc.cpp`. If you add new weather fields to `DecodeWeather`, update the PC version in `main_pc.cpp` to match.
