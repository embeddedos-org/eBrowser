# eBrowser privacy invariants

Developer documentation for the privacy engine (`src/privacy/`, `include/ebrowser/privacy.h`).
These are the guarantees the engine holds — behavior that must not regress.

## Referrer-policy matrix

`eb_priv_apply_referrer()` decides the `Referer` header for a navigation
from `from` to `to`, under the configured `eb_referrer_policy_t`.

| Policy | Same-origin navigation | Cross-origin navigation |
|---|---|---|
| `EB_REF_FULL` | Full `from` URL | Full `from` URL |
| `EB_REF_ORIGIN_ONLY` (default) | `scheme://host/` | `scheme://host/` |
| `EB_REF_SAME_ORIGIN` | Full `from` URL | Empty |
| `EB_REF_NONE` | Empty | Empty |

### Fail-closed construction (the #28 hardening)

The function is built so a bug can only *under*-disclose, never over-disclose:

1. `ref` is zeroed **first**, before the policy switch. Every code path that
   cannot produce a complete result returns with the empty referrer intact.
2. **Never truncates.** If the origin (or full URL) does not fit in `max`,
   the function keeps the empty referrer instead of emitting a partial URL —
   a truncated URL can disclose a malformed partial path, which is worse
   than nothing.
3. Origin extraction requires a `://` separator. Malformed `from` URLs yield
   empty, not a guess.
4. `EB_REF_SAME_ORIGIN` compares scheme + host byte-for-byte (`fl != tl ||
   strncmp(...) != 0` → empty). Subdomains do not match.
5. NULL `priv`/`from`/`to`/`ref` or `max == 0` → return `-1`, `ref` untouched.

### Known quirk (documented, not fixed)

`p->referrers_stripped` is never incremented: the counter update used to sit
inside this function, but `p` is `const`, so the statement had no effect.
Fixing it means changing the function's signature — a public-API decision,
not a bounds fix. The counter stays at zero until that decision is made.

## Mode defaults

| Mode | Cookie policy | Referrer policy | AFP level |
|---|---|---|---|
| Normal (default) | Block third-party | Origin-only | `EB_AFP_STANDARD` |
| Incognito | Delete on close | Origin-only | `EB_AFP_STRICT` |
| Tor | Block all | None | `EB_AFP_TOR_LIKE` |

## Mode → referrer-policy mapping (decided 2026-10-13)

`eb_priv_set_mode()` drives `referrer_policy` with the mapping already
shown in the Mode defaults table — verified against the implementation
on 2026-10-13 (the wiring predates this record; what was missing was
the test coverage, now in `tests/test_privacy_invariants.c`):

| Privacy mode | Referrer policy |
|---|---|
| `EB_PRIV_NORMAL` | `EB_REF_ORIGIN_ONLY` |
| `EB_PRIV_INCOGNITO` | `EB_REF_ORIGIN_ONLY` |
| `EB_PRIV_TOR_MODE` | `EB_REF_NONE` |

Discipline, mirroring the AFP ladder: re-entering a mode re-applies its
policy (no one-way latch), and raising the mode never weakens the
policy — restrictiveness along Normal → Incognito → Tor is
non-decreasing (`EB_REF_FULL` < `EB_REF_ORIGIN_ONLY` <
`EB_REF_SAME_ORIGIN` < `EB_REF_NONE`). Embedders may still set
`referrer_policy` directly between mode changes for a stricter
per-site policy; the next `eb_priv_set_mode()` call re-applies the
mode's decided policy.

## Engine-wide guarantees

- **Tracking parameters** are stripped from URLs (`eb_priv_clean_url`;
  up to 64 params, maintainer-curated list, user-extensible via
  `eb_priv_add_tracking_param`).
- **Bounce tracking** is blocked — redirect chains through known tracker
  domains are not followed for attribution.
- **Third-party cookies** are blocked by default; Tor mode blocks all.
- **DNT (`DNT: 1`) and GPC** headers are injected when enabled.
- **First-party isolation** (`fpi_enabled`) and **storage partitioning**
  keep cross-site state separated.
