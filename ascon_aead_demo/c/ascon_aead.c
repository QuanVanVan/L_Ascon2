/*
 * ascon_aead.c -- Ascon-AEAD128 (NIST SP 800-232) voi tang tuyen tinh p_L thay the duoc.
 *
 * Hoan vi: p = p_L o p_S o p_C, 12 vong (p^a) khi khoi tao / ket thuc, 8 vong (p^b) khi xu ly du lieu.
 * Chi p_L thay doi theo tham so; p_C (hang so vong), p_S (S-box 5 bit), IV va che do AEAD giu nguyen.
 *   Ascon : x_i ^= (x_i >>> a_i) ^ (x_i >>> b_i)
 *   SMRM  : for l = k..1: x[j_l] ^= rotl(x[i_l], r_l);  x[w] = rotl(x[w], d_w);  for l = 1..k: x[i_l] ^= rotl(x[j_l], r_l)
 */
#include "ascon_aead.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint64_t rotl64(uint64_t x, int r) { r &= 63; return r ? (x << r) | (x >> (64 - r)) : x; }
static inline uint64_t rotr64(uint64_t x, int r) { return rotl64(x, 64 - (r & 63)); }

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

/* ------------------------------------------------------------------ tang tuyen tinh */
static int parse_smrm(linear_layer *L, const char *body)
{
    const char *bar = strchr(body, '|');
    if (!bar) return -1;
    L->is_ascon = 0; L->k = 0;
    const char *p = body;
    while (p < bar) {
        int i, j, r, n;
        if (sscanf(p, "%d,%d,%d%n", &i, &j, &r, &n) != 3 || L->k >= SMRM_MAX_OPS) return -1;
        if (i < 0 || i > 4 || j < 0 || j > 4 || i == j || r < 0 || r > 63) return -1;
        L->i[L->k] = i; L->j[L->k] = j; L->r[L->k] = r; L->k++;
        p += n;
        if (*p == ';') p++;
    }
    if (sscanf(bar + 1, "%d,%d,%d,%d,%d", &L->d[0], &L->d[1], &L->d[2], &L->d[3], &L->d[4]) != 5) return -1;
    return L->k > 0 ? 0 : -1;
}

int layer_parse(linear_layer *L, const char *spec, const char *finalists_path)
{
    memset(L, 0, sizeof *L);
    snprintf(L->name, sizeof L->name, "%s", spec);
    if (!strcmp(spec, "ascon")) { L->is_ascon = 1; return 0; }
    if (!strncmp(spec, "smrm:", 5)) { snprintf(L->name, sizeof L->name, "SMRM"); return parse_smrm(L, spec + 5); }
    /* ten ung vien: dong "TEN|ops|d" trong finalists.txt */
    FILE *f = finalists_path ? fopen(finalists_path, "r") : NULL;
    if (!f) return -1;
    char line[512];
    size_t n = strlen(spec);
    int rc = -1;
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, spec, n) && line[n] == '|') {
            line[strcspn(line, "\r\n")] = 0;
            rc = parse_smrm(L, line + n + 1);
            break;
        }
    fclose(f);
    return rc;
}

void layer_apply(const linear_layer *L, uint64_t s[5])
{
    if (L->is_ascon) {
        s[0] ^= rotr64(s[0], 19) ^ rotr64(s[0], 28);
        s[1] ^= rotr64(s[1], 61) ^ rotr64(s[1], 39);
        s[2] ^= rotr64(s[2], 1) ^ rotr64(s[2], 6);
        s[3] ^= rotr64(s[3], 10) ^ rotr64(s[3], 17);
        s[4] ^= rotr64(s[4], 7) ^ rotr64(s[4], 41);
        return;
    }
    for (int l = L->k - 1; l >= 0; l--) s[L->j[l]] ^= rotl64(s[L->i[l]], L->r[l]);
    for (int w = 0; w < 5; w++) s[w] = rotl64(s[w], L->d[w]);
    for (int l = 0; l < L->k; l++) s[L->i[l]] ^= rotl64(s[L->j[l]], L->r[l]);
}

