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

The `simulator/` directory runs the real rendering code (`DisplayWeather()`) against live OWM data and renders it to a browser canvas via WebAssembly — no hardware or C++ toolchain required.

### Prerequisites

- An [OpenWeatherMap](https://openweathermap.org/) API key with One Call API 3.0 access
- [Bun](https://bun.sh/) (already required for the rest of the project)

### Run

Download `simulator.js` and `simulator.wasm` from the [latest release](https://github.com/pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display/releases/latest) and place them in `simulator/wasm/`:

```sh
mkdir -p simulator/wasm
curl -L -o simulator/wasm/simulator.js   https://github.com/pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display/releases/download/latest/simulator-wasm-latest.js
curl -L -o simulator/wasm/simulator.wasm https://github.com/pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display/releases/download/latest/simulator-wasm-latest.wasm
```

Set up credentials and start the server:

```sh
cp simulator/.env.example simulator/.env
# edit simulator/.env with your OWM API key and location
bun run dev
```

Open **http://localhost:3000/display** to see the rendered display.

### Rebuilding the WASM module

Only needed when you change the C++ rendering code. Requires [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) and Ninja.

```sh
emcmake cmake -B simulator/build -S simulator -G Ninja -DCMAKE_BUILD_TYPE=Release
emmake cmake --build simulator/build
```

The build writes `simulator.js` and `simulator.wasm` straight into `simulator/wasm/` (gitignored). When your PR is merged to `main`, CI rebuilds the artifacts and publishes them to the `latest` release.

### How it tracks `main.cpp`

The WASM module `#include`s `src/main.cpp` directly — all rendering changes are picked up automatically on the next WASM rebuild. Hardware-only functions (`BeginSleep`, `StartWiFi`, `DecodeWeather`, etc.) are compiled out with `#ifndef SIMULATOR_BUILD` guards and replaced by stubs in `simulator/main_wasm.cpp`. If you add new weather fields to `DecodeWeather`, update the stub version in `main_wasm.cpp` to match.
