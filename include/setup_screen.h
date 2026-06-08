#pragma once

// Renders the e-paper "SETUP MODE" screen: WiFi/config-portal QR codes plus
// instructions, shown when the device has no valid config or can't connect.
void DisplaySetupScreen(const char* ap_name);
