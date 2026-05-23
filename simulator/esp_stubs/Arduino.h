#pragma once
// PC stub for Arduino.h — types and macros are provided by pc_stubs.h.
// This header exists so project headers that unconditionally include <Arduino.h>
// compile without error on PC. pc_stubs.h must be included before this.
#ifndef PC_SIMULATOR_BUILD
#error "Arduino.h stub included outside PC simulator build"
#endif