- **HTTPS-only** mode refuses plaintext navigation.
- **Per-site exceptions** (`eb_priv_add_exception`) are explicit and bounded
  (max 128); there is no silent allowlist.

## Mode → AFP-level mapping (decided 2026-10-11)

The wiring gap noted in the "Anti-fingerprinting ladder" section is now
closed. `eb_anti_fingerprint_t` is embedded in `eb_privacy_t`
(`include/ebrowser/privacy.h`), owned end-to-end by the privacy engine:
`eb_priv_init()` runs `eb_afp_init()`, `eb_priv_destroy()` runs
`eb_afp_destroy()`, and `eb_priv_set_mode()` drives `eb_afp_set_level()`
with the decided mapping:

| Privacy mode | AFP level |
|---|---|
| `EB_PRIV_NORMAL` | `EB_AFP_STANDARD` (matches the `eb_afp_init` default) |
| `EB_PRIV_INCOGNITO` | `EB_AFP_STRICT` |
| `EB_PRIV_TOR_MODE` | `EB_AFP_TOR_LIKE` |

Rationale: raising the privacy mode must never *reduce* fingerprinting
resistance (the monotonicity invariant in `tests/test_privacy_invariants.c`),
so the ladder order STANDARD → STRICT → TOR_LIKE is the only ordering
consistent with the mode order Normal → Incognito → Tor. The init default
stays `EB_AFP_STANDARD`, so a freshly-initialized engine is already at the
Normal-mode level with no special-casing. Embedders that need finer control
can still call `eb_afp_set_level()` directly on `priv.afp` — it is a public
field, not a hidden member.

## What this doc does not cover

Cross-engine interactions outside the privacy engine: how the render engine
applies `priv.afp` to canvas/WebGL callsites, the per-site AFP override UI,
and telemetry policy for the `eb_afp_score_t` risk field.

## Anti-fingerprinting ladder

`src/security/anti_fingerprint/` (`eb_anti_fingerprint_t`,
`include/ebrowser/anti_fingerprint.h`) implements the fingerprinting
defense an earlier revision of this doc listed as a future milestone.
The AFP struct is embedded in `eb_privacy_t` and wired to privacy modes
as documented above ("Mode → AFP-level mapping").

### Level ladder (monotonic)

| Level | Canvas | WebGL | Fonts | Timing | Screen / UA / cores / mem | API blocks |
|---|---|---|---|---|---|---|
| `EB_AFP_OFF` | — | — | — | — | — | — |
| `EB_AFP_STANDARD` (init default) | 2-bit noise | vendor → "WebKit" | restricted | 100µs jitter, 1ms resolution | — | WebRTC IP, battery, bluetooth, USB; audio noise 0.0001 |
| `EB_AFP_STRICT` | 4-bit noise | vendor+renderer → "WebKit", extensions blocked | restricted | 500µs jitter, 5ms resolution | 1920×1080×24, UA + 4 cores + 8 GB spoofed | all of STANDARD; audio noise 0.001 |
| `EB_AFP_TOR_LIKE` | readback blocked | vendor+renderer → "Mozilla", extensions blocked | restricted | 1ms jitter, 100ms resolution | 1000×1000×24, fixed UA pool, 2 cores / 8 GB | all of STRICT |

Each level is a superset of the previous: raising the level never
*removes* a protection.

### Fail-closed construction

1. `eb_afp_init` memsets the struct to zero **before** applying the
   level — a fresh struct can never sit partially configured, and every
   spoofed value is a fixed constant, never the real device value.
2. Every entry point is NULL-safe: a missing struct disables defenses,
   never crashes or half-applies them.
3. `eb_afp_calculate_score` lets CI assert the ladder — protections on
   ⇒ low entropy score; protections off ⇒ high.

### Integration (landed 2026-10-11)

`eb_priv_set_mode` drives `eb_afp_set_level` via the mode → AFP-level mapping
recorded above. Previously, embedders had to call `eb_afp_set_level`
explicitly — now the privacy engine owns the ladder end-to-end.

