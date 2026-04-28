// SPDX-License-Identifier: MIT
#include "eBrowser/sandbox.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

TEST(test_init) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    ASSERT(sb.profile == EB_SANDBOX_CUSTOM);
    ASSERT(sb.capabilities == EB_CAP_NONE);
    ASSERT(sb.aslr_enabled == true);
    ASSERT(sb.rule_count == 0);
    ASSERT(sb.path_count == 0);
    ASSERT(sb.violations == 0);
    eb_sandbox_destroy(&sb);
}

TEST(test_renderer_profile) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    ASSERT(eb_sandbox_apply_profile(&sb, EB_SANDBOX_RENDERER) == true);
    ASSERT(sb.profile == EB_SANDBOX_RENDERER);
    ASSERT(sb.capabilities == EB_CAP_GPU);
    ASSERT(sb.wxorx_enforced == true);
    ASSERT(sb.ns.isolate_pid == true);
    ASSERT(sb.ns.isolate_net == true);
    ASSERT(sb.ns.isolate_mount == true);
    eb_sandbox_destroy(&sb);
}

TEST(test_network_profile) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    eb_sandbox_apply_profile(&sb, EB_SANDBOX_NETWORK);
    ASSERT((sb.capabilities & EB_CAP_NETWORK) != 0);
    ASSERT((sb.capabilities & EB_CAP_FILE_READ) != 0);
    ASSERT((sb.capabilities & EB_CAP_FILE_WRITE) == 0);
    ASSERT(sb.ns.isolate_pid == true);
    eb_sandbox_destroy(&sb);
}

TEST(test_strict_profile) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    eb_sandbox_apply_profile(&sb, EB_SANDBOX_STRICT);
    ASSERT(sb.capabilities == EB_CAP_NONE);
    ASSERT(sb.wxorx_enforced == true);
    ASSERT(sb.ns.isolate_pid == true);
    ASSERT(sb.ns.isolate_net == true);
    ASSERT(sb.ns.isolate_mount == true);
    ASSERT(sb.ns.isolate_user == true);
    ASSERT(sb.ns.isolate_ipc == true);
    eb_sandbox_destroy(&sb);
}

TEST(test_extension_profile) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    eb_sandbox_apply_profile(&sb, EB_SANDBOX_EXTENSION);
    ASSERT((sb.capabilities & EB_CAP_NETWORK) != 0);
    ASSERT((sb.capabilities & EB_CAP_FILE_READ) != 0);
    ASSERT((sb.capabilities & EB_CAP_CLIPBOARD) != 0);
    ASSERT((sb.capabilities & EB_CAP_FILE_WRITE) == 0);
    ASSERT(sb.wxorx_enforced == true);
    eb_sandbox_destroy(&sb);
}

TEST(test_set_capabilities) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    ASSERT(eb_sandbox_set_caps(&sb, EB_CAP_NETWORK | EB_CAP_GPU) == true);
    ASSERT(sb.capabilities == (EB_CAP_NETWORK | EB_CAP_GPU));
    ASSERT(eb_sandbox_set_caps(NULL, 0) == false);
    eb_sandbox_destroy(&sb);
}

TEST(test_add_paths) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    ASSERT(eb_sandbox_add_path(&sb, "/usr/lib") == true);
    ASSERT(eb_sandbox_add_path(&sb, "/tmp") == true);
    ASSERT(sb.path_count == 2);
    ASSERT(strcmp(sb.allowed_paths[0], "/usr/lib") == 0);
    ASSERT(strcmp(sb.allowed_paths[1], "/tmp") == 0);
    ASSERT(eb_sandbox_add_path(NULL, "/x") == false);
    ASSERT(eb_sandbox_add_path(&sb, NULL) == false);
    eb_sandbox_destroy(&sb);
}

TEST(test_add_seccomp_rules) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    ASSERT(eb_sandbox_add_seccomp(&sb, 1, true) == true);  /* write allowed */
    ASSERT(eb_sandbox_add_seccomp(&sb, 59, false) == true); /* execve blocked */
    ASSERT(sb.rule_count == 2);
    ASSERT(sb.rules[0].syscall_nr == 1);
    ASSERT(sb.rules[0].allow == true);
    ASSERT(sb.rules[1].syscall_nr == 59);
    ASSERT(sb.rules[1].allow == false);
    eb_sandbox_destroy(&sb);
}

TEST(test_seccomp_limit) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    for (int i = 0; i < EB_SANDBOX_MAX_SYSCALLS; i++)
        ASSERT(eb_sandbox_add_seccomp(&sb, i, true) == true);
    ASSERT(eb_sandbox_add_seccomp(&sb, 999, true) == false); /* Full */
    eb_sandbox_destroy(&sb);
}

TEST(test_path_limit) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    for (int i = 0; i < EB_SANDBOX_MAX_PATHS; i++) {
        char p[64]; snprintf(p, sizeof(p), "/path/%d", i);
        ASSERT(eb_sandbox_add_path(&sb, p) == true);
    }
    ASSERT(eb_sandbox_add_path(&sb, "/overflow") == false); /* Full */
    eb_sandbox_destroy(&sb);
}

TEST(test_guarded_alloc) {
    void *p = eb_sandbox_alloc_guarded(256);
    ASSERT(p != NULL);
    /* Should be able to read/write within bounds */
    memset(p, 0xAB, 256);
    ASSERT(((uint8_t *)p)[0] == 0xAB);
    ASSERT(((uint8_t *)p)[255] == 0xAB);
    eb_sandbox_free_guarded(p, 256);
}

TEST(test_destroy_clears) {
    eb_sandbox_t sb;
    eb_sandbox_init(&sb);
    eb_sandbox_apply_profile(&sb, EB_SANDBOX_STRICT);
    eb_sandbox_add_path(&sb, "/test");
    eb_sandbox_destroy(&sb);
    ASSERT(sb.capabilities == 0);
    ASSERT(sb.rule_count == 0);
    ASSERT(sb.path_count == 0);
}

TEST(test_null_safety) {
    eb_sandbox_init(NULL);
    ASSERT(eb_sandbox_apply_profile(NULL, EB_SANDBOX_STRICT) == false);
    ASSERT(eb_sandbox_set_caps(NULL, 0) == false);
    ASSERT(eb_sandbox_add_path(NULL, "/x") == false);
    ASSERT(eb_sandbox_add_seccomp(NULL, 0, true) == false);
    eb_sandbox_destroy(NULL);
}

int main(void) {
    printf("=== Sandbox Tests ===\n");
    RUN(test_init); RUN(test_renderer_profile); RUN(test_network_profile);
    RUN(test_strict_profile); RUN(test_extension_profile);
    RUN(test_set_capabilities); RUN(test_add_paths);
    RUN(test_add_seccomp_rules); RUN(test_seccomp_limit);
    RUN(test_path_limit); RUN(test_guarded_alloc);
    RUN(test_destroy_clears); RUN(test_null_safety);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
