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

- <https://github.com/Xinyuan-LilyGO/LilyGo-EPD47>
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
