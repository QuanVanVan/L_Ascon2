/*
 * search_smrm.c -- Tim kiem ngau nhien co loc theo tang cho ho SMRM:  L = B . D . B^bT
 *
 *   B^bT : for l = k..1 : x[j_l] ^= rotl(x[i_l], r_l)
 *   D    : x[w] = rotl(x[w], d_w)            (d_0 = 0 khong mat tong quat)
 *   B    : for l = 1..k : x[i_l] ^= rotl(x[j_l], r_l)
 *
 * Chi phi 2k XOR 64-bit. Vi L^bT = L nen B_l(L) = B_d(L) (Dinh ly 2, docs/methods/README_methods.md),
 * do do bo loc chi can tinh B_d; B_l duoc kiem tra doc lap sau bang bn.exe tren ma tran chuyen vi.
 *
 * Loc theo tang (moi tang la dieu kien CAN de B_d >= T):
 *   Tang A: a <= 2 cot hoat dong, ca hai chieu L va L^{-1}  -> m2 (= B_d neu m2 <= 6)
 *   Tang B: a = 3 (dung som khi gap gia tri < T)            -> B_d >= min(m3, 8), B_d <= m3
 * Ung vien qua tang B duoc in ra kem N_min[1], N^{-1}_min[1] (chi so "nghich dao phuc tap"
 * cua Yu-Liu) de xep hang.
 *
 * Su dung: search_smrm k ncand seed T
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>

#define NW 5
#define WL 64
#define NB 320
#define MAXK 16

typedef struct { uint64_t w[NW]; } v320;
typedef struct { int k; int i[MAXK], j[MAXK], r[MAXK]; int d[NW]; } smrm_t;

static inline uint64_t rotl64(uint64_t x, int r) { r &= 63; return r ? (x << r) | (x >> (64 - r)) : x; }
static inline int colw(const v320 *v) { return __builtin_popcountll(v->w[0] | v->w[1] | v->w[2] | v->w[3] | v->w[4]); }

static void apply(const smrm_t *p, v320 *x) {
    for (int l = p->k - 1; l >= 0; l--) x->w[p->j[l]] ^= rotl64(x->w[p->i[l]], p->r[l]);
    for (int w = 0; w < NW; w++) x->w[w] = rotl64(x->w[w], p->d[w]);
    for (int l = 0; l < p->k; l++) x->w[p->i[l]] ^= rotl64(x->w[p->j[l]], p->r[l]);
}
static void apply_inv(const smrm_t *p, v320 *x) {
    for (int l = p->k - 1; l >= 0; l--) x->w[p->i[l]] ^= rotl64(x->w[p->j[l]], p->r[l]);
    for (int w = 0; w < NW; w++) x->w[w] = rotl64(x->w[w], 64 - p->d[w]);
    for (int l = 0; l < p->k; l++) x->w[p->j[l]] ^= rotl64(x->w[p->i[l]], p->r[l]);
}

static void build_cols(const smrm_t *p, v320 *cols, int inv) {
    for (int w = 0; w < NW; w++) {
        v320 x; memset(&x, 0, sizeof x);
        x.w[w] = 1;
        if (inv) apply_inv(p, &x); else apply(p, &x);
        for (int c = 0; c < WL; c++)
            for (int q = 0; q < NW; q++) cols[w * WL + c].w[q] = rotl64(x.w[q], c);
    }
}

/* min col(Mx) tren cac x co DUNG a cot hoat dong (cot 0 hoat dong); dung som neu < stop */
static int minact(const v320 *cols, int a, int stop) {
    int best = 1 << 20;
    int c[4] = {0, 1, 2, 3};
    if (a > 3) return -1;
    for (;;) {
        v320 basis[15];
        for (int t = 0; t < a; t++)
            for (int i = 0; i < NW; i++) basis[5 * t + i] = cols[i * WL + c[t]];
        v320 y; memset(&y, 0, sizeof y);
        unsigned grp[3] = {0, 0, 0};
        int nz = 0;
        uint32_t lim = 1u << (5 * a);
        for (uint32_t n = 1; n < lim; n++) {
            int q = __builtin_ctz(n);
            for (int w = 0; w < NW; w++) y.w[w] ^= basis[q].w[w];
            int t = q / 5;
            unsigned before = grp[t];
            grp[t] ^= 1u << (q % 5);
            nz += (grp[t] != 0) - (before != 0);
            if (nz != a) continue;
            int cw = colw(&y);
            if (cw < best) { best = cw; if (best < stop) return best; }
        }
        /* tap cot tiep theo */
        if (a == 1) break;
        int t = a - 1;
        while (t >= 1 && c[t] == WL - a + t) t--;
        if (t < 1) break;
        c[t]++;
        for (int u = t + 1; u < a; u++) c[u] = c[u - 1] + 1;
    }
    return best;
}

