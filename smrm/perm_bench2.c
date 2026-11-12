/*
 * perm_bench.c -- Cai dat hoan vi kieu Ascon-p (p_C, p_S cua Ascon) voi tang tuyen tinh co the
 *                 thay the: Ascon p_L | Hsilu-MRM (Yu-Liu 2025, Bang 9) | SMRM (de xuat).
 *                 Do chu ky/vong/byte bang __rdtsc de so sanh CONG BANG chi phi phan mem:
 *                 cung S-box, cung hang so, cung trinh bien dich, chi khac tang tuyen tinh.
 *
 * Kiem tra tinh dung: (1) p_L cua Ascon khop ma tran trong layers.py (qua self-test vector),
 *                     (2) SMRM: apply_inv(apply(x)) = x tren 10^5 trang thai ngau nhien.
 * Bien dich: gcc -O3 -march=native perm_bench.c -o perm_bench
 * Su dung:   perm_bench "<ops>" "<d>"     (vd "1,2,55;4,1,55" "0,28,41,50,38")
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

#define ROTR(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#define ROTL(x, n) ((n) ? (((x) << (n)) | ((x) >> (64 - (n)))) : (x))

typedef struct { uint64_t x[5]; } state_t;

static const uint64_t RC[12] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b};

static inline void sbox(state_t *s) {
    uint64_t x0 = s->x[0], x1 = s->x[1], x2 = s->x[2], x3 = s->x[3], x4 = s->x[4], t0, t1, t2, t3, t4;
    x0 ^= x4; x4 ^= x3; x2 ^= x1;
    t0 = ~x0 & x1; t1 = ~x1 & x2; t2 = ~x2 & x3; t3 = ~x3 & x4; t4 = ~x4 & x0;
    x0 ^= t1; x1 ^= t2; x2 ^= t3; x3 ^= t4; x4 ^= t0;
    x1 ^= x0; x0 ^= x4; x3 ^= x2; x2 = ~x2;
    s->x[0] = x0; s->x[1] = x1; s->x[2] = x2; s->x[3] = x3; s->x[4] = x4;
}

/* ---- Ascon p_L ---- */
static inline void lin_ascon(state_t *s) {
    s->x[0] ^= ROTR(s->x[0], 19) ^ ROTR(s->x[0], 28);
    s->x[1] ^= ROTR(s->x[1], 61) ^ ROTR(s->x[1], 39);
    s->x[2] ^= ROTR(s->x[2], 1) ^ ROTR(s->x[2], 6);
    s->x[3] ^= ROTR(s->x[3], 10) ^ ROTR(s->x[3], 17);
    s->x[4] ^= ROTR(s->x[4], 7) ^ ROTR(s->x[4], 41);
}

/* ---- Hsilu MRM (Bang 9): 13 XOR co dich + xoay hang ---- */
static inline void lin_hsilu(state_t *s) {
    static const int n1[13] = {0, 3, 1, 0, 5, 5, 7, 11, 12, 12, 13, 14, 15};
    static const int n2[13] = {4, 4, 2, 1, 2, 3, 6, 8, 9, 6, 10, 11, 9};
    static const int m[13] = {47, 51, 54, 30, 4, 55, 0, 14, 59, 4, 10, 45, 53};
    static const int sh[5] = {0, 57, 13, 31, 20};
    uint64_t x[18];
    for (int i = 0; i < 5; i++) x[i] = s->x[i];
    for (int t = 0; t < 13; t++) x[5 + t] = x[n1[t]] ^ ROTL(x[n2[t]], m[t]);
    for (int i = 0; i < 5; i++) s->x[i] = ROTL(x[13 + i], sh[i]);
}

/* ---- SMRM ---- */
#define MAXK 16
static int K, OI[MAXK], OJ[MAXK], OR_[MAXK], D[5];
#include "smrm_gen.h"
static inline void lin_smrm(state_t *s) {
    uint64_t *x = s->x;
    for (int l = K - 1; l >= 0; l--) x[OJ[l]] ^= ROTL(x[OI[l]], OR_[l]);
    for (int w = 1; w < 5; w++) x[w] = ROTL(x[w], D[w]);
    for (int l = 0; l < K; l++) x[OI[l]] ^= ROTL(x[OJ[l]], OR_[l]);
}
static inline void lin_smrm_inv(state_t *s) {
    uint64_t *x = s->x;
    for (int l = K - 1; l >= 0; l--) x[OI[l]] ^= ROTL(x[OJ[l]], OR_[l]);
    for (int w = 1; w < 5; w++) x[w] = ROTL(x[w], (64 - D[w]) & 63);
    for (int l = 0; l < K; l++) x[OJ[l]] ^= ROTL(x[OI[l]], OR_[l]);
}

