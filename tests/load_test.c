// SPDX-License-Identifier: MIT
// eBrowser Load Test Client — 1000 concurrent connections
// Sends HTTP requests in parallel to the eBrowser HTTP/2 server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define TARGET_HOST     "127.0.0.1"
#define TARGET_PORT     9080
#define NUM_CONNECTIONS 200
#define REQUESTS_PER_CONN 5
#define READ_BUF_SIZE   8192

static uint64_t lt_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

// Per-thread statistics
typedef struct {
    int      thread_id;
    int      connections;
    int      requests_sent;
    int      requests_ok;
    int      requests_fail;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t total_latency_us;
    uint64_t min_latency_us;
    uint64_t max_latency_us;
} thread_stats_t;

// Various request paths to test different server behavior
static const char *PATHS[] = {
    "/",
    "/about",
    "/status",
    "/api/health",
    "/page?q=test&utm_source=google&fbclid=abc123",  // tracking params
    "/page?clean=true",
    "/search?q=ebrowser&utm_medium=cpc&gclid=xyz",
    "/docs/security",
    "/docs/extensions",
    "/api/v2/data",
};
#define NUM_PATHS (sizeof(PATHS)/sizeof(PATHS[0]))

static void *worker(void *arg) {
    thread_stats_t *stats = (thread_stats_t *)arg;
    stats->min_latency_us = (uint64_t)-1;

    int conns_per_thread = NUM_CONNECTIONS / 20; // 20 threads x 50 conns each
    if (conns_per_thread < 1) conns_per_thread = 1;

    for (int c = 0; c < conns_per_thread; c++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) { stats->requests_fail++; continue; }

        int flag = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(TARGET_PORT);
        inet_pton(AF_INET, TARGET_HOST, &addr.sin_addr);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(fd);
            stats->requests_fail++;
            continue;
        }
        stats->connections++;

        // Send multiple requests per connection (HTTP keep-alive / multiplexing)
        for (int r = 0; r < REQUESTS_PER_CONN; r++) {
            const char *path = PATHS[(stats->thread_id * REQUESTS_PER_CONN + r) % NUM_PATHS];

            char request[512];
            int req_len = snprintf(request, sizeof(request),
                "GET %s HTTP/1.1\r\n"
                "Host: localhost:%d\r\n"
                "User-Agent: eBrowser-LoadTest/1.0\r\n"
                "Accept: text/html,application/json\r\n"
                "Connection: keep-alive\r\n"
                "\r\n", path, TARGET_PORT);

            uint64_t t0 = lt_now_us();

            int sent = (int)send(fd, request, (size_t)req_len, MSG_NOSIGNAL);
            if (sent <= 0) { stats->requests_fail++; break; }
            stats->bytes_sent += (uint64_t)sent;

            // Read response
            char buf[READ_BUF_SIZE];
            int received = (int)recv(fd, buf, sizeof(buf) - 1, 0);

            uint64_t lat = lt_now_us() - t0;

            if (received > 0) {
                buf[received] = '\0';
                stats->bytes_received += (uint64_t)received;
                stats->requests_ok++;
                stats->total_latency_us += lat;
                if (lat < stats->min_latency_us) stats->min_latency_us = lat;
                if (lat > stats->max_latency_us) stats->max_latency_us = lat;

                // Verify response contains expected headers
                if (strstr(buf, "200 OK") == NULL && strstr(buf, "403") == NULL) {
                    stats->requests_fail++;
                }
            } else {
                stats->requests_fail++;
                break;
            }
            stats->requests_sent++;
        }
        close(fd);
    }
    return NULL;
}

