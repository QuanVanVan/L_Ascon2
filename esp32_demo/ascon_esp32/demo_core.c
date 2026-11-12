/*
 * demo_core.c -- Ascon-AEAD128 (NIST SP 800-232) voi p_L thay the duoc, chay tren ESP32 (va PC de kiem thu).
 *
 * Che do AEAD chep cau truc cua ascon_aead_demo/c/ascon_aead.c (da khop 1089/1089 KAT chinh thuc), chi khac: hoan vi
 * la con tro ham chuyen biet cho tung tang (layers_gen.h) thay vi bo thong dich layer_apply.
 * Kiem chung tren board:
 *   (1) sinh lai tep KAT (luoi PT 0..32 x AD 0..32, dung dinh dang ascon_tool gen), bam SHA-256, so voi hash tep PC
 *       -- voi Ascon, hash nay = hash tep KAT chinh thuc; dong thoi moi vector phai giai ma dung va tu choi the gia;
 *   (2) perm64 (tu 64 bit) == perm32 (bit-interleaved) tren cac trang thai gia ngau nhien (seed co dinh).
 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2")             /* Arduino-ESP32 mac dinh -Os: ep O2 de do toc do cho cong bang moi tang */
#endif

#include "demo_core.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#include "esp_attr.h"
#define PERM_ATTR IRAM_ATTR            /* dat hoan vi trong IRAM: tranh sai so do cache flash */
#else
#define PERM_ATTR
#endif

#define ROL64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define ROR64(x, r) (((x) >> (r)) | ((x) << (64 - (r))))
#define ROR32(x, r) (((x) >> (r)) | ((x) << (32 - (r))))

#include "layers_gen.h"

/* ------------------------------------------------------------------ in ra */
static void out(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    plat_print(buf);
}

static void out_hex(const char *label, const uint8_t *b, size_t n)
{
    out("%s", label);
    for (size_t t = 0; t < n; t++) out("%02X", b[t]);
    out("%s\n", n ? "" : "(rong)");
}

/* ------------------------------------------------------------------ SHA-256 (FIPS 180-4), de doi chieu tep KAT */
typedef struct { uint32_t h[8]; uint8_t buf[64]; uint32_t nbuf; uint64_t len; } sha256_t;

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

