cmake_minimum_required(VERSION 3.20)
project(epd_simulator CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/W3 /wd4244 /wd4267 /wd4305)
else()
  add_compile_options(-Wall -Wno-unused-function -Wno-sign-compare -Wno-narrowing)
endif()

set(ROOT "${CMAKE_SOURCE_DIR}/..")

add_executable(simulator
  main_wasm.cpp
  epdiy_pc/epdiy.c
  epdiy_pc/font.c
  # miniz split-file amalgamation (3.x)
  miniz/miniz.c
  miniz/miniz_tdef.c
  miniz/miniz_tinfl.c
  miniz/miniz_zip.c
)

target_include_directories(simulator PRIVATE
  ${CMAKE_SOURCE_DIR}                           # pc_stubs.h
  ${CMAKE_SOURCE_DIR}/esp_stubs                 # esp_attr.h, esp_err.h, xtensa/
  ${CMAKE_SOURCE_DIR}/miniz                     # miniz.h and split headers
  ${ROOT}/include                               # config.h, fonts/, images/
  ${ROOT}/src                                   # main.cpp
  ${ROOT}/.pio/libdeps/display/epdiy/src        # epdiy.h, epd_internals.h, epd_highlevel.h
  ${ROOT}/.pio/libdeps/display/ArduinoJson/src  # ArduinoJson.h
)

target_compile_definitions(simulator PRIVATE SIMULATOR_BUILD=1)

if(EMSCRIPTEN)
  set_target_properties(simulator PROPERTIES
    OUTPUT_NAME "simulator"
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/wasm"
  )
  target_link_options(simulator PRIVATE
    -sEXPORTED_FUNCTIONS=_wasm_init,_wasm_set_config,_wasm_render,_wasm_get_framebuffer,_wasm_fb_size,_malloc,_free
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU8,stringToUTF8,lengthBytesUTF8
    -sMODULARIZE=1
    "-sEXPORT_NAME=SimulatorModule"
    -sINITIAL_MEMORY=33554432
    -sALLOW_MEMORY_GROWTH=1
    -sNO_EXIT_RUNTIME=1
    --no-entry
  )
endif()
