/*
 * host_main.c -- chay demo_core.c tren PC (kiem thu truoc khi nap ESP32).
 *   host_test                                   tu kiem tra + benchmark (chu ky = TSC cua x86, chi de tham khao)
 *   host_test "<lenh>" ...                      chay tung lenh nhu go tren Serial, vd. "/layer ascon" "Xin chao"
 *   host_test --seal <type> <tang> <seq> <session_hex14> <dir> <msg_hex>      in goi (hex)
 *   host_test --open <dir> <tang> <goi_hex>     in "OK <type> <layer> <seq> <session_hex> <msg_hex>" hoac "ERR <ma>"
 * (--seal / --open dung cho host/test_net.py: doi chieu giao thuc C <-> Python)
 */
#include "../ascon_esp32/demo_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

uint32_t plat_cycles(void) { return (uint32_t)__rdtsc(); }
uint32_t plat_cpu_mhz(void) { return 1000; }                 /* TSC khong phai chu ky loi: so us chi mang tinh minh hoa */
const char *plat_name(void) { return "PC-host"; }
void plat_print(const char *s) { fputs(s, stdout); }
void plat_yield(void) {}

static size_t unhex(uint8_t *out, size_t cap, const char *s)
{
    size_t n = 0;
    unsigned v;
    while (s[0] && s[1] && n < cap && sscanf(s, "%2x", &v) == 1) { out[n++] = (uint8_t)v; s += 2; }
    return n;
}

static void puthex(const uint8_t *b, size_t n)
{
    for (size_t t = 0; t < n; t++) printf("%02x", b[t]);
}

static int layer_arg(const char *name)
{
    int k = demo_find_layer(name);
    if (k < 0) { fprintf(stderr, "khong co tang %s\n", name); exit(2); }
    return k;
}

int main(int argc, char **argv)
{
    if (argc == 8 && !strcmp(argv[1], "--seal")) {
        uint8_t session[7], msg[NET_MAX_MSG + 1], pkt[NET_MAX_PKT];
        unhex(session, 7, argv[5]);
        size_t mlen = unhex(msg, sizeof msg, argv[7]);
        size_t n = net_seal(pkt, atoi(argv[2]), layer_arg(argv[3]), (uint32_t)strtoul(argv[4], NULL, 10), session,
                            (uint8_t)atoi(argv[6]), msg, mlen);
        if (!n) { puts("ERR"); return 1; }
        puthex(pkt, n);
        putchar('\n');
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "--open")) {
        static uint8_t pkt[4096];
        uint8_t msg[NET_MAX_MSG];
        size_t mlen, n = unhex(pkt, sizeof pkt, argv[4]);
        net_hdr_t h;
        int rc = net_open(pkt, n, (uint8_t)atoi(argv[2]), layer_arg(argv[3]), NULL, &h, msg, &mlen);
        if (rc) { printf("ERR %d\n", rc); return 0; }
        printf("OK %d %d %lu ", h.type, h.layer, (unsigned long)h.seq);
        puthex(h.session, 7);
        putchar(' ');
        puthex(msg, mlen);
        putchar('\n');
        return 0;
    }
    if (argc > 1) {
        for (int t = 1; t < argc; t++) demo_command(argv[t]);
        return 0;
    }
    demo_banner();
    int fails = demo_selftest();
    demo_bench();
    return fails ? 1 : 0;
}
