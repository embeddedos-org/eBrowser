// SPDX-License-Identifier: MIT
#include "eBrowser/extension.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

TEST(test_mgr_init) {
    static eb_ext_manager_t mgr;
    eb_ext_mgr_init(&mgr, "/tmp/extensions");
    ASSERT(mgr.count == 0);
    ASSERT(strcmp(mgr.extensions_dir, "/tmp/extensions") == 0);
    eb_ext_mgr_destroy(&mgr);
}

TEST(test_parse_permission) {
    ASSERT(eb_ext_parse_permission("activeTab") == EB_PERM_ACTIVE_TAB);
    ASSERT(eb_ext_parse_permission("tabs") == EB_PERM_TABS);
    ASSERT(eb_ext_parse_permission("storage") == EB_PERM_STORAGE);
    ASSERT(eb_ext_parse_permission("webRequest") == EB_PERM_WEB_REQUEST);
    ASSERT(eb_ext_parse_permission("cookies") == EB_PERM_COOKIES);
    ASSERT(eb_ext_parse_permission("bookmarks") == EB_PERM_BOOKMARKS);
    ASSERT(eb_ext_parse_permission("downloads") == EB_PERM_DOWNLOADS);
    ASSERT(eb_ext_parse_permission("alarms") == EB_PERM_ALARMS);
    ASSERT(eb_ext_parse_permission("<all_urls>") == EB_PERM_ALL_URLS);
    ASSERT(eb_ext_parse_permission("unknown_perm") == 0);
    ASSERT(eb_ext_parse_permission(NULL) == 0);
}

TEST(test_parse_manifest_basic) {
    static eb_extension_t ext;
    const char *json = "{"
        "\"manifest_version\": 3,"
        "\"name\": \"Test Extension\","
        "\"description\": \"A test extension\","
        "\"version\": \"1.0.0\","
        "\"permissions\": [\"storage\", \"tabs\"]"
    "}";
    ASSERT(eb_ext_parse_manifest(&ext, json, strlen(json)) == true);
    ASSERT(strcmp(ext.name, "Test Extension") == 0);
    ASSERT(ext.manifest_version == 3);
    ASSERT((ext.permissions & EB_PERM_STORAGE) != 0);
    ASSERT((ext.permissions & EB_PERM_TABS) != 0);
    ASSERT((ext.permissions & EB_PERM_COOKIES) == 0);
    ASSERT(ext.installed == true);
    ASSERT(ext.enabled == true);
}

TEST(test_parse_manifest_id_gen) {
    static eb_extension_t ext;
    const char *json = "{\"manifest_version\": 3, \"name\": \"My Ad Blocker\", \"version\": \"2.0\"}";
    eb_ext_parse_manifest(&ext, json, strlen(json));
    /* ID should be generated from name: lowercase, spaces -> underscores */
    ASSERT(ext.id[0] != '\0');
    ASSERT(strstr(ext.id, "my") != NULL);
}

TEST(test_parse_manifest_null) {
    ASSERT(eb_ext_parse_manifest(NULL, "{}", 2) == false);
    static eb_extension_t ext;
    ASSERT(eb_ext_parse_manifest(&ext, NULL, 0) == false);
}

TEST(test_match_pattern_all_urls) {
    ASSERT(eb_ext_match_pattern("<all_urls>", "https://example.com/page") == true);
    ASSERT(eb_ext_match_pattern("<all_urls>", "http://test.org") == true);
}

TEST(test_match_pattern_scheme_host) {
    ASSERT(eb_ext_match_pattern("https://*.example.com/*", "https://sub.example.com/page") == true);
    ASSERT(eb_ext_match_pattern("https://*.example.com/*", "https://example.com/page") == true);
    ASSERT(eb_ext_match_pattern("https://*.example.com/*", "http://example.com/page") == false);
    ASSERT(eb_ext_match_pattern("*://*.example.com/*", "http://sub.example.com/") == true);
}

TEST(test_match_pattern_exact_host) {
    ASSERT(eb_ext_match_pattern("https://exact.com/*", "https://exact.com/any") == true);
    ASSERT(eb_ext_match_pattern("https://exact.com/*", "https://other.com/any") == false);
}

TEST(test_match_pattern_null) {
    ASSERT(eb_ext_match_pattern(NULL, "https://x.com") == false);
    ASSERT(eb_ext_match_pattern("*://*/*", NULL) == false);
}

TEST(test_storage_api) {
    static eb_extension_t ext;
    memset(&ext, 0, sizeof(ext));
    ext.permissions = EB_PERM_STORAGE;
    ext.enabled = true;

    ASSERT(eb_ext_storage_set(&ext, "theme", "dark") == true);
    ASSERT(ext.storage_count == 1);

    char val[256];
    ASSERT(eb_ext_storage_get(&ext, "theme", val, sizeof(val)) == true);
    ASSERT(strcmp(val, "dark") == 0);

    /* Update existing key */
    ASSERT(eb_ext_storage_set(&ext, "theme", "light") == true);
    ASSERT(ext.storage_count == 1); /* Not duplicated */
    eb_ext_storage_get(&ext, "theme", val, sizeof(val));
    ASSERT(strcmp(val, "light") == 0);

    /* Get nonexistent */
    ASSERT(eb_ext_storage_get(&ext, "nokey", val, sizeof(val)) == false);

    /* Remove */
    ASSERT(eb_ext_storage_remove(&ext, "theme") == true);
    ASSERT(ext.storage_count == 0);
    ASSERT(eb_ext_storage_remove(&ext, "theme") == false); /* Already removed */
}

