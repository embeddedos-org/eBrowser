if(eBrowser_PLATFORM_SDL2 AND eBrowser_BUILD_STANDALONE)
    # SDL2 must be located *before* the target is declared: main_sdl2.c includes
    # <SDL.h> unconditionally, so declaring the target first and only warning
    # afterwards turns a missing optional dependency into a hard build failure.
    set(EBROWSER_SDL2_MODE "none")

    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        set(EBROWSER_SDL2_MODE "cmake")
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
            if(SDL2_FOUND)
                set(EBROWSER_SDL2_MODE "pkgconfig")
            endif()
        endif()
        if(NOT SDL2_FOUND)
            find_library(SDL2_LIB NAMES SDL2 sdl2)
            find_path(SDL2_INC NAMES SDL.h PATH_SUFFIXES SDL2)
            if(SDL2_LIB AND SDL2_INC)
                set(EBROWSER_SDL2_MODE "manual")
            endif()
        endif()
    endif()

    if(EBROWSER_SDL2_MODE STREQUAL "none")
        message(STATUS "SDL2 not found - skipping the standalone eBrowser GUI target.")
        message(STATUS "  Enable it by installing SDL2:")
        message(STATUS "    apt install libsdl2-dev | brew install sdl2 | pacman -S mingw-w64-x86_64-SDL2")
    else()
        add_executable(eBrowser
            ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/main_sdl2.c
            ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/platform_open_url.c
        )
        target_link_libraries(eBrowser PRIVATE eBrowser_browser lvgl)
        target_include_directories(eBrowser PRIVATE ${CMAKE_SOURCE_DIR}/include)

        if(EBROWSER_SDL2_MODE STREQUAL "cmake")
            target_link_libraries(eBrowser PRIVATE SDL2::SDL2)
        elseif(EBROWSER_SDL2_MODE STREQUAL "pkgconfig")
            target_include_directories(eBrowser PRIVATE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(eBrowser PRIVATE ${SDL2_LIBRARIES})
        else()
            target_include_directories(eBrowser PRIVATE ${SDL2_INC})
            target_link_libraries(eBrowser PRIVATE ${SDL2_LIB})
        endif()

        if(WIN32)
            target_link_libraries(eBrowser PRIVATE ws2_32)
        endif()

        message(STATUS "eBrowser standalone GUI: enabled (SDL2 found via ${EBROWSER_SDL2_MODE})")
    endif()
endif()


if(eBrowser_PLATFORM_EOS)
    add_library(eBrowser_eos STATIC ${CMAKE_CURRENT_SOURCE_DIR}/eos/main_eos.c)
    target_link_libraries(eBrowser_eos PUBLIC eBrowser_browser lvgl)
    target_include_directories(eBrowser_eos PUBLIC ${CMAKE_SOURCE_DIR}/include)
endif()

if(eBrowser_PLATFORM_WEB)
    add_executable(eBrowser_web ${CMAKE_CURRENT_SOURCE_DIR}/web/main_web.c)
    target_link_libraries(eBrowser_web PRIVATE eBrowser_browser lvgl)
    target_include_directories(eBrowser_web PRIVATE ${CMAKE_SOURCE_DIR}/include)
endif()
