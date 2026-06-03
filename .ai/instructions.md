# LilyGo EPD 4.7" OWM Weather Display — AI Collaboration Guide

This document provides essential context for AI models working in this repository. Adhering to these guidelines will ensure consistency and maintain code quality.

## 1. Project Overview & Purpose

- **Primary Goal:** Firmware for the LilyGo T5 4.7" e-paper display module (ESP32). Fetches current weather and 8-day forecasts from the OpenWeatherMap One Call API 3.0 and renders them on the 4.7" 960×540 e-paper panel using the epdiy driver.
- **Hardware:** TTGO T5 4.7" (also sold as LilyGo T5 4.7") — original non-S3 revision; ESP32-WROVER-E, 16 MB flash, 8 MB PSRAM, 4.7" 960×540 16-grey e-paper panel.
- **Configuration:** WiFi credentials and OWM API settings are entered via a captive-portal setup wizard on first boot (or when the button is held), stored in LittleFS. A runtime display-config portal is also served over WiFi for adjusting display preferences.
- **Simulator:** A WebAssembly module that runs the real rendering code against live OWM data in a browser canvas — no hardware required. Pre-built WASM artifacts are published with each release.

## 2. Core Technologies & Stack

- **Language:** C++ (Arduino framework on ESP32)
- **Build System:** PlatformIO (`platformio.ini`) — single `[env:display]` environment; do not use Arduino IDE
- **Key Libraries:**
  - `epdiy @ 2.0.0` — e-paper display driver (replaces the frozen upstream LilyGo-EPD47 fork)
  - `bblanchon/ArduinoJson @ ^7.4.3` — JSON deserialisation of OWM API responses
- **Filesystem:** LittleFS — setup portal HTML is **embedded at compile time** (no separate filesystem flash step)
- **WASM Simulator:** Emscripten + CMake + Ninja; builds `simulator.js` + `simulator.wasm` from the same C++ rendering code
- **Web tooling:** Bun (version pinned in `.bun-version`) + Biome for web file formatting and linting
- **Changelog:** git-cliff (`cliff.toml`) — generates release notes from Conventional Commits

## 3. Architecture

```
src/
  main.cpp              # Firmware entrypoint: weather fetch + deep-sleep cycle
  config.cpp / .h       # Runtime config load/save via LittleFS
  display.cpp / .h      # All e-paper rendering (DisplayWeather, graphs, icons)
  icons.cpp / .h        # Weather condition icon bitmaps
  power.cpp / .h        # Deep-sleep and wake management
  setup_portal.cpp / .h # Captive-portal WiFi + OWM config wizard
  weather_api.cpp / .h  # OpenWeatherMap HTTP fetch + JSON parse
include/
  defaults.h            # Compile-time defaults (location, units, etc.)
  forecast_record.h     # Shared weather data structs (Forecast_record_type, etc.)
  config_html.h         # Auto-generated PROGMEM copy of web/config.html — do not edit
  update_html.h         # Auto-generated PROGMEM copy of web/update.html — do not edit
  fonts/                # E-paper font headers
  images/               # Icon/image headers
  translations/         # Locale string tables
simulator/
  src/                  # WASM build entry: main_wasm.cpp with stubs for hardware-only functions
  CMakeLists.txt        # Emscripten CMake build config
  wasm/                 # Pre-built simulator.js + simulator.wasm (gitignored; download from releases)
  .env.example          # OWM API key + location template
web/
  config.html           # Setup portal HTML — source of truth; never edit config_html.h directly
  display.html          # Display configuration portal page
docs/
  index.html            # GitHub Pages firmware flasher (ESP Web Tools; served at pfeerick.github.io/LilyGo-EPD-4-7-OWM-Weather-Display/)
scripts/
  embed_html.py         # Pre-build: regenerates include/config_html.h from web/config.html
  merge_binary.py       # Post-build: merges firmware + bootloader + partition table into factory binary
  screenshot.js         # Bun script: renders WASM display to PNG without a browser
```

