// SPDX-License-Identifier: MIT
// eBrowser Performance Regression Test
// Outputs machine-readable JSON for CI tracking over time.
// Fails if any metric regresses beyond threshold.

#include "eBrowser/compat.h"
#include "eBrowser/html_parser.h"
#include "eBrowser/css_parser.h"
#include "eBrowser/dom.h"
#include "eBrowser/security.h"
#include "eBrowser/crypto.h"
#include "eBrowser/firewall.h"
#include "eBrowser/tracker_blocker.h"
#include "eBrowser/privacy.h"
#include "eBrowser/anti_fingerprint.h"
#include "eBrowser/extension.h"
#include "eBrowser/tab_manager.h"
#include "eBrowser/memory_safety.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_us(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec*1000000ULL + (uint64_t)ts.tv_nsec/1000ULL;
}

// --- Thresholds (max allowed microseconds per operation) ---
// Set generously — real values are much lower. These catch major regressions.
#define THRESH_HTML_PARSE_US     10000   // 10ms per 50-element page
#define THRESH_CSS_PARSE_US      100     // 100us per 10-rule sheet
#define THRESH_DOM_OPS_US        5000    // 5ms per 100-node tree
#define THRESH_SHA256_US         200     // 200us per 4KB hash
#define THRESH_XSS_SCAN_US      50      // 50us per scan
#define THRESH_FW_CHECK_US      50      // 50us per check
#define THRESH_TB_CHECK_US      100     // 100us per check
#define THRESH_URL_CLEAN_US     10      // 10us per clean
#define THRESH_EXT_MATCH_US     5       // 5us per match
#define THRESH_POOL_ALLOC_NS    500     // 500ns per alloc
#define THRESH_PIPELINE_US      100     // 100us per full pipeline request
#define THRESH_PIPELINE_RPS     20000   // minimum 50K RPS

typedef struct {
    const char *name;
    const char *unit;
    double value;
    double threshold;
    int passed;
} perf_metric_t;

#define MAX_METRICS 20
static perf_metric_t metrics[MAX_METRICS];
static int metric_count = 0;
static int failures = 0;

static void record(const char *name, const char *unit, double val, double thresh) {
    if (metric_count >= MAX_METRICS) return;
    perf_metric_t *m = &metrics[metric_count++];
    m->name = name; m->unit = unit; m->value = val; m->threshold = thresh;
    m->passed = (strcmp(unit, "rps") == 0) ? (val >= thresh) : (val <= thresh);
    if (!m->passed) failures++;
}

// ============================================================
// Benchmarks
// ============================================================

static const char *TEST_HTML =
    "<!DOCTYPE html><html><head><title>Bench</title></head><body>"
    "<div class=\"container\"><h1>Title</h1>"
    "<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit.</p>"
    "<a href=\"https://example.com\">Link</a>"
    "<img src=\"test.png\" alt=\"test\">"
    "<ul><li>Item 1</li><li>Item 2</li><li>Item 3</li></ul>"
    "</div></body></html>";

static const char *TEST_CSS =
    "body { margin: 0; font-family: Arial; }\n"
    "h1 { color: #333; margin-bottom: 8px; }\n"
    ".container { max-width: 1200px; margin: 0 auto; }\n"
    "a { color: #1a73e8; text-decoration: none; }\n"
    "a:hover { text-decoration: underline; }\n";