static uint64_t rng_state;
static inline uint64_t rnd(uint64_t *s) {     /* splitmix64 -- tat dinh theo seed */
    uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static void random_smrm(smrm_t *p, int k, uint64_t *s) {
    p->k = k;
    for (int l = 0; l < k; l++) {
        int i = rnd(s) % NW, j;
        do { j = rnd(s) % NW; } while (j == i);
        p->i[l] = i; p->j[l] = j; p->r[l] = rnd(s) % WL;
    }
    p->d[0] = 0;
    for (int w = 1; w < NW; w++) p->d[w] = rnd(s) % WL;
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s k ncand seed T\n", argv[0]); return 1; }
    int k = atoi(argv[1]);
    long long ncand = atoll(argv[2]);
    uint64_t seed = strtoull(argv[3], 0, 10);
    int T = atoi(argv[4]);
    long long hist[16] = {0};
    long long passA = 0, passB = 0;
    double t0 = omp_get_wtime();
    #pragma omp parallel
    {
        v320 *F = malloc(sizeof(v320) * NB), *I = malloc(sizeof(v320) * NB);
        long long lh[16] = {0};
        #pragma omp for schedule(dynamic, 64) reduction(+:passA, passB)
        for (long long n = 0; n < ncand; n++) {
            uint64_t s = seed * 0x100000001B3ULL + (uint64_t)n;    /* moi ung vien co seed rieng -> tai lap */
            smrm_t p; random_smrm(&p, k, &s);
            build_cols(&p, F, 0); build_cols(&p, I, 1);
            /* Tang A: a = 1, 2 (khong dung som -> dung cho thong ke) */
            int nf1 = minact(F, 1, -1), ni1 = minact(I, 1, -1);
            int m2 = 1 + (nf1 < ni1 ? nf1 : ni1);
            int nf2 = minact(F, 2, -1), ni2 = minact(I, 2, -1);
            if (2 + nf2 < m2) m2 = 2 + nf2;
            if (2 + ni2 < m2) m2 = 2 + ni2;
            lh[m2 < 15 ? m2 : 15]++;
            if (m2 < T) continue;
            passA++;
            /* Tang B: a = 3 */
            int nf3 = minact(F, 3, T - 3);
            if (3 + nf3 < T) continue;
            int ni3 = minact(I, 3, T - 3);
            if (3 + ni3 < T) continue;
            int m3 = m2;
            if (3 + nf3 < m3) m3 = 3 + nf3;
            if (3 + ni3 < m3) m3 = 3 + ni3;
            passB++;
            #pragma omp critical
            {
                printf("CAND k=%d n=%lld Bd_in=[%d,%d] N1=%d Ninv1=%d N2=%d Ninv2=%d N3=%d Ninv3=%d ops=",
                       k, n, m3 < 8 ? m3 : 8, m3, nf1, ni1, nf2, ni2, nf3, ni3);
                for (int l = 0; l < k; l++) printf("%d,%d,%d%s", p.i[l], p.j[l], p.r[l], l < k - 1 ? ";" : "");
                printf(" d=%d,%d,%d,%d,%d\n", p.d[0], p.d[1], p.d[2], p.d[3], p.d[4]);
                fflush(stdout);
            }
        }
        #pragma omp critical
        for (int b = 0; b < 16; b++) hist[b] += lh[b];
        free(F); free(I);
    }
    printf("SUMMARY k=%d ncand=%lld T=%d passA=%lld passB=%lld time=%.1fs\nHIST(upper bound m2 from a<=2):",
           k, ncand, T, passA, passB, omp_get_wtime() - t0);
    for (int b = 0; b < 16; b++) if (hist[b]) printf(" %d:%lld", b, hist[b]);
    printf("\n");
    return 0;
}