/* ------------------------------------------------------------------ hoan vi */
void ascon_permutation(uint64_t s[5], int rounds, const linear_layer *L)
{
    for (int r = 12 - rounds; r < 12; r++) {
        s[2] ^= (uint64_t)(0xF0 - 0x0F * r);                      /* p_C */
        uint64_t x0 = s[0], x1 = s[1], x2 = s[2], x3 = s[3], x4 = s[4], t0, t1, t2, t3, t4;
        x0 ^= x4; x4 ^= x3; x2 ^= x1;                              /* p_S (bitslice) */
        t0 = ~x0 & x1; t1 = ~x1 & x2; t2 = ~x2 & x3; t3 = ~x3 & x4; t4 = ~x4 & x0;
        x0 ^= t1; x1 ^= t2; x2 ^= t3; x3 ^= t4; x4 ^= t0;
        x1 ^= x0; x0 ^= x4; x3 ^= x2; x2 = ~x2;
        s[0] = x0; s[1] = x1; s[2] = x2; s[3] = x3; s[4] = x4;
        layer_apply(L, s);                                         /* p_L */
    }
}

/* ------------------------------------------------------------------ Ascon-AEAD128 */
#define IV   0x00001000808C0001ULL
#define RATE 16
#define DSEP (1ULL << 63)

static void absorb_init(const linear_layer *L, uint64_t s[5], const uint8_t *ad, size_t adlen,
                        const uint8_t *nonce, const uint8_t *key)
{
    uint64_t k0 = load64(key), k1 = load64(key + 8);
    s[0] = IV; s[1] = k0; s[2] = k1; s[3] = load64(nonce); s[4] = load64(nonce + 8);
    ascon_permutation(s, 12, L);
    s[3] ^= k0; s[4] ^= k1;
    if (adlen) {
        while (adlen >= RATE) {
            s[0] ^= load64(ad); s[1] ^= load64(ad + 8);
            ascon_permutation(s, 8, L);
            ad += RATE; adlen -= RATE;
        }
        uint8_t blk[RATE] = { 0 };
        memcpy(blk, ad, adlen); blk[adlen] = 0x01;
        s[0] ^= load64(blk); s[1] ^= load64(blk + 8);
        ascon_permutation(s, 8, L);
    }
    s[4] ^= DSEP;
}

static void finalize(const linear_layer *L, uint64_t s[5], const uint8_t *key, uint8_t tag[16])
{
    uint64_t k0 = load64(key), k1 = load64(key + 8);
    s[2] ^= k0; s[3] ^= k1;
    ascon_permutation(s, 12, L);
    store64(tag, s[3] ^ k0); store64(tag + 8, s[4] ^ k1);
}

size_t ascon_aead_encrypt(const linear_layer *L, uint8_t *c, const uint8_t *m, size_t mlen,
                          const uint8_t *ad, size_t adlen, const uint8_t *nonce, const uint8_t *key)
{
    uint64_t s[5];
    size_t clen = mlen + ASCON_TAG_BYTES;
    absorb_init(L, s, ad, adlen, nonce, key);
    while (mlen >= RATE) {
        s[0] ^= load64(m); s[1] ^= load64(m + 8);
        store64(c, s[0]); store64(c + 8, s[1]);
        ascon_permutation(s, 8, L);
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

int ascon_aead_decrypt(const linear_layer *L, uint8_t *m, const uint8_t *c, size_t clen,
                       const uint8_t *ad, size_t adlen, const uint8_t *nonce, const uint8_t *key)
{
    if (clen < ASCON_TAG_BYTES) return -1;
    uint64_t s[5];
    size_t mlen = clen - ASCON_TAG_BYTES, total = mlen;
    uint8_t *m0 = m;
    const uint8_t *tag = c + mlen;
    absorb_init(L, s, ad, adlen, nonce, key);
    while (mlen >= RATE) {
        uint64_t c0 = load64(c), c1 = load64(c + 8);
        store64(m, s[0] ^ c0); store64(m + 8, s[1] ^ c1);
        s[0] = c0; s[1] = c1;
        ascon_permutation(s, 8, L);
        m += RATE; c += RATE; mlen -= RATE;
    }
    uint8_t blk[RATE];
    store64(blk, s[0]); store64(blk + 8, s[1]);
    for (size_t t = 0; t < mlen; t++) { m[t] = blk[t] ^ c[t]; blk[t] = c[t]; }
    blk[mlen] ^= 0x01;
    s[0] = load64(blk); s[1] = load64(blk + 8);
    uint8_t t2[ASCON_TAG_BYTES], diff = 0;
    finalize(L, s, key, t2);
    for (int t = 0; t < ASCON_TAG_BYTES; t++) diff |= t2[t] ^ tag[t];     /* so sanh thoi gian hang */
    if (diff) { memset(m0, 0, total); return -1; }                       /* khong tra ban ro khi xac thuc sai */
    return 0;
}
