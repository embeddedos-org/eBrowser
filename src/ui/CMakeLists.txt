add_library(eb_ui tab_manager.c bookmark.c download.c devtools.c theme.c reader_mode.c)
target_include_directories(eb_ui PUBLIC ${CMAKE_SOURCE_DIR}/include)
# bookmark.c logs load/save diagnostics (#67); PUBLIC so dependents
# (eBrowser_browser, test_bookmark) link the logger too.
target_link_libraries(eb_ui PUBLIC eBrowser_telemetry)
