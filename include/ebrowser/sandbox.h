// SPDX-License-Identifier: MIT
#ifndef eBrowser_SANDBOX_H
#define eBrowser_SANDBOX_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_SANDBOX_MAX_SYSCALLS   64
#define EB_SANDBOX_MAX_PATHS      32
#define EB_SANDBOX_MAX_PATH_LEN   512

typedef enum {
    EB_CAP_NONE=0, EB_CAP_NETWORK=(1<<0), EB_CAP_FILE_READ=(1<<1),
    EB_CAP_FILE_WRITE=(1<<2), EB_CAP_EXEC=(1<<3), EB_CAP_IPC=(1<<4),
    EB_CAP_GPU=(1<<5), EB_CAP_AUDIO=(1<<6), EB_CAP_CAMERA=(1<<7),
    EB_CAP_CLIPBOARD=(1<<8), EB_CAP_GEOLOCATION=(1<<9), EB_CAP_ALL=0x7FFFFFFF
} eb_sandbox_cap_t;

typedef enum {
    EB_SANDBOX_RENDERER, EB_SANDBOX_NETWORK, EB_SANDBOX_EXTENSION,
    EB_SANDBOX_GPU_PROC, EB_SANDBOX_PLUGIN, EB_SANDBOX_STRICT, EB_SANDBOX_CUSTOM
} eb_sandbox_profile_t;

typedef struct { int syscall_nr; bool allow; uint32_t arg_mask[6]; } eb_seccomp_rule_t;

typedef struct {
    bool isolate_pid, isolate_net, isolate_mount, isolate_user, isolate_ipc;
    char rootfs[EB_SANDBOX_MAX_PATH_LEN];
} eb_namespace_config_t;

typedef struct {
    eb_sandbox_profile_t  profile;
    uint32_t              capabilities;
    eb_seccomp_rule_t     rules[EB_SANDBOX_MAX_SYSCALLS];
    int                   rule_count;
    char                  allowed_paths[EB_SANDBOX_MAX_PATHS][EB_SANDBOX_MAX_PATH_LEN];
    int                   path_count;
    eb_namespace_config_t ns;
    bool wxorx_enforced, aslr_enabled, seccomp_active, ns_active, privs_dropped;
    int  violations;
} eb_sandbox_t;

void  eb_sandbox_init(eb_sandbox_t *sb);
bool  eb_sandbox_apply_profile(eb_sandbox_t *sb, eb_sandbox_profile_t profile);
bool  eb_sandbox_set_caps(eb_sandbox_t *sb, uint32_t caps);
bool  eb_sandbox_add_path(eb_sandbox_t *sb, const char *path);
bool  eb_sandbox_activate(eb_sandbox_t *sb);
void  eb_sandbox_destroy(eb_sandbox_t *sb);
bool  eb_sandbox_add_seccomp(eb_sandbox_t *sb, int syscall_nr, bool allow);
bool  eb_sandbox_install_seccomp(eb_sandbox_t *sb);
bool  eb_sandbox_create_ns(eb_sandbox_t *sb);
bool  eb_sandbox_drop_privs(eb_sandbox_t *sb);
bool  eb_sandbox_enforce_wxorx(eb_sandbox_t *sb);
void *eb_sandbox_alloc_guarded(size_t size);
void  eb_sandbox_free_guarded(void *ptr, size_t size);
typedef void (*eb_sandbox_violation_cb)(const char *desc, void *ud);
void  eb_sandbox_set_violation_cb(eb_sandbox_t *sb, eb_sandbox_violation_cb cb, void *ud);
#endif