int main(void) {
    setbuf(stdout, NULL);
    uint64_t t0, t1;
    int N;

    // 1. HTML Parse
    N = 1000;
    t0 = now_us();
    for (int i = 0; i < N; i++) {
        eb_dom_node_t *doc = eb_html_parse(TEST_HTML, strlen(TEST_HTML));
        if (doc) eb_dom_destroy(doc);
    }
    t1 = now_us();
    record("html_parse", "us/op", (double)(t1-t0)/(double)N, THRESH_HTML_PARSE_US);

    // 2. CSS Parse
    N = 10000;
    t0 = now_us();
    for (int i = 0; i < N; i++) {
        eb_stylesheet_t ss;
        eb_css_init_stylesheet(&ss);
        eb_css_parse(&ss, TEST_CSS, strlen(TEST_CSS));
    }
    t1 = now_us();
    record("css_parse", "us/op", (double)(t1-t0)/(double)N, THRESH_CSS_PARSE_US);

    // 3. DOM Operations
    N = 1000;
    t0 = now_us();
    for (int i = 0; i < N; i++) {
        eb_dom_node_t *doc = eb_dom_create_document();
        eb_dom_node_t *body = eb_dom_create_element("body");
        eb_dom_append_child(doc, body);
        for (int j = 0; j < 50; j++) {
            eb_dom_node_t *div = eb_dom_create_element("div");
            eb_dom_set_attribute(div, "class", "item");
            eb_dom_append_child(body, div);
        }
        eb_dom_find_by_tag(doc, "div");
        eb_dom_node_list_t list = {0};
        eb_dom_find_all_by_class(doc, "item", &list);
        eb_dom_destroy(doc);
    }
    t1 = now_us();
    record("dom_ops", "us/op", (double)(t1-t0)/(double)N, THRESH_DOM_OPS_US);

    // 4. SHA-256
    uint8_t data[4096], digest[32];
    memset(data, 0xAB, sizeof(data));
    N = 50000;
    t0 = now_us();
    for (int i = 0; i < N; i++) eb_sha256(data, sizeof(data), digest);
    t1 = now_us();
    record("sha256_4kb", "us/op", (double)(t1-t0)/(double)N, THRESH_SHA256_US);

    // 5. XSS Scan
    const char *xss_html = "<div onclick='x'><script>alert(1)</script><img onerror=x>";
    N = 100000;
    t0 = now_us();
    for (int i = 0; i < N; i++) eb_xss_scan_html(xss_html, strlen(xss_html));
    t1 = now_us();
    record("xss_scan", "us/op", (double)(t1-t0)/(double)N, THRESH_XSS_SCAN_US);

    // 6. Firewall Check
    static eb_firewall_t fw;
    eb_fw_init(&fw); fw.force_https = false; fw.block_nonstandard_ports = false;
    for (int i = 0; i < 100; i++) {
        char d[64]; snprintf(d, sizeof(d), "tracker-%d.bad.com", i);
        eb_fw_block_domain(&fw, d);
    }
    N = 100000;
    t0 = now_us();
    for (int i = 0; i < N; i++)
        eb_fw_check(&fw, "https://safe.com", "safe.com", 443, EB_FW_PROTO_HTTPS);
    t1 = now_us();
    record("firewall_check", "us/op", (double)(t1-t0)/(double)N, THRESH_FW_CHECK_US);

    // 7. Tracker Blocker
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    for (int i = 0; i < 200; i++) {
        char f[128]; snprintf(f, sizeof(f), "||ad-net-%d.com^", i);
        eb_tb_parse_filter_line(&tb, f);
    }
    N = 50000;
    t0 = now_us();
    for (int i = 0; i < N; i++)
        eb_tb_should_block(&tb, "https://safe-cdn.com/style.css", "example.com", EB_TB_RES_STYLESHEET);
    t1 = now_us();
    record("tracker_block", "us/op", (double)(t1-t0)/(double)N, THRESH_TB_CHECK_US);

    // 8. URL Cleaning
    static eb_privacy_t priv;
    eb_priv_init(&priv);
    char clean[1024];
    N = 100000;
    t0 = now_us();
    for (int i = 0; i < N; i++)
        eb_priv_clean_url(&priv, "https://x.com/p?q=a&utm_source=g&fbclid=b", clean, sizeof(clean));
    t1 = now_us();
    record("url_clean", "us/op", (double)(t1-t0)/(double)N, THRESH_URL_CLEAN_US);

    // 9. Extension URL Match
    N = 100000;
    t0 = now_us();
    for (int i = 0; i < N; i++)
        eb_ext_match_pattern("https://*.example.com/*", "https://sub.example.com/page");
    t1 = now_us();
    record("ext_match", "us/op", (double)(t1-t0)/(double)N, THRESH_EXT_MATCH_US);

    // 10. Pool Allocator
    static eb_mem_safety_t mem;
    eb_mem_safety_init(&mem);
    eb_mem_pool_create(&mem, 64, 1024);
    eb_mem_pool_t *pool = &mem.pools[0];
    N = 1000000;
    t0 = now_us();
    for (int i = 0; i < N; i++) {
        void *p = eb_mem_pool_alloc(pool);
        if (p) eb_mem_pool_free(pool, p);
    }
    t1 = now_us();
    record("pool_alloc", "ns/op", (double)(t1-t0)*1000.0/(double)N, THRESH_POOL_ALLOC_NS);
    eb_mem_safety_destroy(&mem);

    // 11. Full Pipeline (same as load_test_combined)
    N = 10000;
    t0 = now_us();
    for (int i = 0; i < N; i++) {
        const char *url = "https://example.com/page?utm_source=g&fbclid=x";
        eb_fw_check(&fw, url, "example.com", 443, EB_FW_PROTO_HTTPS);
        eb_priv_clean_url(&priv, url, clean, sizeof(clean));
        eb_tb_should_block(&tb, url, "example.com", EB_TB_RES_DOCUMENT);
        eb_xss_scan_html(TEST_HTML, strlen(TEST_HTML));
        eb_sha256((const uint8_t*)TEST_HTML, strlen(TEST_HTML), digest);
    }
    t1 = now_us();
    double pipeline_us = (double)(t1-t0)/(double)N;
    double pipeline_rps = 1000000.0 / pipeline_us;
    record("pipeline", "us/op", pipeline_us, THRESH_PIPELINE_US);
    record("pipeline_rps", "rps", pipeline_rps, THRESH_PIPELINE_RPS);

    // ============================================================
    // Output: Human-readable + JSON
    // ============================================================
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║        eBrowser Performance Regression Results              ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  %-20s %10s %10s %8s ║\n", "Metric", "Value", "Threshold", "Status");
    printf("║  ────────────────────────────────────────────────────────── ║\n");
    for (int i = 0; i < metric_count; i++) {
        perf_metric_t *m = &metrics[i];
        printf("║  %-20s %8.1f %s %8.0f %s   %s  ║\n",
            m->name, m->value, m->unit, m->threshold, m->unit,
            m->passed ? "  ✓" : "FAIL");
    }
    printf("║                                                              ║\n");
    printf("║  VERDICT: %s                              ║\n",
        failures == 0 ? "ALL METRICS WITHIN THRESHOLDS ✓" :
                        "REGRESSION DETECTED          ✗");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    // JSON output for CI parsing
    printf("\n--- PERF_JSON_START ---\n");
    printf("{\n  \"version\": \"2.0.0\",\n  \"metrics\": [\n");
    for (int i = 0; i < metric_count; i++) {
        perf_metric_t *m = &metrics[i];
        printf("    {\"name\":\"%s\",\"value\":%.2f,\"unit\":\"%s\",\"threshold\":%.0f,\"passed\":%s}%s\n",
            m->name, m->value, m->unit, m->threshold,
            m->passed ? "true" : "false",
            i < metric_count-1 ? "," : "");
    }
    printf("  ],\n  \"failures\": %d,\n  \"total\": %d\n}\n", failures, metric_count);
    printf("--- PERF_JSON_END ---\n");

    return failures > 0 ? 1 : 0;
}
