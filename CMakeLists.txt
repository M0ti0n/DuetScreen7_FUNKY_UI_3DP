cmake_minimum_required(VERSION 3.16)
project(funky_ui C CXX)

# Where the app runs:
#   sdl      - desktop simulator window (Linux / WSL2 with WSLg)
#   linux    - DuetScreen 7 (framebuffer + evdev touch)
#   headless - no window; renders screenshots (used by CI and for previews)
set(FUNKY_PLATFORM "sdl" CACHE STRING "sdl | linux | headless")
set_property(CACHE FUNKY_PLATFORM PROPERTY STRINGS sdl linux headless)

set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- LVGL -------------------------------------------------------------
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
add_subdirectory(lib/lvgl)
target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR}/config)

if(FUNKY_PLATFORM STREQUAL "sdl")
  find_package(SDL2 REQUIRED)
  target_compile_definitions(lvgl PUBLIC FUNKY_PLATFORM_SDL=1)
  target_include_directories(lvgl PUBLIC ${SDL2_INCLUDE_DIRS})
  target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES})
  set(PLATFORM_SRC src/platform/platform_sdl.c)
elseif(FUNKY_PLATFORM STREQUAL "linux")
  target_compile_definitions(lvgl PUBLIC FUNKY_PLATFORM_LINUX=1)
  set(PLATFORM_SRC src/platform/platform_linux.c)
elseif(FUNKY_PLATFORM STREQUAL "headless")
  set(PLATFORM_SRC src/platform/platform_headless.c)
else()
  message(FATAL_ERROR "Unknown FUNKY_PLATFORM '${FUNKY_PLATFORM}'")
endif()

# ---- App --------------------------------------------------------------
add_executable(funky_ui
  src/main.c
  src/model/printer.c
  src/ui/home.c
  src/ui/drawer_page.c
  src/ui/estop.c
  ${PLATFORM_SRC})
target_include_directories(funky_ui PRIVATE src)
target_link_libraries(funky_ui PRIVATE lvgl m)
# Default asset folder = the repo's assets/ (override at runtime with FUNKY_ASSETS)
target_compile_definitions(funky_ui PRIVATE FUNKY_DEFAULT_ASSETS="${CMAKE_SOURCE_DIR}/assets")
if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(funky_ui PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
