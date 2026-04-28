// SPDX-License-Identifier: MIT
#include "eBrowser/firewall.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

TEST(test_init) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    ASSERT(fw.enabled == true);
    ASSERT(fw.force_https == true);
    ASSERT(fw.block_mixed == true);
    ASSERT(fw.block_nonstandard_ports == true);
    ASSERT(fw.rule_count == 0);
    ASSERT(fw.bl_count == 0);
    eb_fw_destroy(&fw);
}

TEST(test_block_domain) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    ASSERT(eb_fw_block_domain(&fw, "evil.com") == true);
    ASSERT(fw.bl_count == 1);
    ASSERT(eb_fw_is_blocked(&fw, "evil.com") == true);
    ASSERT(eb_fw_is_blocked(&fw, "good.com") == false);
    eb_fw_destroy(&fw);
}

TEST(test_block_duplicate) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    ASSERT(eb_fw_block_domain(&fw, "dup.com") == true);
    ASSERT(eb_fw_block_domain(&fw, "dup.com") == true); /* No error, idempotent */
    ASSERT(fw.bl_count == 1); /* Not added twice */
    eb_fw_destroy(&fw);
}

TEST(test_unblock_domain) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    eb_fw_block_domain(&fw, "temp.com");
    ASSERT(eb_fw_is_blocked(&fw, "temp.com") == true);
    ASSERT(eb_fw_unblock_domain(&fw, "temp.com") == true);
    ASSERT(eb_fw_is_blocked(&fw, "temp.com") == false);
    ASSERT(fw.bl_count == 0);
    eb_fw_destroy(&fw);
}

TEST(test_unblock_nonexistent) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    ASSERT(eb_fw_unblock_domain(&fw, "nope.com") == false);
    eb_fw_destroy(&fw);
}

TEST(test_wildcard_blocklist) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    eb_fw_block_domain(&fw, "*.tracker.com");
    ASSERT(eb_fw_is_blocked(&fw, "ads.tracker.com") == true);
    ASSERT(eb_fw_is_blocked(&fw, "sub.deep.tracker.com") == true);
    ASSERT(eb_fw_is_blocked(&fw, "other.com") == false);
    eb_fw_destroy(&fw);
}

TEST(test_wildcard_match_fn) {
    ASSERT(eb_fw_wildcard_match("*", "anything.com") == true);
    ASSERT(eb_fw_wildcard_match("*.example.com", "sub.example.com") == true);
    ASSERT(eb_fw_wildcard_match("exact.com", "exact.com") == true);
    ASSERT(eb_fw_wildcard_match("exact.com", "other.com") == false);
    ASSERT(eb_fw_wildcard_match(NULL, "test.com") == false);
    ASSERT(eb_fw_wildcard_match("test.com", NULL) == false);
}

TEST(test_cidr_match) {
    /* 192.168.1.100 = 0xC0A80164 */
    uint32_t ip = (192u << 24) | (168u << 16) | (1u << 8) | 100u;
    ASSERT(eb_fw_cidr_match("192.168.0.0/16", ip) == true);
    ASSERT(eb_fw_cidr_match("192.168.1.0/24", ip) == true);
    ASSERT(eb_fw_cidr_match("10.0.0.0/8", ip) == false);
    ASSERT(eb_fw_cidr_match("192.168.1.100/32", ip) == true);
    ASSERT(eb_fw_cidr_match(NULL, ip) == false);
}

TEST(test_force_https) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    eb_fw_action_t r = eb_fw_check(&fw, "http://example.com", "example.com", 80, EB_FW_PROTO_HTTP);
    ASSERT(r == EB_FW_UPGRADE_HTTPS);
    r = eb_fw_check(&fw, "https://example.com", "example.com", 443, EB_FW_PROTO_HTTPS);
    ASSERT(r == EB_FW_ALLOW);
    eb_fw_destroy(&fw);
}

TEST(test_block_nonstandard_port) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    eb_fw_action_t r = eb_fw_check(&fw, "https://example.com:9999", "example.com", 9999, EB_FW_PROTO_HTTPS);
    ASSERT(r == EB_FW_BLOCK);
    r = eb_fw_check(&fw, "https://example.com:443", "example.com", 443, EB_FW_PROTO_HTTPS);
    ASSERT(r == EB_FW_ALLOW);
    r = eb_fw_check(&fw, "https://example.com:8080", "example.com", 8080, EB_FW_PROTO_HTTPS);
    ASSERT(r == EB_FW_ALLOW); /* 8080 is standard */
    eb_fw_destroy(&fw);
}

