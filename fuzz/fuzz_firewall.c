// SPDX-License-Identifier: MIT
// libFuzzer harness for eBrowser firewall and tracker blocker
#include "eBrowser/firewall.h"
#include "eBrowser/tracker_blocker.h"
#include "eBrowser/compat.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 4096) return 0;

    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    // Fuzz firewall domain matching
    static eb_firewall_t fw;
    static int fw_init = 0;
    if (!fw_init) {
        eb_fw_init(&fw);
        fw.force_https = false;
        fw.block_nonstandard_ports = false;
        eb_fw_block_domain(&fw, "*.evil.com");
        eb_fw_block_domain(&fw, "malware.net");
        fw_init = 1;
    }
    eb_fw_check(&fw, input, input, 443, EB_FW_PROTO_HTTPS);
    eb_fw_is_blocked(&fw, input);
    eb_fw_wildcard_match("*.test.com", input);
    eb_fw_wildcard_match(input, "sub.test.com");

    // Fuzz tracker blocker filter parsing and matching
    static eb_tracker_blocker_t tb;
    static int tb_init = 0;
    if (!tb_init) {
        eb_tb_init(&tb);
        eb_tb_parse_filter_line(&tb, "||ads.example.com^");
        tb_init = 1;
    }
    eb_tb_parse_filter_line(&tb, input);
    eb_tb_should_block(&tb, input, "example.com", EB_TB_RES_SCRIPT);
    eb_tb_should_block(&tb, "https://example.com/page", input, EB_TB_RES_IMAGE);

    free(input);
    return 0;
}
