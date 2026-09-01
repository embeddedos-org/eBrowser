<!-- generated: eos-ai-scaffold -->
# Tasks

Working ledger for `eBrowser`. The planner writes entries; each owning role
updates its own row. Roles are in [AGENTS.md](./AGENTS.md), the workflow in
[ORCHESTRATION.md](./ORCHESTRATION.md), the gate in [VERIFY.md](./VERIFY.md).

Status is one of: `todo`, `in-progress`, `blocked`, `review`, `done`.

## Active

| ID | Task | Owner | Mode | Status | Depends on |
|----|------|-------|------|--------|------------|
| —  | No active tasks. | — | — | — | — |

## Completed

| ID | Task | Owner | Verified by | Evidence |
|----|------|-------|-------------|----------|
| T-001 | Fix a 2,888-byte stack overflow in every consumer of `eBrowser_security` | security | reviewer | `EB_USE_MBEDTLS` was defined `PRIVATE` in `src/security/CMakeLists.txt`, but it adds seven members to `eb_tls_ctx_t` in the **public** header `include/eBrowser/tls.h`. Measured: the library compiled `sizeof(eb_tls_ctx_t) == 10832`, every consumer saw `7944`, so `eb_tls_init()` wrote 2,888 bytes past the end of a caller-allocated stack object. `test_tls` died with `*** stack smashing detected ***`. Changed the define and the mbedtls link to `PUBLIC`; both sides now measure 10832 and the test passes. |
| T-002 | Repair `tests/test_tls.c`, which did not compile | testing | reviewer | `test_config_default()` was truncated mid-body and a duplicate copy of `test_tls_free()` was spliced inside it, so every following function nested illegally — 20+ `invalid storage class for function` errors plus a redefinition. This one file failed the whole parallel build, which is why `test_url`, `test_tls` and `test_modules` were reported `***Not Run`. Restored the assertions (default config must require TLS 1.2+ and verify both peer and hostname) and removed the duplicate. 20/20 tests pass. |

---

## Task template

```markdown
### T-000 — <short title>

Owner: <role>
Mode: <see MODES.md>
Status: todo
Depends on: <task ids, or none>

Goal
: <one sentence: what is true afterwards that is not true now>

Acceptance criteria
: - <observable, checkable statement>
  - <observable, checkable statement>

Files in scope
: <paths the owner is expected to touch>

Out of scope
: <what this task deliberately does not change>

Risks
: <what could break, and what would reveal it>

Verification
: | Check | Command | Result |
  |-------|---------|--------|
  | <name> | `<command>` | `NOT RUN` |
```

## Verification commands for this repository

These commands were derived from the manifests at the repository root. Confirm one works before relying on it; a listed script may still be a stub.

| Check | Command | Default state |
|-------|---------|---------------|
| Build | `cmake --build build -j` | `NOT RUN` |
| Unit tests | `ctest --test-dir build --output-on-failure` | `NOT RUN` |

## Rules

- One task per unit of work that can be verified on its own.
- Acceptance criteria are written before work starts and are not edited to match
  what was built. If they were wrong, say so and rewrite them explicitly.
- A task reaches `done` only when the definition of done in
  [ORCHESTRATION.md](./ORCHESTRATION.md) is met and the verification commands
  were actually run.
- `blocked` requires a note naming what it is blocked on and who can unblock it.