TEST(test_blocked_domain_check) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    fw.force_https = false; /* Disable for this test */
    eb_fw_block_domain(&fw, "malware.com");
    eb_fw_action_t r = eb_fw_check(&fw, "https://malware.com/path", "malware.com", 443, EB_FW_PROTO_HTTPS);
    ASSERT(r == EB_FW_BLOCK);
    eb_fw_destroy(&fw);
}

TEST(test_custom_rule) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    fw.force_https = false;
    eb_fw_rule_t rule = {0};
    rule.action = EB_FW_BLOCK;
    rule.match_type = EB_FW_EXACT;
    rule.protocol = EB_FW_PROTO_ANY;
    strncpy(rule.pattern, "blocked.org", sizeof(rule.pattern)-1);
    ASSERT(eb_fw_add_rule(&fw, &rule) == true);
    ASSERT(fw.rule_count == 1);
    eb_fw_action_t r = eb_fw_check(&fw, "https://blocked.org", "blocked.org", 443, EB_FW_PROTO_HTTPS);
    ASSERT(r == EB_FW_BLOCK);
    ASSERT(fw.rules[0].hits == 1);
    eb_fw_destroy(&fw);
}

TEST(test_rate_limiting) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    fw.force_https = false;
    fw.block_nonstandard_ports = false;
    ASSERT(eb_fw_add_rate_limit(&fw, "api.example.com", 3, 100) == true);
    /* First 3 should pass */
    ASSERT(eb_fw_check_rate(&fw, "api.example.com") == true);
    ASSERT(eb_fw_check_rate(&fw, "api.example.com") == true);
    ASSERT(eb_fw_check_rate(&fw, "api.example.com") == true);
    /* 4th should be throttled */
    ASSERT(eb_fw_check_rate(&fw, "api.example.com") == false);
    /* Different domain should be fine */
    ASSERT(eb_fw_check_rate(&fw, "other.com") == true);
    eb_fw_destroy(&fw);
}

TEST(test_disabled_firewall) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    fw.enabled = false;
    eb_fw_block_domain(&fw, "evil.com");
    eb_fw_action_t r = eb_fw_check(&fw, "http://evil.com", "evil.com", 80, EB_FW_PROTO_HTTP);
    ASSERT(r == EB_FW_ALLOW); /* Firewall disabled */
    eb_fw_destroy(&fw);
}

TEST(test_stats) {
    static eb_firewall_t fw;
    eb_fw_init(&fw);
    fw.force_https = false;
    eb_fw_block_domain(&fw, "bad.com");
    eb_fw_check(&fw, "https://bad.com", "bad.com", 443, EB_FW_PROTO_HTTPS);
    eb_fw_check(&fw, "https://good.com", "good.com", 443, EB_FW_PROTO_HTTPS);
    eb_fw_stats_t stats;
    eb_fw_get_stats(&fw, &stats);
    ASSERT(stats.blocked == 1);
    ASSERT(stats.allowed == 1);
    ASSERT(stats.total_requests == 2);
    eb_fw_destroy(&fw);
}

TEST(test_null_args) {
    eb_fw_init(NULL);
    ASSERT(eb_fw_block_domain(NULL, "x.com") == false);
    ASSERT(eb_fw_is_blocked(NULL, "x.com") == false);
    ASSERT(eb_fw_check_rate(NULL, "x.com") == true);
}

int main(void) {
    printf("=== Firewall Tests ===\n");
    RUN(test_init); RUN(test_block_domain); RUN(test_block_duplicate);
    RUN(test_unblock_domain); RUN(test_unblock_nonexistent);
    RUN(test_wildcard_blocklist); RUN(test_wildcard_match_fn);
    RUN(test_cidr_match); RUN(test_force_https);
    RUN(test_block_nonstandard_port); RUN(test_blocked_domain_check);
    RUN(test_custom_rule); RUN(test_rate_limiting);
    RUN(test_disabled_firewall); RUN(test_stats); RUN(test_null_args);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