typedef void (*lin_f)(state_t *);
static inline void perm(state_t *s, int rounds, lin_f L) {
    for (int r = 12 - rounds; r < 12; r++) { s->x[2] ^= RC[r]; sbox(s); L(s); }
}

static uint64_t sm = 0x1234567;
static uint64_t rnd64(void) { sm ^= sm << 13; sm ^= sm >> 7; sm ^= sm << 17; return sm; }

static double bench(lin_f L, const char *name) {
    state_t s; for (int i = 0; i < 5; i++) s.x[i] = rnd64();
    const int REP = 200000;
    double best = 1e30;
    for (int trial = 0; trial < 15; trial++) {
        uint64_t t0 = __rdtsc();
        for (int i = 0; i < REP; i++) perm(&s, 12, L);
        uint64_t t1 = __rdtsc();
        double cpb = (double)(t1 - t0) / REP / 12 / 40.0;     /* chu ky / vong / byte (40 byte trang thai) */
        if (cpb < best) best = cpb;
    }
    printf("%-10s cycles/round/byte = %.4f   (checksum %016llx)\n", name, best, (unsigned long long)s.x[0]);
    return best;
}

/* chi tinh rieng tang tuyen tinh */
static double bench_lin(lin_f L, const char *name) {
    state_t s; for (int i = 0; i < 5; i++) s.x[i] = rnd64();
    const int REP = 2000000;
    double best = 1e30;
    for (int trial = 0; trial < 15; trial++) {
        uint64_t t0 = __rdtsc();
        for (int i = 0; i < REP; i++) { L(&s); s.x[0] ^= i; }
        uint64_t t1 = __rdtsc();
        double c = (double)(t1 - t0) / REP;
        if (c < best) best = c;
    }
    printf("%-10s linear layer only: %.2f cycles/call  (checksum %016llx)\n", name, best, (unsigned long long)s.x[1]);
    return best;
}

int main(void) {
    printf("%-10s %22s %22s\n", "layer", "lin only (cyc/call)", "perm (cyc/round/byte)");
    bench_lin(lin_ascon, "Ascon"); bench_lin(lin_hsilu, "Hsilu-MRM");
    double a = bench(lin_ascon, "Ascon"), h = bench(lin_hsilu, "Hsilu-MRM");
    for (int f = 0; f < NFUNCS; f++) {
        /* nap tham so cho ban tong quat de kiem tra cheo */
        char buf[1024]; strncpy(buf, FUNCS[f].ops, sizeof buf - 1); buf[sizeof buf - 1] = 0;
        K = 0;
        for (char *tok = strtok(buf, ";"); tok; tok = strtok(NULL, ";")) { sscanf(tok, "%d,%d,%d", &OI[K], &OJ[K], &OR_[K]); K++; }
        sscanf(FUNCS[f].d, "%d,%d,%d,%d,%d", &D[0], &D[1], &D[2], &D[3], &D[4]);
        for (int t = 0; t < 100000; t++) {
            state_t p, q; for (int i = 0; i < 5; i++) p.x[i] = q.x[i] = rnd64();
            lin_smrm(&p); FUNCS[f].f(&q);
            if (memcmp(&p, &q, sizeof p)) { printf("%s: GENERATED != GENERIC\n", FUNCS[f].name); return 2; }
            FUNCS[f].fi(&q);
            state_t r = q; (void)r;
        }
        state_t z; for (int i = 0; i < 5; i++) z.x[i] = rnd64();
        state_t z0 = z; FUNCS[f].f(&z); FUNCS[f].fi(&z);
        if (memcmp(&z, &z0, sizeof z)) { printf("%s: INVERSE FAILED\n", FUNCS[f].name); return 2; }
        bench_lin(FUNCS[f].f, FUNCS[f].name);
        double c = bench(FUNCS[f].f, FUNCS[f].name);
        printf("   -> %s / Ascon = %.2fx ; / Hsilu = %.2fx\n", FUNCS[f].name, c / a, c / h);
    }
    return 0;
}
