# Fuzzing harnesses — requires Clang with libFuzzer
if(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR eBrowser_FORCE_FUZZ)
    message(STATUS "Building fuzzing harnesses (Clang detected)")

    set(FUZZ_FLAGS "-fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -g")
    set(FUZZ_LIBS eBrowser_engine eBrowser_network eBrowser_security
                  eb_firewall eb_privacy eb_extensions)

    # HTML parser fuzzer
    add_executable(fuzz_html_parser fuzz_html_parser.c)
    target_link_libraries(fuzz_html_parser PRIVATE ${FUZZ_LIBS})
    target_include_directories(fuzz_html_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(fuzz_html_parser PROPERTIES
        COMPILE_FLAGS "${FUZZ_FLAGS}"
        LINK_FLAGS "${FUZZ_FLAGS}")

    # CSS parser fuzzer
    add_executable(fuzz_css_parser fuzz_css_parser.c)
    target_link_libraries(fuzz_css_parser PRIVATE ${FUZZ_LIBS})
    target_include_directories(fuzz_css_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(fuzz_css_parser PROPERTIES
        COMPILE_FLAGS "${FUZZ_FLAGS}"
        LINK_FLAGS "${FUZZ_FLAGS}")

    # URL parser fuzzer
    add_executable(fuzz_url_parser fuzz_url_parser.c)
    target_link_libraries(fuzz_url_parser PRIVATE ${FUZZ_LIBS})
    target_include_directories(fuzz_url_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(fuzz_url_parser PROPERTIES
        COMPILE_FLAGS "${FUZZ_FLAGS}"
        LINK_FLAGS "${FUZZ_FLAGS}")

    # XSS scanner fuzzer
    add_executable(fuzz_xss fuzz_xss.c)
    target_link_libraries(fuzz_xss PRIVATE ${FUZZ_LIBS})
    target_include_directories(fuzz_xss PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(fuzz_xss PROPERTIES
        COMPILE_FLAGS "${FUZZ_FLAGS}"
        LINK_FLAGS "${FUZZ_FLAGS}")

    # Firewall + tracker blocker fuzzer
    add_executable(fuzz_firewall fuzz_firewall.c)
    target_link_libraries(fuzz_firewall PRIVATE ${FUZZ_LIBS})
    target_include_directories(fuzz_firewall PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(fuzz_firewall PROPERTIES
        COMPILE_FLAGS "${FUZZ_FLAGS}"
        LINK_FLAGS "${FUZZ_FLAGS}")

else()
    message(STATUS "Skipping fuzz targets (requires Clang with libFuzzer)")
endif()
