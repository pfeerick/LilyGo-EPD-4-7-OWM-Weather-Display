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
  main_pc.cpp
  owm_http.cpp
  epdiy_pc/epdiy.c
  epdiy_pc/font.c
  # miniz split-file amalgamation (3.x)
  miniz/miniz.c
  miniz/miniz_tdef.c
  miniz/miniz_tinfl.c
  miniz/miniz_zip.c
)

target_include_directories(simulator PRIVATE
  ${CMAKE_SOURCE_DIR}                           # pc_stubs.h, owm_http.h
  ${CMAKE_SOURCE_DIR}/esp_stubs                 # esp_attr.h, esp_err.h, xtensa/
  ${CMAKE_SOURCE_DIR}/miniz                     # miniz.h and split headers
  ${ROOT}/include                               # config.h, fonts/, images/
  ${ROOT}/src                                   # main.cpp
  ${ROOT}/.pio/libdeps/display/epdiy/src        # epdiy.h, epd_internals.h, epd_highlevel.h
  ${ROOT}/.pio/libdeps/display/ArduinoJson/src  # ArduinoJson.h
)

target_compile_definitions(simulator PRIVATE PC_SIMULATOR_BUILD=1)

if(WIN32)
  target_link_libraries(simulator PRIVATE winhttp)
endif()
