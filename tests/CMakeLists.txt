enable_testing()

add_executable(test_dom test_dom.c)
target_link_libraries(test_dom PRIVATE eBrowser_engine)
add_test(NAME test_dom COMMAND test_dom)

add_executable(test_html_parser test_html_parser.c)
target_link_libraries(test_html_parser PRIVATE eBrowser_engine)
add_test(NAME test_html_parser COMMAND test_html_parser)

add_executable(test_css_parser test_css_parser.c)
target_link_libraries(test_css_parser PRIVATE eBrowser_engine)
add_test(NAME test_css_parser COMMAND test_css_parser)

add_executable(test_layout test_layout.c)
target_link_libraries(test_layout PRIVATE eBrowser_engine)
add_test(NAME test_layout COMMAND test_layout)

add_executable(test_url test_url.c)
target_link_libraries(test_url PRIVATE eBrowser_network)
add_test(NAME test_url COMMAND test_url)

add_executable(test_input test_input.c)
target_link_libraries(test_input PRIVATE eBrowser_input)
add_test(NAME test_input COMMAND test_input)

add_executable(test_platform test_platform.c)
target_link_libraries(test_platform PRIVATE eBrowser_platform)
add_test(NAME test_platform COMMAND test_platform)

add_executable(test_crypto test_crypto.c)
target_link_libraries(test_crypto PRIVATE eBrowser_security)
add_test(NAME test_crypto COMMAND test_crypto)

add_executable(test_tls test_tls.c)
target_link_libraries(test_tls PRIVATE eBrowser_security)
add_test(NAME test_tls COMMAND test_tls)

add_executable(test_security test_security.c)
target_link_libraries(test_security PRIVATE eBrowser_security)
add_test(NAME test_security COMMAND test_security)

add_executable(test_svg test_svg.c)
target_link_libraries(test_svg PRIVATE eBrowser_engine)
add_test(NAME test_svg COMMAND test_svg)

add_executable(test_modules test_modules.c)
target_link_libraries(test_modules PRIVATE eBrowser_engine eBrowser_network eBrowser_telemetry eBrowser_plugin eBrowser_security)
add_test(NAME test_modules COMMAND test_modules)

# --- New module tests ---

add_executable(test_memory_safety test_memory_safety.c)
target_link_libraries(test_memory_safety PRIVATE eb_memory_safety)
add_test(NAME test_memory_safety COMMAND test_memory_safety)

add_executable(test_sandbox test_sandbox.c)
target_link_libraries(test_sandbox PRIVATE eb_sandbox)
add_test(NAME test_sandbox COMMAND test_sandbox)

add_executable(test_firewall test_firewall.c)
target_link_libraries(test_firewall PRIVATE eb_firewall)
add_test(NAME test_firewall COMMAND test_firewall)

add_executable(test_anti_fingerprint test_anti_fingerprint.c)
target_link_libraries(test_anti_fingerprint PRIVATE eb_anti_fingerprint m)
add_test(NAME test_anti_fingerprint COMMAND test_anti_fingerprint)

add_executable(test_extension test_extension.c)
target_link_libraries(test_extension PRIVATE eb_extensions)
add_test(NAME test_extension COMMAND test_extension)

add_executable(test_privacy test_privacy.c)
target_link_libraries(test_privacy PRIVATE eb_privacy)
add_test(NAME test_privacy COMMAND test_privacy)

add_executable(test_tab_manager test_tab_manager.c)
target_link_libraries(test_tab_manager PRIVATE eb_ui)
add_test(NAME test_tab_manager COMMAND test_tab_manager)

add_executable(test_bookmark test_bookmark.c)
target_link_libraries(test_bookmark PRIVATE eb_ui)
add_test(NAME test_bookmark COMMAND test_bookmark)

# --- Benchmark ---
add_executable(benchmark benchmark.c)
target_link_libraries(benchmark PRIVATE
    eBrowser_engine eBrowser_network eBrowser_security eBrowser_plugin eBrowser_telemetry
    eb_memory_safety eb_sandbox eb_firewall eb_anti_fingerprint eb_privacy
    eb_dns_security eb_extensions eb_perf eb_ui m)
target_include_directories(benchmark PRIVATE /include)

# --- HTTP/2 Server + Load Test ---
add_executable(http2_server http2_server.c)
target_link_libraries(http2_server PRIVATE
    eb_perf eb_memory_safety eb_firewall eb_anti_fingerprint
    eb_privacy eBrowser_security eb_extensions eb_ui pthread m)
target_include_directories(http2_server PRIVATE /include)

add_executable(load_test load_test.c)
target_link_libraries(load_test PRIVATE pthread)

add_executable(load_test_combined load_test_combined.c)
target_link_libraries(load_test_combined PRIVATE
    eb_perf eb_memory_safety eb_firewall eb_anti_fingerprint
    eb_privacy eBrowser_security eb_extensions eb_ui pthread m)
target_include_directories(load_test_combined PRIVATE /include)
