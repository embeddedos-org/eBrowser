// SPDX-License-Identifier: MIT
#include "eBrowser/tls.h"
#include "eBrowser/compat.h"
#include "eBrowser/crypto.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* mbedtls 3.x moved OID constants out of public umbrella headers and made some
 * x509 fields PRIVATE. Pull these in explicitly so newer toolchains compile. */
#ifdef EB_USE_MBEDTLS
#include "mbedtls/oid.h"
#include "mbedtls/x509_crt.h"
#ifndef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(member) member
#endif
#endif

eb_tls_config_t eb_tls_config_default(void) {
    eb_tls_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.min_version = EB_TLS_1_2;
    cfg.max_version = EB_TLS_1_3;
    cfg.verify_peer = true;
    cfg.verify_hostname = true;
    cfg.ca_cert_path = NULL;
    cfg.preferred_ciphers[0] = EB_CIPHER_AES_128_GCM_SHA256;
    cfg.preferred_ciphers[1] = EB_CIPHER_AES_256_GCM_SHA384;
    cfg.preferred_ciphers[2] = EB_CIPHER_AES_128_CBC_SHA256;
    cfg.cipher_count = 3;
    return cfg;
}

#ifdef EB_USE_MBEDTLS

int eb_tls_init(eb_tls_ctx_t *ctx, const eb_tls_config_t *config) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(eb_tls_ctx_t));
    ctx->state = EB_TLS_STATE_IDLE;
    ctx->socket_fd = -1;

    if (config) {
        ctx->version = config->max_version;
        ctx->verify_peer = config->verify_peer;
        ctx->verify_hostname = config->verify_hostname;
        ctx->cipher_suite = config->cipher_count > 0 ? config->preferred_ciphers[0] : EB_CIPHER_AES_128_GCM_SHA256;
    } else {
        ctx->version = EB_TLS_1_2;
        ctx->verify_peer = true;
        ctx->verify_hostname = true;
        ctx->cipher_suite = EB_CIPHER_AES_128_GCM_SHA256;
    }
    eb_crypto_random_bytes(ctx->client_random, 32);

    /* Initialize mbedTLS contexts */
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_x509_crt_init(&ctx->cacert);
    mbedtls_net_init(&ctx->server_fd);

    /* Seed the PRNG */
    int ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                     &ctx->entropy,
                                     (const unsigned char *)"eBrowser", 8);
    if (ret != 0) {
        fprintf(stderr, "eb_tls_init: ctr_drbg_seed failed: -0x%04x\n", (unsigned)-ret);
        return -1;
    }

    /* Configure SSL defaults (client, TCP, TLS) */
    ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                       MBEDTLS_SSL_IS_CLIENT,
                                       MBEDTLS_SSL_TRANSPORT_STREAM,
                                       MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        fprintf(stderr, "eb_tls_init: ssl_config_defaults failed: -0x%04x\n", (unsigned)-ret);
        return -1;
    }

    /* Set certificate verification mode */
    mbedtls_ssl_conf_authmode(&ctx->conf,
        ctx->verify_peer ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);

    /* Set RNG */
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    /* Load CA certificates */
    if (config && config->ca_cert_path) {
        ret = mbedtls_x509_crt_parse_path(&ctx->cacert, config->ca_cert_path);
    } else {
        /* Try common system CA paths */
#if defined(_WIN32)
        /* On Windows, mbedTLS can use the Windows certificate store via parse_path
         * but typically we'd need a bundled CA file. Try common locations. */
        ret = mbedtls_x509_crt_parse_path(&ctx->cacert, "C:\\ProgramData\\ssl\\certs");
        if (ret != 0) {
            /* Fallback: skip CA loading, rely on VERIFY_OPTIONAL if no certs available */
            ret = 0;
            mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        }
#elif defined(__APPLE__)
        ret = mbedtls_x509_crt_parse_path(&ctx->cacert, "/etc/ssl/certs");
        if (ret != 0)
            ret = mbedtls_x509_crt_parse_path(&ctx->cacert, "/usr/local/etc/openssl/certs");
#else
        ret = mbedtls_x509_crt_parse_path(&ctx->cacert, "/etc/ssl/certs");
        if (ret != 0)
            ret = mbedtls_x509_crt_parse_path(&ctx->cacert, "/usr/share/ca-certificates");
#endif
    }

    /* Set CA chain (even if empty — verification will handle it) */
    mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);

    /* Apply config to SSL context */
    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if (ret != 0) {
        fprintf(stderr, "eb_tls_init: ssl_setup failed: -0x%04x\n", (unsigned)-ret);
        return -1;
    }

    return 0;
}

