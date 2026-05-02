add_library(eBrowser_browser STATIC
    browser.c
    omnibox.c
    settings_page.c
)
target_include_directories(eBrowser_browser PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(eBrowser_browser PUBLIC
    eBrowser_engine
    eBrowser_render
    eBrowser_network
    eBrowser_input
    eBrowser_platform
    eb_ui                # Phase B: wire bookmark/download/devtools/reader_mode/theme/tab_manager
    eBrowser_security    # for eb_secure_store_t (settings_page)
    eb_privacy           # for eb_priv_* (incognito mode)
    lvgl
)

# Windows: ShellExecuteA in port/sdl2/platform_open_url.c needs shell32.
if(WIN32)
    target_link_libraries(eBrowser_browser PUBLIC shell32)
endif()
