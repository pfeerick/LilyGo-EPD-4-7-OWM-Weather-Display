#pragma once

// Starts AP mode, captive-portal DNS, and the web config server.
// Never returns — device restarts after user saves config.
void enterSetupMode();