int eb_tls_set_hostname(eb_tls_ctx_t *ctx, const char *hostname) {
    if (!ctx || !hostname) return -1;
    strncpy(ctx->hostname, hostname, EB_TLS_MAX_HOSTNAME - 1);
    strncpy(ctx->session.hostname, hostname, EB_TLS_MAX_HOSTNAME - 1);

    /* Set SNI hostname for mbedTLS */
    int ret = mbedtls_ssl_set_hostname(&ctx->ssl, hostname);
    if (ret != 0) {
        fprintf(stderr, "eb_tls_set_hostname: ssl_set_hostname failed: -0x%04x\n", (unsigned)-ret);
        return -1;
    }
    return 0;
}

int eb_tls_handshake(eb_tls_ctx_t *ctx, int socket_fd) {
    if (!ctx) return -1;

    ctx->state = EB_TLS_STATE_CLIENT_HELLO;
    ctx->socket_fd = socket_fd;

    /* Wrap the socket FD for mbedTLS I/O */
    ctx->server_fd.fd = socket_fd;
    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->server_fd,
                         mbedtls_net_send, mbedtls_net_recv, NULL);

    /* Perform the TLS handshake */
    int ret;
    while ((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            char errbuf[128];
            mbedtls_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "eb_tls_handshake: failed: -0x%04x (%s)\n", (unsigned)-ret, errbuf);
            ctx->state = EB_TLS_STATE_ERROR;
            return -1;
        }
    }

    ctx->state = EB_TLS_STATE_ESTABLISHED;

    /* Extract peer certificate info */
    const mbedtls_x509_crt *peer_cert = mbedtls_ssl_get_peer_cert(&ctx->ssl);
    if (peer_cert) {
        ctx->peer_certs.cert_count = 0;
        const mbedtls_x509_crt *cur = peer_cert;
        while (cur && ctx->peer_certs.cert_count < EB_TLS_MAX_CERT_CHAIN) {
            eb_certificate_t *ec = &ctx->peer_certs.certs[ctx->peer_certs.cert_count];
            memset(ec, 0, sizeof(*ec));

            /* Extract subject and issuer as strings */
            mbedtls_x509_dn_gets(ec->subject, sizeof(ec->subject), &cur->subject);
            mbedtls_x509_dn_gets(ec->issuer, sizeof(ec->issuer), &cur->issuer);

            /* Extract common name from subject */
            const mbedtls_x509_name *name = &cur->subject;
            while (name) {
                if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &name->oid) == 0) {
                    size_t cn_len = name->val.len < sizeof(ec->common_name) - 1
                                    ? name->val.len : sizeof(ec->common_name) - 1;
                    memcpy(ec->common_name, name->val.p, cn_len);
                    ec->common_name[cn_len] = '\0';
                    break;
                }
                name = name->next;
            }

            ec->raw_data = cur->raw.p;
            ec->raw_len = cur->raw.len;
            ec->is_ca = cur->MBEDTLS_PRIVATE(ca_istrue) != 0;

            /* Compute SHA-256 fingerprint */
            eb_sha256(cur->raw.p, cur->raw.len, ec->fingerprint_sha256);

            ctx->peer_certs.cert_count++;
            cur = cur->next;
        }
        ctx->peer_certs.status = EB_CERT_OK;
    }

    /* Populate session info */
    eb_crypto_random_bytes(ctx->session.session_id, EB_TLS_SESSION_ID_LEN);
    eb_crypto_random_bytes(ctx->session.master_secret, EB_TLS_MASTER_SECRET_LEN);
    ctx->session.valid = true;

    return 0;
}

int eb_tls_write(eb_tls_ctx_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data || ctx->state != EB_TLS_STATE_ESTABLISHED) return -1;

    size_t written = 0;
    while (written < len) {
        int ret = mbedtls_ssl_write(&ctx->ssl, data + written, len - written);
        if (ret > 0) {
            written += (size_t)ret;
        } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        } else {
            return -1;
        }
    }
    return (int)written;
}

int eb_tls_read(eb_tls_ctx_t *ctx, uint8_t *buf, size_t buf_len) {
    if (!ctx || !buf || ctx->state != EB_TLS_STATE_ESTABLISHED) return -1;

    int ret;
    do {
        ret = mbedtls_ssl_read(&ctx->ssl, buf, buf_len);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ);

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        return 0; /* Clean EOF */
    }
    return ret; /* >0 = bytes read, <0 = error */
}