TEST(test_storage_no_permission) {
    static eb_extension_t ext;
    memset(&ext, 0, sizeof(ext));
    ext.permissions = 0; /* No storage permission */
    ASSERT(eb_ext_storage_set(&ext, "key", "val") == false);
}

TEST(test_storage_clear) {
    static eb_extension_t ext;
    memset(&ext, 0, sizeof(ext));
    ext.permissions = EB_PERM_STORAGE;
    eb_ext_storage_set(&ext, "a", "1");
    eb_ext_storage_set(&ext, "b", "2");
    ASSERT(ext.storage_count == 2);
    eb_ext_storage_clear(&ext);
    ASSERT(ext.storage_count == 0);
}

TEST(test_alarms) {
    static eb_extension_t ext;
    memset(&ext, 0, sizeof(ext));
    ext.permissions = EB_PERM_ALARMS;
    int idx = eb_ext_create_alarm(&ext, "checkUpdates", 5.0, 60.0);
    ASSERT(idx >= 0);
    ASSERT(ext.alarm_count == 1);
    ASSERT(strcmp(ext.alarms[0].name, "checkUpdates") == 0);
    ASSERT(ext.alarms[0].active == true);
    ASSERT(ext.alarms[0].period_min == 60.0);
    ASSERT(eb_ext_clear_alarm(&ext, "checkUpdates") == true);
    ASSERT(ext.alarms[0].active == false);
    ASSERT(eb_ext_clear_alarm(&ext, "nonexistent") == false);
}

TEST(test_alarms_no_permission) {
    static eb_extension_t ext;
    memset(&ext, 0, sizeof(ext));
    ext.permissions = 0;
    ASSERT(eb_ext_create_alarm(&ext, "test", 1.0, 0) == -1);
}

TEST(test_context_menus) {
    static eb_extension_t ext;
    memset(&ext, 0, sizeof(ext));
    ext.permissions = EB_PERM_CONTEXT_MENUS;
    eb_context_menu_t menu = {0};
    strcpy(menu.id, "copy-link");
    strcpy(menu.title, "Copy Link");
    menu.enabled = true;
    ASSERT(eb_ext_add_menu(&ext, &menu) == true);
    ASSERT(ext.menu_count == 1);
    ASSERT(strcmp(ext.menus[0].title, "Copy Link") == 0);
    ASSERT(eb_ext_remove_menu(&ext, "copy-link") == true);
    ASSERT(ext.menu_count == 0);
    ASSERT(eb_ext_remove_menu(&ext, "nonexistent") == false);
}

TEST(test_messaging) {
    static eb_ext_manager_t mgr;
    eb_ext_mgr_init(&mgr, "/tmp");
    /* Create two extensions manually */
    eb_extension_t *a = &mgr.extensions[0];
    eb_extension_t *b = &mgr.extensions[1];
    memset(a, 0, sizeof(*a)); memset(b, 0, sizeof(*b));
    strcpy(a->id, "ext_a"); a->enabled = true; a->installed = true;
    strcpy(b->id, "ext_b"); b->enabled = true; b->installed = true;
    mgr.count = 2;

    int port = eb_ext_connect(&mgr, "ext_a", "ext_b", "channel1");
    ASSERT(port > 0);
    ASSERT(b->port_count == 1);
    ASSERT(b->ports[0].connected == true);

    ASSERT(eb_ext_post_message(&mgr, port, "hello") == true);
    eb_ext_disconnect(&mgr, port);
    ASSERT(b->ports[0].connected == false);
    eb_ext_mgr_destroy(&mgr);
}

TEST(test_find_extension) {
    static eb_ext_manager_t mgr;
    eb_ext_mgr_init(&mgr, "/tmp");
    strcpy(mgr.extensions[0].id, "ext1");
    mgr.extensions[0].enabled = true;
    mgr.count = 1;
    ASSERT(eb_ext_find(&mgr, "ext1") != NULL);
    ASSERT(eb_ext_find(&mgr, "ext2") == NULL);
    ASSERT(eb_ext_find(NULL, "ext1") == NULL);
    ASSERT(eb_ext_find(&mgr, NULL) == NULL);
    eb_ext_mgr_destroy(&mgr);
}

int main(void) {
    printf("=== Extension Tests ===\n");
    RUN(test_mgr_init); RUN(test_parse_permission);
    RUN(test_parse_manifest_basic); RUN(test_parse_manifest_id_gen);
    RUN(test_parse_manifest_null);
    RUN(test_match_pattern_all_urls); RUN(test_match_pattern_scheme_host);
    RUN(test_match_pattern_exact_host); RUN(test_match_pattern_null);
    RUN(test_storage_api); RUN(test_storage_no_permission);
    RUN(test_storage_clear);
    RUN(test_alarms); RUN(test_alarms_no_permission);
    RUN(test_context_menus); RUN(test_messaging); RUN(test_find_extension);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