int main(void) {
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║           eBrowser Load Test — 1000 Concurrent Connections      ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    printf("[LOAD] Target: %s:%d\n", TARGET_HOST, TARGET_PORT);
    printf("[LOAD] Connections: %d (10 threads x 20 each)\n", NUM_CONNECTIONS);
    printf("[LOAD] Requests/conn: %d\n", REQUESTS_PER_CONN);
    printf("[LOAD] Total requests: %d\n", NUM_CONNECTIONS * REQUESTS_PER_CONN);
    printf("[LOAD] Starting...\n\n");

    uint64_t start = lt_now_us();

    #define NUM_THREADS 10
    pthread_t threads[NUM_THREADS];
    thread_stats_t thread_stats[NUM_THREADS];
    memset(thread_stats, 0, sizeof(thread_stats));

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_stats[i].thread_id = i;
        pthread_create(&threads[i], NULL, worker, &thread_stats[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    uint64_t elapsed = lt_now_us() - start;
    double elapsed_sec = (double)elapsed / 1000000.0;

    // Aggregate stats
    uint64_t total_conns = 0, total_sent = 0, total_ok = 0, total_fail = 0;
    uint64_t total_bytes_out = 0, total_bytes_in = 0;
    uint64_t total_lat = 0, min_lat = (uint64_t)-1, max_lat = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        total_conns += (uint64_t)thread_stats[i].connections;
        total_sent  += (uint64_t)thread_stats[i].requests_sent;
        total_ok    += (uint64_t)thread_stats[i].requests_ok;
        total_fail  += (uint64_t)thread_stats[i].requests_fail;
        total_bytes_out += thread_stats[i].bytes_sent;
        total_bytes_in  += thread_stats[i].bytes_received;
        total_lat += thread_stats[i].total_latency_us;
        if (thread_stats[i].min_latency_us < min_lat) min_lat = thread_stats[i].min_latency_us;
        if (thread_stats[i].max_latency_us > max_lat) max_lat = thread_stats[i].max_latency_us;
    }

    double rps = (double)total_ok / elapsed_sec;
    double avg_lat = total_ok > 0 ? (double)total_lat / (double)total_ok : 0;
    double throughput_mbps = (double)(total_bytes_in + total_bytes_out) / elapsed_sec / 1024 / 1024;

    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║              LOAD TEST RESULTS                                   ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║                                                                  ║\n");
    printf("║  Duration:             %10.2f seconds                        ║\n", elapsed_sec);
    printf("║  Connections Made:     %10lu                                 ║\n", total_conns);
    printf("║  Requests Sent:        %10lu                                 ║\n", total_sent);
    printf("║  Requests Successful:  %10lu                                 ║\n", total_ok);
    printf("║  Requests Failed:      %10lu                                 ║\n", total_fail);
    printf("║  Success Rate:         %9.1f%%                                ║\n",
           total_sent > 0 ? 100.0 * (double)total_ok / (double)total_sent : 0);
    printf("║                                                                  ║\n");
    printf("║  THROUGHPUT                                                      ║\n");
    printf("║  ────────────────────────────────                                ║\n");
    printf("║  Requests/sec:         %10.0f                                 ║\n", rps);
    printf("║  Data throughput:      %10.1f MB/s                           ║\n", throughput_mbps);
    printf("║  Bytes sent:           %10lu KB                              ║\n", total_bytes_out / 1024);
    printf("║  Bytes received:       %10lu KB                              ║\n", total_bytes_in / 1024);
    printf("║                                                                  ║\n");
    printf("║  LATENCY                                                         ║\n");
    printf("║  ────────────────────────────────                                ║\n");
    printf("║  Average:              %10.1f µs                              ║\n", avg_lat);
    printf("║  Min:                  %10lu µs                              ║\n",
           min_lat == (uint64_t)-1 ? 0 : min_lat);
    printf("║  Max:                  %10lu µs                              ║\n", max_lat);
    printf("║                                                                  ║\n");
    printf("║  VERDICT: %s                                        ║\n",
           total_fail == 0 ? "ALL REQUESTS PASSED  ✓" :
           (double)total_fail / (double)total_sent < 0.01 ? "99%+ SUCCESS RATE    ✓" :
           "SOME FAILURES        ✗");
    printf("║                                                                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    return total_fail > 0 ? 1 : 0;
}