**Deep-sleep model:** The firmware wakes from deep sleep, fetches weather data, renders the display, then immediately returns to deep sleep until the next update interval. There is no persistent `loop()` polling — `setup()` does all the work.

**HTML embedding:** `web/config.html` and `web/update.html` are the authoritative sources. `scripts/embed_html.py` runs as a PlatformIO pre-build script and regenerates `include/config_html.h` and `include/update_html.h` on every build. Never edit these generated headers directly — they are overwritten on every build.

**Simulator seam:** `simulator/src/main_wasm.cpp` `#include`s `src/main.cpp` directly, so all rendering changes are picked up automatically on the next WASM rebuild. Hardware-only functions (`BeginSleep`, `StartWiFi`, `DecodeWeather`, etc.) are compiled out with `#ifndef SIMULATOR_BUILD` guards and replaced by stubs in `main_wasm.cpp`. If you add new weather fields to `DecodeWeather`, update the stub in `main_wasm.cpp` to match.

## 4. Coding Conventions

- **C/C++ style:** `.clang-format` is present — Google base style, 120-character line limit, 2-space indentation, `SortIncludes: Never`. Run `clang-format -i` on changed `.cpp`/`.h` files before committing. **Do not run clang-format on `include/fonts/` or `include/images/`** — those directories contain auto-generated binary data and have nested `.clang-format` files with `DisableFormat: true` to exclude them from CI checks.
- **Naming:** Arduino camelCase conventions for variables and functions (`displayWeather`, `fetchWeatherData`).
- **Web files:** Biome enforced — 2-space indentation, 120-column limit, double quotes, trailing commas, semicolons. Run `bun run format` after editing any file under `web/` or `docs/`.
- **Generated files:** Do not manually edit `include/config_html.h` or `include/update_html.h` — both are regenerated on every `pio run`.

## 5. Key Files & Entrypoints

- `platformio.ini` — single `[env:display]`; `espressif32 @ 7.0.1`, `esp32dev` board, LittleFS, `min_spiffs` partitions, `-DBOARD_HAS_PSRAM`, epdiy and ArduinoJson deps
- `src/main.cpp` — firmware entrypoint; weather fetch + deep-sleep cycle
- `src/display.cpp` — all e-paper rendering logic (`DisplayWeather` and helpers)
- `web/config.html` — setup portal HTML source (embed_html.py publishes it to `include/config_html.h`)
- `web/update.html` — OTA update portal HTML source (embed_html.py publishes it to `include/update_html.h`)
- `include/defaults.h` — compile-time defaults (location, units, language, update interval)
- `include/forecast_record.h` — shared weather data structs
- `simulator/src/main_wasm.cpp` — WASM build entry + hardware function stubs
- `.clang-format` — C++ formatting config
- `biome.json` — web file formatting/linting config
- `cliff.toml` — git-cliff changelog config

## 6. Development Workflow

### Firmware Build & Flash

```sh
pio run                      # build only
pio run --target upload      # flash via USB serial
pio device monitor           # serial monitor at 115200 baud
```

No separate filesystem upload step is needed — the setup portal HTML is embedded at compile time by `embed_html.py`.

### Web UI Development (no hardware required)

```sh
bun install
bun run dev    # preview server at http://localhost:3000 (config portal)
```

After editing `web/config.html`, refresh the browser. The next `pio run` regenerates `include/config_html.h` automatically.

```sh
bun run format        # format web files with Biome (run before committing)
bun run format:check  # lint check only (used in CI)
```

### Simulator (display preview, no hardware required)