void eb_tls_close(eb_tls_ctx_t *ctx) {
    if (!ctx) return;
    if (ctx->state == EB_TLS_STATE_ESTABLISHED) {
        mbedtls_ssl_close_notify(&ctx->ssl);
    }
    ctx->state = EB_TLS_STATE_CLOSED;
    eb_crypto_secure_zero(ctx->session.master_secret, EB_TLS_MASTER_SECRET_LEN);
}

void eb_tls_free(eb_tls_ctx_t *ctx) {
    if (!ctx) return;
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_net_free(&ctx->server_fd);
    eb_crypto_secure_zero(ctx, sizeof(eb_tls_ctx_t));
}

#else /* !EB_USE_MBEDTLS — stub fallback */

int eb_tls_init(eb_tls_ctx_t *ctx, const eb_tls_config_t *config) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(eb_tls_ctx_t));
    ctx->state = EB_TLS_STATE_IDLE;
    if (config) {
        ctx->version = config->max_version;
        ctx->verify_peer = config->verify_peer;
        ctx->verify_hostname = config->verify_hostname;
        ctx->cipher_suite = config->cipher_count > 0 ? config->preferred_ciphers[0] : EB_CIPHER_AES_128_GCM_SHA256;
    } else {
        ctx->version = EB_TLS_1_2;
        ctx->verify_peer = true;
        ctx->verify_hostname = true;
        ctx->cipher_suite = EB_CIPHER_AES_128_GCM_SHA256;
    }
    eb_crypto_random_bytes(ctx->client_random, 32);
    return 0;
}

int eb_tls_set_hostname(eb_tls_ctx_t *ctx, const char *hostname) {
    if (!ctx || !hostname) return -1;
    strncpy(ctx->hostname, hostname, EB_TLS_MAX_HOSTNAME - 1);
    strncpy(ctx->session.hostname, hostname, EB_TLS_MAX_HOSTNAME - 1);
    return 0;
}

int eb_tls_handshake(eb_tls_ctx_t *ctx, int socket_fd) {
    // TODO: STUB — no real TLS handshake is performed. This function simulates
    // state transitions without actual network I/O or cryptographic negotiation.
    // A production implementation must perform a real TLS handshake over socket_fd.
    if (!ctx) return -1;
    (void)socket_fd;
    fprintf(stderr, "WARNING: eb_tls_handshake stub — no real TLS. Build with eBrowser_USE_MBEDTLS=ON.\n");
    ctx->state = EB_TLS_STATE_CLIENT_HELLO;
    ctx->state = EB_TLS_STATE_SERVER_HELLO;
    eb_crypto_random_bytes(ctx->server_random, 32);
    ctx->state = EB_TLS_STATE_CERTIFICATE;

    // Validate peer certificate during handshake
    if (ctx->verify_peer) {
        eb_cert_status_t cert_status = eb_cert_validate_chain(
            &ctx->peer_certs, ctx->verify_hostname ? ctx->hostname : NULL);
        if (cert_status != EB_CERT_OK) {
            fprintf(stderr, "ERROR: TLS certificate validation failed: %s\n",
                    eb_cert_status_string(cert_status));
            ctx->state = EB_TLS_STATE_CLOSED;
            return -1;
        }
    }

    ctx->state = EB_TLS_STATE_KEY_EXCHANGE;
    eb_crypto_random_bytes(ctx->session.session_id, EB_TLS_SESSION_ID_LEN);
    eb_crypto_random_bytes(ctx->session.master_secret, EB_TLS_MASTER_SECRET_LEN);
    ctx->session.valid = true;
    ctx->state = EB_TLS_STATE_FINISHED;
    ctx->state = EB_TLS_STATE_ESTABLISHED;
    return 0;
}

int eb_tls_write(eb_tls_ctx_t *ctx, const uint8_t *data, size_t len) {
    // TODO: STUB — data is not actually encrypted or sent over the network.
    // A production implementation must encrypt data using the negotiated
    // cipher suite and write TLS records to the underlying socket.
    if (!ctx || !data || ctx->state != EB_TLS_STATE_ESTABLISHED) return -1;
    fprintf(stderr, "WARNING: eb_tls_write stub — no encryption. Build with eBrowser_USE_MBEDTLS=ON.\n");
    (void)len;
    return (int)len;
}

int eb_tls_read(eb_tls_ctx_t *ctx, uint8_t *buf, size_t buf_len) {
    // TODO: STUB — no data is actually read or decrypted from the network.
    // A production implementation must read TLS records from the underlying
    // socket and decrypt them using the negotiated cipher suite.
    if (!ctx || !buf || ctx->state != EB_TLS_STATE_ESTABLISHED) return -1;
    fprintf(stderr, "WARNING: eb_tls_read stub — no decryption. Build with eBrowser_USE_MBEDTLS=ON.\n");
    (void)buf_len;
    return 0;
}