static void sha256_block(sha256_t *c, const uint8_t *p)
{
    uint32_t w[64], a, b, cc, d, e, f, g, h;
    for (int t = 0; t < 16; t++) w[t] = (uint32_t)p[4 * t] << 24 | (uint32_t)p[4 * t + 1] << 16 | p[4 * t + 2] << 8 | p[4 * t + 3];
    for (int t = 16; t < 64; t++) {
        uint32_t s0 = ROR32(w[t - 15], 7) ^ ROR32(w[t - 15], 18) ^ (w[t - 15] >> 3);
        uint32_t s1 = ROR32(w[t - 2], 17) ^ ROR32(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3]; e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (int t = 0; t < 64; t++) {
        uint32_t t1 = h + (ROR32(e, 6) ^ ROR32(e, 11) ^ ROR32(e, 25)) + ((e & f) ^ (~e & g)) + K256[t] + w[t];
        uint32_t t2 = (ROR32(a, 2) ^ ROR32(a, 13) ^ ROR32(a, 22)) + ((a & b) ^ (a & cc) ^ (b & cc));
        h = g; g = f; f = e; e = d + t1; d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d; c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}

static void sha256_init(sha256_t *c)
{
    static const uint32_t H0[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    memcpy(c->h, H0, sizeof H0);
    c->nbuf = 0; c->len = 0;
}

static void sha256_update(sha256_t *c, const void *data, size_t n)
{
    const uint8_t *p = (const uint8_t *)data;
    c->len += n;
    while (n--) {
        c->buf[c->nbuf++] = *p++;
        if (c->nbuf == 64) { sha256_block(c, c->buf); c->nbuf = 0; }
    }
}

static void sha256_hex(sha256_t *c, char hex[65])
{
    uint64_t bits = c->len * 8;
    uint8_t pad = 0x80, z = 0, lenb[8];
    sha256_update(c, &pad, 1);
    while (c->nbuf != 56) sha256_update(c, &z, 1);
    for (int t = 0; t < 8; t++) lenb[t] = (uint8_t)(bits >> (56 - 8 * t));
    sha256_update(c, lenb, 8);
    for (int t = 0; t < 32; t++) snprintf(hex + 2 * t, 3, "%02x", (unsigned)(c->h[t / 4] >> (24 - 8 * (t % 4))) & 0xff);
}

/* ------------------------------------------------------------------ Ascon-AEAD128 */
#define IV   0x00001000808C0001ULL
#define RATE 16
#define DSEP (1ULL << 63)

static uint64_t load64(const uint8_t *b)
{
    uint64_t x = 0;
    for (int t = 7; t >= 0; t--) x = (x << 8) | b[t];
    return x;
}

static void store64(uint8_t *b, uint64_t x)
{
    for (int t = 0; t < 8; t++) { b[t] = (uint8_t)x; x >>= 8; }
}

static void absorb_init(const layer_t *L, uint64_t s[5], const uint8_t *ad, size_t adlen,
                        const uint8_t *nonce, const uint8_t *key)
{
    uint64_t k0 = load64(key), k1 = load64(key + 8);
    s[0] = IV; s[1] = k0; s[2] = k1; s[3] = load64(nonce); s[4] = load64(nonce + 8);
    L->perm64(s, 12);
    s[3] ^= k0; s[4] ^= k1;
    if (adlen) {
        while (adlen >= RATE) {
            s[0] ^= load64(ad); s[1] ^= load64(ad + 8);
            L->perm64(s, 8);
            ad += RATE; adlen -= RATE;
        }
        uint8_t blk[RATE] = { 0 };
        memcpy(blk, ad, adlen); blk[adlen] = 0x01;
        s[0] ^= load64(blk); s[1] ^= load64(blk + 8);
        L->perm64(s, 8);
    }
    s[4] ^= DSEP;
}

static void finalize(const layer_t *L, uint64_t s[5], const uint8_t *key, uint8_t tag[16])
{
    uint64_t k0 = load64(key), k1 = load64(key + 8);
    s[2] ^= k0; s[3] ^= k1;
    L->perm64(s, 12);
    store64(tag, s[3] ^ k0); store64(tag + 8, s[4] ^ k1);
}

static size_t aead_encrypt(const layer_t *L, uint8_t *c, const uint8_t *m, size_t mlen,
                           const uint8_t *ad, size_t adlen, const uint8_t *nonce, const uint8_t *key)
{
    uint64_t s[5];
    size_t clen = mlen + 16;
    absorb_init(L, s, ad, adlen, nonce, key);
    while (mlen >= RATE) {
        s[0] ^= load64(m); s[1] ^= load64(m + 8);
        store64(c, s[0]); store64(c + 8, s[1]);
        L->perm64(s, 8);
        m += RATE; c += RATE; mlen -= RATE;
    }
    uint8_t blk[RATE] = { 0 };
    memcpy(blk, m, mlen); blk[mlen] = 0x01;
    s[0] ^= load64(blk); s[1] ^= load64(blk + 8);
    store64(blk, s[0]); store64(blk + 8, s[1]);
    memcpy(c, blk, mlen);
    finalize(L, s, key, c + mlen);
    return clen;
}

static int aead_decrypt(const layer_t *L, uint8_t *m, const uint8_t *c, size_t clen,
                        const uint8_t *ad, size_t adlen, const uint8_t *nonce, const uint8_t *key)
{
    if (clen < 16) return -1;
    uint64_t s[5];
    size_t mlen = clen - 16, total = mlen;
    uint8_t *m0 = m;
    const uint8_t *tag = c + mlen;
    absorb_init(L, s, ad, adlen, nonce, key);
    while (mlen >= RATE) {
        uint64_t c0 = load64(c), c1 = load64(c + 8);
        store64(m, s[0] ^ c0); store64(m + 8, s[1] ^ c1);
        s[0] = c0; s[1] = c1;
        L->perm64(s, 8);
        m += RATE; c += RATE; mlen -= RATE;
    }
    uint8_t blk[RATE];
    store64(blk, s[0]); store64(blk + 8, s[1]);
    for (size_t t = 0; t < mlen; t++) { m[t] = blk[t] ^ c[t]; blk[t] = c[t]; }
    blk[mlen] ^= 0x01;
    s[0] = load64(blk); s[1] = load64(blk + 8);
    uint8_t t2[16], diff = 0;
    finalize(L, s, key, t2);
    for (int t = 0; t < 16; t++) diff |= t2[t] ^ tag[t];                 /* so sanh thoi gian hang */
    if (diff) { memset(m0, 0, total); return -1; }
    return 0;
}

/* ------------------------------------------------------------------ chon tang */
static int g_layer = -1;                                                 /* tang dung cho demo nhan tin */

static int find_layer(const char *name)
{
    for (int t = 0; t < NLAYERS; t++)
        if (!strcmp(LAYERS[t].name, name)) return t;
    return -1;
}

static const layer_t *cur_layer(void)
{
    if (g_layer < 0) g_layer = find_layer("L_Ascon2");
    if (g_layer < 0) g_layer = 0;
    return &LAYERS[g_layer];
}

int demo_nlayers(void) { return NLAYERS; }
const layer_t *demo_layer(int idx) { return idx >= 0 && idx < NLAYERS ? &LAYERS[idx] : NULL; }
int demo_find_layer(const char *name) { return find_layer(name); }
int demo_layer_index(void) { cur_layer(); return g_layer; }

void demo_set_layer(int idx)
{
    if (idx < 0 || idx >= NLAYERS) return;
    g_layer = idx;
    out("Tang hien tai: %s (%s)\n", LAYERS[idx].name, LAYERS[idx].spec);
}

/* ------------------------------------------------------------------ giao thuc goi tin */
static const uint8_t NET_KEY[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };  /* khoa chia se truoc (DEMO) */

static void put32(uint8_t *b, uint32_t v) { for (int t = 0; t < 4; t++) b[t] = (uint8_t)(v >> (8 * t)); }
static uint32_t get32(const uint8_t *b) { return b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24; }

size_t net_seal(uint8_t *pkt, int type, int layer_idx, uint32_t seq, const uint8_t session[7], uint8_t dir,
                const uint8_t *msg, size_t mlen)
{
    if (layer_idx < 0 || layer_idx >= NLAYERS || mlen > NET_MAX_MSG) return 0;
    uint8_t *nonce = pkt + NET_HDR;
    pkt[0] = 'S'; pkt[1] = 'M'; pkt[2] = 1; pkt[3] = (uint8_t)type; pkt[4] = (uint8_t)layer_idx;
    put32(pkt + 5, seq);
    nonce[0] = dir;
    memcpy(nonce + 1, session, 7);
    put32(nonce + 8, seq);
    memset(nonce + 12, 0, 4);
    return NET_HDR + 16 + aead_encrypt(&LAYERS[layer_idx], pkt + NET_HDR + 16, msg, mlen, pkt, NET_HDR, nonce, NET_KEY);
}

int net_open(const uint8_t *pkt, size_t n, uint8_t dir, int layer_idx, net_peer_t *peer, net_hdr_t *h,
             uint8_t *msg, size_t *mlen)
{
    const uint8_t *nonce = pkt + NET_HDR;
    *mlen = 0;
    if (n < NET_HDR + 32 || n > NET_MAX_PKT || pkt[0] != 'S' || pkt[1] != 'M' || pkt[2] != 1 || pkt[3] == NET_NACK)
        return NET_ERR_FORMAT;
    h->type = pkt[3]; h->layer = pkt[4]; h->seq = get32(pkt + 5);
    memcpy(h->session, nonce + 1, 7);
    if (nonce[0] != dir) return NET_ERR_DIR;
    if (get32(nonce + 8) != h->seq || nonce[12] | nonce[13] | nonce[14] | nonce[15]) return NET_ERR_FORMAT;
    size_t clen = n - NET_HDR - 16;
    if (aead_decrypt(&LAYERS[layer_idx], msg, nonce + 16, clen, pkt, NET_HDR, nonce, NET_KEY)) return NET_ERR_AUTH;
    if (peer) {                                    /* chi xet sau khi xac thuc: ke tan cong khong lam lech trang thai */
        if (peer->valid && !memcmp(peer->session, h->session, 7) && h->seq <= peer->last_seq) return NET_ERR_REPLAY;
        peer->valid = 1;
        memcpy(peer->session, h->session, 7);
        peer->last_seq = h->seq;
    }
    *mlen = clen - 16;
    return NET_OK;
}

const char *net_strerror(int code)
{
    switch (code) {
    case NET_OK: return "hop le";
    case NET_ERR_FORMAT: return "sai dinh dang";
    case NET_ERR_DIR: return "sai chieu (nonce)";
    case NET_ERR_AUTH: return "the xac thuc SAI (du lieu bi sua hoac khac tang)";
    case NET_ERR_REPLAY: return "PHAT LAI (seq da dung)";
    }
    return "?";
}

/* ------------------------------------------------------------------ (1) KAT: sinh lai tep, bam, kiem giai ma */
static void sha_hex(sha256_t *h, const char *label, const uint8_t *b, size_t n)
{
    char tmp[8];
    sha256_update(h, label, strlen(label));
    for (size_t t = 0; t < n; t++) { snprintf(tmp, sizeof tmp, "%02X", b[t]); sha256_update(h, tmp, 2); }
}

static int kat_layer(const layer_t *L)
{
    uint8_t key[16], nonce[16], pt[32], ad[32], ct[48], back[32];
    for (int t = 0; t < 16; t++) { key[t] = (uint8_t)t; nonce[t] = (uint8_t)(16 + t); }
    for (int t = 0; t < 32; t++) { pt[t] = (uint8_t)(0x20 + t); ad[t] = (uint8_t)(0x30 + t); }
    sha256_t h;
    sha256_init(&h);
    int count = 1, bad_dec = 0;
    char tmp[32];
    uint32_t c0 = plat_cycles();
    for (int lp = 0; lp <= 32; lp++) {
        for (int la = 0; la <= 32; la++) {
            size_t n = aead_encrypt(L, ct, pt, lp, ad, la, nonce, key);
            snprintf(tmp, sizeof tmp, "Count = %d\n", count++);
            sha256_update(&h, tmp, strlen(tmp));
            sha_hex(&h, "Key = ", key, 16);
            sha_hex(&h, "\nNonce = ", nonce, 16);
            sha_hex(&h, "\nPT = ", pt, lp);
            sha_hex(&h, "\nAD = ", ad, la);
            sha_hex(&h, "\nCT = ", ct, n);
            sha256_update(&h, "\n\n", 2);
            int ok = aead_decrypt(L, back, ct, n, ad, la, nonce, key) == 0 && !memcmp(back, pt, lp);
            ct[n - 1] ^= 1;
            ok &= aead_decrypt(L, back, ct, n, ad, la, nonce, key) != 0;
            bad_dec += !ok;
        }
        plat_yield();
    }
    uint32_t c1 = plat_cycles();
    char hex[65];
    sha256_hex(&h, hex);
    int hash_ok = !strcmp(hex, L->kat_sha256);
    out("  %-8s %d vector | giai ma + tu choi the gia: %s | SHA-256 KAT %.16s... %s | %lu ms\n", L->name, count - 1,
        bad_dec ? "LOI" : "dung", hex, hash_ok ? "== PC" : "KHAC PC (LOI)",
        (unsigned long)((c1 - c0) / (plat_cpu_mhz() * 1000UL)));
    return !hash_ok + (bad_dec != 0);
}

/* ------------------------------------------------------------------ (2) perm64 == perm32 */
static uint64_t g_rng = 0x0123456789ABCDEFULL;                          /* xorshift64, seed co dinh */

static uint64_t rng64(void)
{
    g_rng ^= g_rng << 13; g_rng ^= g_rng >> 7; g_rng ^= g_rng << 17;
    return g_rng;
}

static void to_bi(const uint64_t *x, uint32_t *y)
{
    for (int w = 0; w < 5; w++) {
        uint32_t e = 0, o = 0;
        for (int b = 0; b < 32; b++) {
            e |= (uint32_t)((x[w] >> (2 * b)) & 1) << b;
            o |= (uint32_t)((x[w] >> (2 * b + 1)) & 1) << b;
        }
        y[2 * w] = e; y[2 * w + 1] = o;
    }
}

static int cross_layer(const layer_t *L, int trials)
{
    int bad = 0;
    for (int t = 0; t < trials; t++) {
        uint64_t x[5];
        uint32_t y[10], z[10];
        for (int w = 0; w < 5; w++) x[w] = rng64();
        int rounds = (t & 1) ? 12 : 8;
        to_bi(x, y);
        L->perm64(x, rounds);
        L->perm32(y, rounds);
        to_bi(x, z);
        bad += memcmp(y, z, sizeof y) != 0;
    }
    return bad;
}

int demo_selftest(void)
{
    int fails = 0;
    out("\n=== TU KIEM TRA (%s, %lu MHz) ===\n", plat_name(), (unsigned long)plat_cpu_mhz());
    out("[1] Sinh lai KAT (33 x 33 = 1089 vector) va so SHA-256 voi tep do PC sinh (C = Python);\n"
        "    voi 'ascon' tep do trung tung byte tep KAT CHINH THUC (ascon-c).\n");
    for (int t = 0; t < NLAYERS; t++) fails += kat_layer(&LAYERS[t]);
    out("[2] Hoan vi tu 64 bit == hoan vi bit-interleaved 32 bit (1000 trang thai/tang, seed co dinh)\n");
    for (int t = 0; t < NLAYERS; t++) {
        int bad = cross_layer(&LAYERS[t], 1000);
        out("  %-8s %s\n", LAYERS[t].name, bad ? "LOI" : "1000/1000 khop");
        fails += bad;
        plat_yield();
    }
    out("TU KIEM TRA: %s\n", fails ? "CO LOI" : "TAT CA PASS");
    return fails;
}

/* ------------------------------------------------------------------ benchmark */
#define NS 31                                                            /* so mau, lay trung vi */

static uint32_t median(uint32_t *a, int n)
{
    for (int t = 1; t < n; t++) {
        uint32_t v = a[t];
        int u = t - 1;
        while (u >= 0 && a[u] > v) { a[u + 1] = a[u]; u--; }
        a[u + 1] = v;
    }
    return a[n / 2];
}

static uint64_t g_s64[5];
static uint32_t g_s32[10];
static uint8_t g_msg[1024], g_ct[1024 + 16];

typedef enum { B_PL, B_P64, B_P32, B_AEAD64, B_AEAD1K } bench_kind;

/* tra trung vi so chu ky cho MOT lan goi (pl/perm: trung binh trong khoi `inner` lan goi) */
static uint32_t measure(const layer_t *L, bench_kind kind)
{
    static const uint8_t key[16] = { 0 }, nonce[16] = { 0 };
    uint32_t smp[NS];
    int inner = kind == B_PL ? 100 : kind == B_AEAD1K ? 1 : 10;
    for (int k = -2; k < NS; k++) {                                      /* 2 lan dau: lam nong cache */
        uint32_t c0 = plat_cycles();
        for (int t = 0; t < inner; t++) {
            switch (kind) {
            case B_PL: L->pl(g_s64); break;
            case B_P64: L->perm64(g_s64, 12); break;
            case B_P32: L->perm32(g_s32, 12); break;
            case B_AEAD64: aead_encrypt(L, g_ct, g_msg, 64, NULL, 0, nonce, key); break;
            case B_AEAD1K: aead_encrypt(L, g_ct, g_msg, 1024, NULL, 0, nonce, key); break;
            }
        }
        uint32_t c1 = plat_cycles();
        if (k >= 0) smp[k] = (c1 - c0) / (uint32_t)inner;
    }
    return median(smp, NS);
}

void demo_bench(void)
{
    out("\n=== BENCHMARK (%s, %lu MHz; trung vi %d mau; don vi: chu ky CPU) ===\n", plat_name(),
        (unsigned long)plat_cpu_mhz(), NS);
    out("p_L     : 1 lan tang tuyen tinh (tu 64 bit)\n"
        "p12/64  : hoan vi 12 vong, tu 64 bit      p12/32: hoan vi 12 vong, bit-interleaved 32 bit\n"
        "AEAD 64B, AEAD 1KiB: ma hoa Ascon-AEAD128 (AD rong), cyc/B = chu ky/byte\n");
    out("%-8s %4s %6s %8s %8s %9s %10s %8s %8s\n", "tang", "XOR", "p_L", "p12/64", "p12/32", "AEAD64B", "AEAD1KiB",
        "cyc/B", "x Ascon");
    for (int t = 0; t < 1024; t++) g_msg[t] = (uint8_t)t;
    uint32_t base = 0;
    for (int t = 0; t < NLAYERS; t++) {
        const layer_t *L = &LAYERS[t];
        uint32_t pl = measure(L, B_PL), p64 = measure(L, B_P64), p32 = measure(L, B_P32);
        uint32_t a64 = measure(L, B_AEAD64), a1k = measure(L, B_AEAD1K);
        if (t == 0) base = a1k;
        out("%-8s %4d %6lu %8lu %8lu %9lu %10lu %8.2f %8.2f\n", L->name, L->xor64, (unsigned long)pl,
            (unsigned long)p64, (unsigned long)p32, (unsigned long)a64, (unsigned long)a1k, a1k / 1024.0,
            (double)a1k / base);
        plat_yield();
    }
    out("(checksum %08lx -- chong trinh bien dich loai bo phep tinh)\n",
        (unsigned long)(g_s64[0] ^ g_s64[4] ^ g_s32[3] ^ g_ct[5]));
}

/* ------------------------------------------------------------------ demo nhan tin */
static uint32_t g_msg_counter = 0;

static void demo_message(const char *msg)
{
    const layer_t *L = cur_layer();
    static const char *adtext = "header: DATN 2026";
    uint8_t key[16], nonce[16], c[256 + 16], back[257], bad[256 + 16];
    size_t mlen = strlen(msg);
    if (mlen > 256) mlen = 256;
    for (int t = 0; t < 16; t++) { key[t] = (uint8_t)t; nonce[t] = (uint8_t)(16 + t); }
    g_msg_counter++;                                                     /* nonce KHONG duoc lap lai */
    for (int t = 0; t < 4; t++) nonce[12 + t] ^= (uint8_t)(g_msg_counter >> (8 * t));

    out("\n--- Tang: %s  (%s, %d XOR 64 bit)\n", L->name, L->spec, L->xor64);
    out_hex("Khoa  K = ", key, 16);
    out_hex("Nonce N = ", nonce, 16);
    out("AD      = \"%s\"\nBan ro  = \"%.*s\" (%u byte)\n", adtext, (int)mlen, msg, (unsigned)mlen);
    uint32_t c0 = plat_cycles();
    size_t clen = aead_encrypt(L, c, (const uint8_t *)msg, mlen, (const uint8_t *)adtext, strlen(adtext), nonce, key);
    uint32_t c1 = plat_cycles();
    out_hex("Ban ma C = ", c, mlen);
    out_hex("The    T = ", c + mlen, 16);
    out("(ma hoa: %lu chu ky = %.1f us)\n", (unsigned long)(c1 - c0), (double)(c1 - c0) / plat_cpu_mhz());

    int rc = aead_decrypt(L, back, c, clen, (const uint8_t *)adtext, strlen(adtext), nonce, key);
    back[mlen] = 0;
    out("Giai ma : the %s, ban ro \"%s\" %s\n", rc ? "SAI" : "hop le", rc ? "" : (char *)back,
        rc == 0 && !memcmp(back, msg, mlen) ? "(trung ban goc)" : "(LOI)");

    memcpy(bad, c, clen); bad[clen - 1] ^= 1;
    rc = aead_decrypt(L, back, bad, clen, (const uint8_t *)adtext, strlen(adtext), nonce, key);
    out("Lat 1 bit cua the        -> %s\n", rc ? "tu choi (dung)" : "CHAP NHAN (SAI!)");
    if (mlen) {
        memcpy(bad, c, clen); bad[0] ^= 1;
        rc = aead_decrypt(L, back, bad, clen, (const uint8_t *)adtext, strlen(adtext), nonce, key);
        out("Lat 1 bit cua ban ma     -> %s\n", rc ? "tu choi (dung)" : "CHAP NHAN (SAI!)");
    }
    if (g_layer != 0) {
        rc = aead_decrypt(&LAYERS[0], back, c, clen, (const uint8_t *)adtext, strlen(adtext), nonce, key);
        out("Giai ma bang Ascon goc   -> %s\n", rc ? "tu choi (dung: hai hoan vi khac nhau)" : "CHAP NHAN (SAI!)");
    }
}

void demo_banner(void)
{
    out("\nAscon-AEAD128 / Ascon-SMRM tren %s -- DATN (tang tuyen tinh SMRM)\n", plat_name());
    out("Lenh: /kat (tu kiem tra)  /bench  /list  /layer <ten>  /help  -- hoac go ban ro bat ky de ma hoa.\n");
    out("Tang hien tai: %s\n", cur_layer()->name);
}

void demo_command(const char *line)
{
    while (*line == ' ') line++;
    if (!*line) return;
    if (!strcmp(line, "/kat")) demo_selftest();
    else if (!strcmp(line, "/bench")) demo_bench();
    else if (!strcmp(line, "/help")) demo_banner();
    else if (!strcmp(line, "/list")) {
        cur_layer();
        for (int t = 0; t < NLAYERS; t++)
            out("  %s %-8s %s\n", t == g_layer ? "*" : " ", LAYERS[t].name, LAYERS[t].spec);
    } else if (!strncmp(line, "/layer ", 7)) {
        int k = find_layer(line + 7);
        if (k < 0) out("Khong co tang '%s' (xem /list)\n", line + 7);
        else demo_set_layer(k);
    } else if (line[0] == '/') out("Lenh khong hop le (xem /help)\n");
    else demo_message(line);
}