Download pre-built WASM artifacts from the [latest release](https://github.com/pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display/releases/latest) and place them in `simulator/wasm/`:

```sh
cp simulator/.env.example simulator/.env
# edit simulator/.env — set OWM_API_KEY and location
bun run dev         # open http://localhost:3000/display
bun run screenshot  # render to simulator-screenshot.png without a browser
```

**Rebuilding WASM** (only needed when changing C++ rendering code; requires Emscripten + Ninja):

```sh
emcmake cmake -B simulator/build -S simulator -G Ninja -DCMAKE_BUILD_TYPE=Release
emmake cmake --build simulator/build
```

Output goes to `simulator/wasm/` (gitignored). CI rebuilds and publishes WASM artifacts on every merge to `main`.

## 7. CI/CD

Five GitHub Actions workflows run on pull requests (or push to main):

| Workflow | Trigger (path filters) | What it does |
|---|---|---|
| `compile-check.yml` | `src/`, `include/`, `platformio.ini` | PlatformIO firmware build; uploads firmware binaries as artifacts |
| `lint.yml` | `src/`, `include/`, `simulator/`, `web/`, `docs/`, `index.js` | clang-format-18 dry-run on C++ files; Biome format check on web files |
| `simulator-build.yml` | `simulator/`, `src/`, `include/` | Emscripten CMake WASM build; uploads `simulator.js` + `simulator.wasm` |
| `release.yml` | Push to `main` or version tags | Runs all three above; creates/updates GitHub release with git-cliff changelog and firmware + WASM artifacts |
| `pages.yml` | Push to `main` (`docs/**`) | Deploys `docs/` to GitHub Pages (firmware flasher at `pfeerick.github.io/LilyGo-EPD-4-7-OWM-Weather-Display/`) |

**Before submitting a PR:**
- Run `clang-format -i` on any changed `.cpp`/`.h` files
- Run `bun run format` on any changed files under `web/` or `docs/`
- Confirm `pio run` succeeds locally

**GitHub Pages setup (one-time manual step):** For `pages.yml` to deploy successfully, go to **Settings > Pages > Build and deployment** and set Source to **GitHub Actions**.

## 8. AI Collaboration Guidelines

### Branches

Always create a new branch before starting any work. Never commit directly to `main`.

- Name the branch after the conventional commit type and a short description: `feat/add-humidity-graph`, `fix/graph-overflow`, `chore/update-deps`
- Create with `git checkout -b <branch-name>` before making any changes
- This applies to all change types: features, fixes, refactors, docs, chores, etc.

### Commits

- Prefer small, focused commits.
- Use Conventional Commits format: `type(scope): description`
  - Examples: `feat(display): add humidity graph`, `fix(weather_api): handle missing hourly forecast`
- Allowed types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `perf`, `build`, `ci`, `chore`
- When correcting an earlier commit on the same branch, use a fixup commit rather than amending or creating a loose fix:
  ```sh
  git commit --fixup=<sha-of-commit-being-fixed>
  ```
  Before the PR is merged, squash fixups into their targets with:
  ```sh
  git rebase --autosquash origin/main
  ```

### Pull Requests

Push branches to `origin` (`pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display`). When creating a PR with `gh`, always pass `--repo pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display` — without it, `gh` defaults to the upstream `DzikuVx/LilyGo-EPD-4-7-OWM-Weather-Display` repo.

```sh
git push -u origin <branch-name>
gh pr create --repo pfeerick/LilyGo-EPD-4-7-OWM-Weather-Display --title "..." --body "..."
```

### Symlink quirk: `.cursorrules` and `.windsurfrules`

These two files are git symlinks (mode `120000`) whose stored content is the bare target path `.ai/instructions.md` with no trailing newline — which is correct; newlines are not valid in filesystem paths. GitHub suppresses the "no newline at end of file" warning for `.md` files (likely because trailing whitespace is meaningful in Markdown), so only these two non-markdown symlinks show the warning in PR diffs. Do not attempt to add a trailing newline to fix it — it would corrupt the symlink target.

### What Lives Where

- Agent-facing guidance → `.ai/instructions.md` (this file)
- User-facing docs → `README.md` and `CONTRIBUTING.md`
- Don't duplicate content across these files.

### Keeping Instructions Current

When a repo-specific workflow or repeated correction comes up more than once, update this file so the guidance stays current for future agents.