void eb_tls_close(eb_tls_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state = EB_TLS_STATE_CLOSED;
    eb_crypto_secure_zero(ctx->session.master_secret, EB_TLS_MASTER_SECRET_LEN);
}

void eb_tls_free(eb_tls_ctx_t *ctx) {
    if (!ctx) return;
    eb_crypto_secure_zero(ctx, sizeof(eb_tls_ctx_t));
}

#endif /* EB_USE_MBEDTLS */

/* --- Shared code (not mbedTLS-dependent) --- */

eb_cert_status_t eb_tls_verify_certificate(const eb_tls_ctx_t *ctx) {
    if (!ctx) return EB_CERT_INVALID_SIGNATURE;
    return ctx->peer_certs.status;
}

const eb_certificate_t *eb_tls_get_peer_certificate(const eb_tls_ctx_t *ctx) {
    if (!ctx || ctx->peer_certs.cert_count == 0) return NULL;
    return &ctx->peer_certs.certs[0];
}

eb_cert_status_t eb_cert_check_expiry(const eb_certificate_t *cert, uint64_t now) {
    if (!cert) return EB_CERT_INVALID_SIGNATURE;
    if (now < cert->not_before) return EB_CERT_NOT_YET_VALID;
    if (now > cert->not_after) return EB_CERT_EXPIRED;
    return EB_CERT_OK;
}

bool eb_cert_hostname_match(const char *cert_cn, const char *hostname) {
    if (!cert_cn || !hostname) return false;
    if (cert_cn[0] == '*' && cert_cn[1] == '.') {
        const char *dot = strchr(hostname, '.');
        if (!dot) return false;
        return strcasecmp(cert_cn + 2, dot + 1) == 0;
    }
    return strcasecmp(cert_cn, hostname) == 0;
}

eb_cert_status_t eb_cert_validate_chain(const eb_cert_chain_t *chain, const char *hostname) {
    if (!chain || chain->cert_count == 0) return EB_CERT_UNTRUSTED_CA;
    if (chain->cert_count > EB_TLS_MAX_CERT_CHAIN) return EB_CERT_CHAIN_TOO_LONG;
    if (hostname && !eb_cert_hostname_match(chain->certs[0].common_name, hostname))
        return EB_CERT_HOSTNAME_MISMATCH;
    if (chain->cert_count == 1 && !chain->certs[0].is_ca)
        return EB_CERT_SELF_SIGNED;
    return EB_CERT_OK;
}

const char *eb_cert_status_string(eb_cert_status_t status) {
    switch (status) {
        case EB_CERT_OK:                return "Certificate valid";
        case EB_CERT_EXPIRED:           return "Certificate expired";
        case EB_CERT_NOT_YET_VALID:     return "Certificate not yet valid";
        case EB_CERT_HOSTNAME_MISMATCH: return "Hostname mismatch";
        case EB_CERT_UNTRUSTED_CA:      return "Untrusted certificate authority";
        case EB_CERT_SELF_SIGNED:       return "Self-signed certificate";
        case EB_CERT_REVOKED:           return "Certificate revoked";
        case EB_CERT_INVALID_SIGNATURE: return "Invalid signature";
        case EB_CERT_CHAIN_TOO_LONG:    return "Certificate chain too long";
        default:                         return "Unknown certificate error";
    }
}

static eb_tls_session_t s_session_cache[16];
static int s_session_count = 0;

int eb_tls_session_save(const eb_tls_session_t *session) {
    if (!session || s_session_count >= 16) return -1;
    for (int i = 0; i < s_session_count; i++) {
        if (strcmp(s_session_cache[i].hostname, session->hostname) == 0) {
            s_session_cache[i] = *session;
            return 0;
        }
    }
    s_session_cache[s_session_count++] = *session;
    return 0;
}

int eb_tls_session_load(const char *hostname, eb_tls_session_t *session) {
    if (!hostname || !session) return -1;
    for (int i = 0; i < s_session_count; i++) {
        if (strcmp(s_session_cache[i].hostname, hostname) == 0 && s_session_cache[i].valid) {
            *session = s_session_cache[i];
            return 0;
        }
    }
    return -1;
}

void eb_tls_session_invalidate(const char *hostname) {
    if (!hostname) return;
    for (int i = 0; i < s_session_count; i++) {
        if (strcmp(s_session_cache[i].hostname, hostname) == 0) {
            eb_crypto_secure_zero(&s_session_cache[i], sizeof(eb_tls_session_t));
        }
    }
}
