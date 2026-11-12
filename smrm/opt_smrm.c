/*
 * opt_smrm.c -- Toi uu cuc bo (leo doi co buoc ngang + khoi dong lai) cho ho SMRM  L = B . D . B^bT.
 *
 * Ham muc tieu (tu dien thu tu, lon hon la tot hon):
 *   score = ( min(m2, m3) , N_min[1] + N^{-1}_min[1] )
 *   m2 = min_{a<=2} (a + N_min[a]) tren L va L^{-1}   (can tren cua B_d, chinh xac neu <= 6)
 *   m3 = min_{a<=3} (...)                               (chinh xac neu <= 8; chi tinh khi m2 >= Tm3)
 * Vi B_l = B_d voi moi phan tu cua ho (Dinh ly 2), toi uu B_d dong nghia toi uu ca hai.
 * Thanh phan phu N_min[1] + N^{-1}_min[1] uu tien lan truyen manh cua 1 S-box hoat dong
 * (lien quan truc tiep vet 3 vong, xem Yu-Liu ToSC 2025 Muc 4).
 *
 * Dot bien: (1) doi r_l ; (2) doi cap (i_l, j_l) ; (3) doi d_w ; (4) hoan doi 2 phep lien ke.
 * Moi chuoi co seed rieng -> ket qua tai lap duoc.
 * Su dung: opt_smrm k chains iters seed Tm3
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
static int minact(const v320 *cols, int a, int stop) {
    int best = 1 << 20;
    int c[4] = {0, 1, 2, 3};
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
        if (a == 1) break;
        int t = a - 1;
        while (t >= 1 && c[t] == WL - a + t) t--;
        if (t < 1) break;
        c[t]++;
        for (int u = t + 1; u < a; u++) c[u] = c[u - 1] + 1;
    }
    return best;
}
static inline uint64_t rnd(uint64_t *s) {
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
static void mutate(smrm_t *p, uint64_t *s) {
    int kind = rnd(s) % 4, l = rnd(s) % p->k;
    if (kind == 0) p->r[l] = rnd(s) % WL;
    else if (kind == 1) { int i = rnd(s) % NW, j; do { j = rnd(s) % NW; } while (j == i); p->i[l] = i; p->j[l] = j; }
    else if (kind == 2) p->d[1 + rnd(s) % (NW - 1)] = rnd(s) % WL;
    else if (p->k > 1) {
        int m = (l + 1) % p->k;
        int ti = p->i[l], tj = p->j[l], tr = p->r[l];
        p->i[l] = p->i[m]; p->j[l] = p->j[m]; p->r[l] = p->r[m];
        p->i[m] = ti; p->j[m] = tj; p->r[m] = tr;
    }
}

typedef struct { int m; int tie; int n1, ni1; } score_t;

static score_t evaluate(const smrm_t *p, v320 *F, v320 *I, int Tm3) {
    score_t sc;
    build_cols(p, F, 0); build_cols(p, I, 1);
    sc.n1 = minact(F, 1, -1); sc.ni1 = minact(I, 1, -1);
    int m = 1 + (sc.n1 < sc.ni1 ? sc.n1 : sc.ni1);
    int v;
    v = 2 + minact(F, 2, -1); if (v < m) m = v;
    v = 2 + minact(I, 2, -1); if (v < m) m = v;
    if (m >= Tm3) {
        /* KHONG dung som: dung som chi tra ve mot gia tri < nguong (khong phai min) -> sai diem so */
        v = 3 + minact(F, 3, -1); if (v < m) m = v;
        v = 3 + minact(I, 3, -1); if (v < m) m = v;
    }
    sc.m = m; sc.tie = sc.n1 + sc.ni1;
    return sc;
}
static int better_eq(score_t a, score_t b) { return a.m > b.m || (a.m == b.m && a.tie >= b.tie); }

static void print_p(const char *tag, const smrm_t *p, score_t sc) {
    printf("%s m=%d N1=%d Ninv1=%d ops=", tag, sc.m, sc.n1, sc.ni1);
    for (int l = 0; l < p->k; l++) printf("%d,%d,%d%s", p->i[l], p->j[l], p->r[l], l < p->k - 1 ? ";" : "");
    printf(" d=%d,%d,%d,%d,%d\n", p->d[0], p->d[1], p->d[2], p->d[3], p->d[4]);
    fflush(stdout);
}

static void parse_smrm(smrm_t *p, const char *ops, const char *d) {
    char buf[1024]; strncpy(buf, ops, sizeof buf - 1); buf[sizeof buf - 1] = 0;
    p->k = 0;
    for (char *tok = strtok(buf, ";"); tok; tok = strtok(NULL, ";")) {
        sscanf(tok, "%d,%d,%d", &p->i[p->k], &p->j[p->k], &p->r[p->k]); p->k++;
    }
    sscanf(d, "%d,%d,%d,%d,%d", &p->d[0], &p->d[1], &p->d[2], &p->d[3], &p->d[4]);
}

int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[1], "eval")) {      /* che do kiem tra: opt_smrm eval ops d */
        smrm_t p; parse_smrm(&p, argv[2], argv[3]);
        v320 *F = malloc(sizeof(v320) * NB), *I = malloc(sizeof(v320) * NB);
        build_cols(&p, F, 0); build_cols(&p, I, 1);
        for (int a = 1; a <= 3; a++) printf("a=%d Nmin=%d Ninv_min=%d\n", a, minact(F, a, -1), minact(I, a, -1));
        score_t sc = evaluate(&p, F, I, 7);
        printf("score m=%d\n", sc.m);
        return 0;
    }
    if (argc < 6) { fprintf(stderr, "usage: %s k chains iters seed Tm3\n", argv[0]); return 1; }
    int k = atoi(argv[1]), chains = atoi(argv[2]);
    long iters = atol(argv[3]);
    uint64_t seed = strtoull(argv[4], 0, 10);
    int Tm3 = atoi(argv[5]);
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(dynamic, 1)
    for (int ch = 0; ch < chains; ch++) {
        uint64_t s = seed * 1000003ULL + ch;
        v320 *F = malloc(sizeof(v320) * NB), *I = malloc(sizeof(v320) * NB);
        smrm_t cur, best;
        random_smrm(&cur, k, &s);
        score_t sc = evaluate(&cur, F, I, Tm3), bsc = sc;
        best = cur;
        long stall = 0;
        for (long it = 0; it < iters; it++) {
            smrm_t cand = cur;
            int nm = 1 + rnd(&s) % 2;
            for (int t = 0; t < nm; t++) mutate(&cand, &s);
            score_t c = evaluate(&cand, F, I, Tm3);
            if (better_eq(c, sc)) { cur = cand; sc = c; }
            if (c.m > bsc.m || (c.m == bsc.m && c.tie > bsc.tie)) { best = cand; bsc = c; stall = 0; }
            else if (++stall > iters / 4) { random_smrm(&cur, k, &s); sc = evaluate(&cur, F, I, Tm3); stall = 0; }
        }
        #pragma omp critical
        {
            char tag[64]; snprintf(tag, sizeof tag, "BEST k=%d chain=%d", k, ch);
            print_p(tag, &best, bsc);
        }
        free(F); free(I);
    }
    printf("time %.1fs\n", omp_get_wtime() - t0);
    return 0;
}
