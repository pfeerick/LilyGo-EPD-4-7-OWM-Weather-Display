# LilyGo-EPD-4-7-OWM-Weather-Display

Open Weather Map weather station using [LilyGo EPD 4.7" display](https://bit.ly/3exI3Hb)

[![Presetation video](doc/assets/001.png)](https://www.youtube.com/watch?v=TQaVQcld1Pk)

# License

This code created by https://github.com/G6EJD/ is using the GPLv3 https://github.com/Xinyuan-LilyGO/LilyGo-EPD47 library to handle the display and as such falls into the GPLv3 license itself. This situation is described in the https://www.gnu.org/licenses/gpl-faq.html#IfLibraryIsGPL

> If a library is released under the GPL (not the LGPL), does that mean that any software which uses it has to be under the GPL or a GPL-compatible license?

> Yes, because the program actually links to the library. As such, the terms of the GPL apply to the entire combination. The software modules that link with the library may be under various GPL compatible licenses, but the work as a whole must be licensed under the GPL.

This means that the original proprietary license that G6EJD tried to enforce is unlawful as it is not compatible with the GPLv3.

This fork fixes the problem by setting the correct license for the code while keeping the attribution and all the copyright of the original creator.

# Compiling and flashing

Edit `owm_credentials.h` and enter OWM API key as well as the location for which you want to display the weather data

To compile you will need following libraries

* https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
* https://github.com/bblanchon/ArduinoJson

Note: I successfully used the following package versions
 - ESP32 Board Support Package 2.0.17 (3.0 fails - looks like some minor API changes and possibly EPD47 needs some changes)
 - ArduinoJson 6.19.4 (6.20 onwards has compile errors due to removal of depreciated functions)
 - LilyGo-EPD47 1.0.1

In board manager choose ESP32 Dev Module with PSRAM Enabled