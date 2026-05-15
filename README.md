# LilyGo EPD 4.7" OpenWeather Weather Display

OpenWeather weather station using [LilyGo EPD 4.7" display](https://bit.ly/3exI3Hb)

[![Presentation video](doc/assets/001.png)](https://www.youtube.com/watch?v=TQaVQcld1Pk)

# License

This code created by https://github.com/G6EJD/ is using the GPLv3 https://github.com/Xinyuan-LilyGO/LilyGo-EPD47 library to handle the display and as such falls into the GPLv3 license itself. This situation is described in the https://www.gnu.org/licenses/gpl-faq.html#IfLibraryIsGPL

> If a library is released under the GPL (not the LGPL), does that mean that any software which uses it has to be under the GPL or a GPL-compatible license?

> Yes, because the program actually links to the library. As such, the terms of the GPL apply to the entire combination. The software modules that link with the library may be under various GPL compatible licenses, but the work as a whole must be licensed under the GPL.

This means that the original proprietary license that G6EJD tried to enforce is unlawful as it is not compatible with the GPLv3.

This fork fixes the problem by setting the correct license for the code while keeping the attribution and all the copyright of the original creator.

# Configuration

Configuration is stored on the device's filesystem (LittleFS) and can be set either through the built-in web setup portal or by pre-seeding credentials at compile time.

## Option 1: Web setup portal (recommended)

Setup mode is entered automatically when:
- No valid configuration exists (e.g. first boot after flashing)
- WiFi connection fails with the saved credentials

To force setup mode manually at any time:
- Hold the **right-most button (IO39)** for 2 seconds while the device boots, **or**
- Hold **IO39** and press **RST** (left-most button) from a running device

When setup mode is active, the display shows the Access Point name and instructions. On your phone or computer:

1. Connect to the WiFi network named **`WeatherSetup-XXYY`** (where `XXYY` are the last 4 hex digits of the device's MAC address)
2. Open any URL in a browser — the captive portal will redirect you automatically, or navigate directly to **`http://192.168.4.1`**
3. Fill in your settings and click **Save & Restart**

The form is pre-filled with the current saved configuration, so you can review and make targeted edits without re-entering everything. The password field is always shown blank for security; leaving it empty on save preserves the existing password.

The raw config can also be retrieved as JSON from **`http://192.168.4.1/config.json`** — useful for backup or inspection. The response includes the WiFi password by default; define `REDACT_PASSWORD_IN_CONFIG_DOWNLOAD` in `include/config.h` to suppress it. The endpoint can be disabled entirely by commenting out `ENABLE_CONFIG_DOWNLOAD` in the same file.

The following settings are available in the portal:

| Category | Settings |
|---|---|
| WiFi | SSID, password |
| OpenWeatherMap | API key, server URL |
| Location | City, latitude, longitude, hemisphere (north/south) |
| Units & language | Metric/Imperial, 16 language options (EN, AR, CZ, EL, FA, FR, GL, HU, JA, KR, LA, LT, MK, SK, SL, VI) |
| Time | POSIX timezone string, NTP server, GMT offset, DST offset |
| Schedule | Update interval (5–720 min), wake hour, sleep hour |
| Debug | Debug display update flag |

## Option 2: Pre-seeding via owm_credentials.h (build-time)

If you prefer to bake your settings into the firmware at compile time (useful for deploying without needing a phone or computer to configure):

1. Copy `include/owm_credentials.h.example` to `include/owm_credentials.h` and fill in your values. The local file is gitignored so your credentials won't be committed.
2. Compile and flash as normal.

On first boot the values are automatically written to the on-device config store and the device starts up without entering the web portal. The portal remains available afterwards — hold IO39 at boot (or hold IO39 + press RST) to enter setup mode and modify any setting.

# Compiling and flashing

To compile you will need the following libraries, compatible versions of which will be installed automatically by PlatformIO:

* https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
* https://github.com/bblanchon/ArduinoJson

The setup portal HTML lives in `web/config.html`. On each PlatformIO build, `scripts/embed_html.py` automatically regenerates `include/config_html.h` (the embedded PROGMEM copy) — no separate filesystem upload is needed.

## Previewing the web UI

You can iterate on the setup portal page without flashing firmware using the included Node.js preview server. The required Node.js version is pinned in `.node-version`. Using [fnm](https://github.com/Schniz/fnm) is recommended:

```sh
# Install fnm (if not already installed)
winget install Schniz.fnm          # Windows
curl -fsSL https://fnm.vercel.app/install | bash   # Linux / macOS

# Install the pinned Node.js version and activate it
fnm install
fnm use

# Install dependencies and start the preview server
npm install
npm run dev
```

Then open **http://localhost:3000** in a browser. The page is served from `web/config.html` with placeholder values filled from the mock config in `index.js`. `POST /save` is stubbed — it logs the submitted values and returns a confirmation page without restarting anything.

After editing `web/config.html`, refresh the browser to see changes immediately. The next `pio run` will regenerate `include/config_html.h` automatically.

# GPIOs
## Buttons
Reset button + 4 user buttons
- IO39 (right-most, used for config mode trigger)
- IO34
- IO35
- IO0 (RST / left-most)

## Touch (6-pin FPC connector)
V | 15 | 14 | 13 | G

## 4-pin Molex Connector
G | SMP | POS | NEG

## 4-pin Molex Connector
V | 12 | 13 | G

## 4-pin Molex Connector
V | 21 | 22 | G
