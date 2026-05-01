cmake_minimum_required(VERSION 3.16)
project(eBrowser
    VERSION 2.0.0
    DESCRIPTION "eBrowser - The Most Secure & Powerful Lightweight Browser"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Feature flags ---
option(eBrowser_BUILD_ENGINE    "Build HTML/CSS/Layout engine"           ON)
option(eBrowser_BUILD_RENDER    "Build rendering backend"                ON)
option(eBrowser_BUILD_NETWORK   "Build HTTP/HTTPS network stack"        ON)
option(eBrowser_BUILD_INPUT     "Build input abstraction layer"         ON)
option(eBrowser_BUILD_BROWSER   "Build browser UI application"         ON)
option(eBrowser_BUILD_JS        "Build optional JavaScript engine"     OFF)
option(eBrowser_BUILD_SECURITY  "Build security and crypto layer"      ON)
option(eBrowser_BUILD_STANDALONE "Build standalone executable"          ON)
option(eBrowser_BUILD_SANDBOX   "Build process sandbox"                ON)
option(eBrowser_BUILD_FIREWALL  "Build application firewall"           ON)
option(eBrowser_BUILD_PRIVACY   "Build privacy & tracker blocker"      ON)
option(eBrowser_BUILD_EXTENSIONS "Build WebExtensions system"          ON)
option(eBrowser_BUILD_PERF      "Build performance engine"             ON)
option(eBrowser_BUILD_UI        "Build advanced UI modules"            ON)

# --- Platform detection ---
if(CMAKE_TOOLCHAIN_FILE MATCHES "eos\\.cmake$")
    set(eBrowser_PLATFORM_EOS ON)
    add_definitions(-DeBrowser_PLATFORM_EOS=1)
elseif(EMSCRIPTEN)
    set(eBrowser_PLATFORM_WEB ON)
    add_definitions(-DeBrowser_PLATFORM_WEB=1)
else()
    set(eBrowser_PLATFORM_SDL2 ON)
    add_definitions(-DeBrowser_PLATFORM_SDL2=1)
endif()

# --- Platform defines ---
if(UNIX AND NOT APPLE)
    add_definitions(-D_GNU_SOURCE)
endif()

# --- Compiler warnings & hardening ---
if(MSVC)
    add_compile_options(/W3 /GS /DYNAMICBASE /NXCOMPAT)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(
        -Wall -Wextra -Wno-unused-parameter
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -fPIE
    )
    if(NOT APPLE)
        add_link_options(-pie -Wl,-z,relro,-z,now,-z,noexecstack)
    endif()
endif()

# --- LVGL ---
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/lv_conf.h" CACHE STRING "" FORCE)
add_definitions(-DLV_CONF_INCLUDE_SIMPLE=1)
add_definitions(-DLV_USE_DRAW_SW_COMPLEX=0)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/lvgl/CMakeLists.txt")
    message(STATUS "Using LVGL from extern/lvgl/ submodule")
    set(LV_USE_DRAW_SW_ASM "none" CACHE STRING "" FORCE)
    add_subdirectory(extern/lvgl)
else()
    message(STATUS "LVGL submodule not found - auto-downloading via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(lvgl
        GIT_REPOSITORY https://github.com/lvgl/lvgl.git
        GIT_TAG        release/v9.2
        GIT_SHALLOW    TRUE
    )
    set(LV_USE_DRAW_SW_ASM "none" CACHE STRING "" FORCE)
    FetchContent_MakeAvailable(lvgl)
    # Remove ARM assembly files on non-ARM platforms (x86/x64)
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64|ARM")
        get_target_property(_lvgl_srcs lvgl SOURCES)
        if(_lvgl_srcs)
            list(FILTER _lvgl_srcs EXCLUDE REGEX "\\.S$")
            set_target_properties(lvgl PROPERTIES SOURCES "${_lvgl_srcs}")
        endif()
    endif()
endif()

# --- Core engine ---
if(eBrowser_BUILD_ENGINE)
    add_subdirectory(src/engine)
endif()

# --- Rendering backend ---
if(eBrowser_BUILD_RENDER)
    add_subdirectory(src/render)
endif()

# --- Network stack ---
if(eBrowser_BUILD_NETWORK)
    add_subdirectory(src/network)
endif()

# --- Input layer ---
if(eBrowser_BUILD_INPUT)
    add_subdirectory(src/input)
endif()

# --- Security & Crypto ---
if(eBrowser_BUILD_SECURITY)
    add_subdirectory(src/security)
endif()

# --- Sandbox ---
if(eBrowser_BUILD_SANDBOX)
    add_subdirectory(src/security/sandbox)
    add_subdirectory(src/security/memory)
    add_subdirectory(src/security/firewall)
    add_subdirectory(src/security/anti_fingerprint)
    add_subdirectory(src/security/dns)
endif()

# --- Privacy & Tracker Blocker ---
if(eBrowser_BUILD_PRIVACY)
    add_subdirectory(src/privacy)
endif()

# --- WebExtensions ---
if(eBrowser_BUILD_EXTENSIONS)
    add_subdirectory(src/extensions)
endif()

# --- Performance Engine ---
if(eBrowser_BUILD_PERF)
    add_subdirectory(src/perf)
endif()

# --- Advanced UI ---
if(eBrowser_BUILD_UI)
    add_subdirectory(src/ui)
endif()

# --- Telemetry ---
add_subdirectory(src/telemetry)

# --- Plugin System ---
add_subdirectory(src/plugin)

# --- Platform abstraction ---
add_subdirectory(platform)

# --- Browser UI ---
if(eBrowser_BUILD_BROWSER)
    add_subdirectory(src/browser)
endif()

# --- Port layer ---
add_subdirectory(port)

# --- Tests ---
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Fuzzing ---
option(eBrowser_BUILD_FUZZ "Build fuzzing harnesses (requires Clang)" OFF)
if(eBrowser_BUILD_FUZZ)
    add_subdirectory(fuzz)
endif()
