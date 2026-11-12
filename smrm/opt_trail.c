/*
 * opt_trail.c -- Toi uu ho SMRM theo TRONG SO VET 3 VONG (muc tieu chinh) voi rang buoc so nhanh.
 *
 * score(L) = ( min(Wd, Wl) , Wd + Wl )   (tu dien thu tu, lon hon tot hon), voi dieu kien m2(L) >= Bmin
 *   Wd = trong so nho nhat cua vet VI SAI 3 vong co a1 <= 2 S-box hoat dong o vong giua (DDT chinh xac)
 *   Wl = tuong tu cho vet TUYEN TINH (LAT chinh xac)
 *   m2 = can tren so nhanh tu a <= 2 (L va L^{-1}); B_l = B_d theo Dinh ly 2.
 * Ly do khoa hoc: Yu-Liu (ToSC 2025, Muc 4) chi ra vet 3 vong toi uu thuong co rat it S-box o vong giua
 * (1-3); so nhanh 2 vong khong quyet dinh vet 3 vong. Wd, Wl o day la CAN TREN (vet thuc); ung vien cuoi
 * duoc danh gia lai voi a1 <= 3 (trail3.exe) va can duoi bang SAT (trail_sat.py + Kissat).
 *
 * Su dung: opt_trail k chains iters seed Bmin [ascon|chi] [Dmax]
 *   Dmax: rang buoc do sau logic (bien the "do tre thap"); do sau tinh nhu cost.py.
 *          opt_trail eval "<ops>" "<d>" [ascon|chi]
 */
#include "lscommon.h"
#include <omp.h>

static wtab_t TD, TL;
static int DMAX = 1000;          /* rang buoc do sau logic (so tang XOR2 tren duong gang) */

/* do sau logic cua chuong trinh SMRM: muc san sang cua tung tu, moi phep x_dst ^= rot(x_src) tang 1 */
static int smrm_depth(const smrm_t *p) {
    int lv[NW] = {0}, m = 0;
    for (int l = p->k - 1; l >= 0; l--) { int a = lv[p->j[l]], b = lv[p->i[l]]; lv[p->j[l]] = (a > b ? a : b) + 1; }
    for (int l = 0; l < p->k; l++) { int a = lv[p->i[l]], b = lv[p->j[l]]; lv[p->i[l]] = (a > b ? a : b) + 1; }
    for (int w = 0; w < NW; w++) if (lv[w] > m) m = lv[w];
    return m;
}

typedef struct { int ok, m2, wd, wl; } score_t;

static score_t evaluate(const smrm_t *p, int Bmin, v320 *F, v320 *I, v320 *Ft, v320 *It, int *buf) {
    score_t s;
    if (smrm_depth(p) > DMAX) { s.ok = 0; s.m2 = -500; s.wd = s.wl = 0; return s; }
    smrm_cols(p, F, 0); smrm_cols(p, I, 1);
    int m = 1 + minact(F, 1, -1), v;
    v = 1 + minact(I, 1, -1); if (v < m) m = v;
    v = 2 + minact(F, 2, -1); if (v < m) m = v;
    v = 2 + minact(I, 2, -1); if (v < m) m = v;
    s.m2 = m; s.ok = m >= Bmin; s.wd = s.wl = 0;
    if (!s.ok) return s;
    transpose320(F, Ft); transpose320(I, It);
    s.wd = best3(I, F, &TD, 2, buf);          /* vi sai: Back = L^-1, Fwd = L */
    s.wl = best3(Ft, It, &TL, 2, buf);        /* tuyen tinh: Back = L^T, Fwd = L^-T */
    return s;
}
static int key1(score_t s) { return s.ok ? (s.wd < s.wl ? s.wd : s.wl) : s.m2 - 1000; }
static int better_eq(score_t a, score_t b) {
    int ka = key1(a), kb = key1(b);
    return ka > kb || (ka == kb && a.wd + a.wl >= b.wd + b.wl);
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

int main(int argc, char **argv) {
    int chi = 0;
    if (argc >= 4 && !strcmp(argv[1], "eval")) {
        chi = argc > 4 && !strcmp(argv[4], "chi");
        if (chi) make_sbox_chi(); else make_sbox_ascon();
        make_wtab(&TD, 0); make_wtab(&TL, 1);
        smrm_t p; parse_smrm(&p, argv[2], argv[3]);
        v320 *F = malloc(sizeof(v320) * NB * 4); int *buf = malloc(sizeof(int) * 4096);
        score_t s = evaluate(&p, 0, F, F + NB, F + 2 * NB, F + 3 * NB, buf);
        printf("m2=%d Wd(a1<=2)=%d Wl(a1<=2)=%d depth=%d\n", s.m2, s.wd, s.wl, smrm_depth(&p));
        return 0;
    }
    if (argc < 6) { fprintf(stderr, "usage: %s k chains iters seed Bmin [ascon|chi]\n", argv[0]); return 1; }
    int k = atoi(argv[1]), chains = atoi(argv[2]), Bmin = atoi(argv[5]);
    long iters = atol(argv[3]);
    uint64_t seed = strtoull(argv[4], 0, 10);
    chi = argc > 6 && !strcmp(argv[6], "chi");
    if (argc > 7) DMAX = atoi(argv[7]);
    if (chi) make_sbox_chi(); else make_sbox_ascon();
    make_wtab(&TD, 0); make_wtab(&TL, 1);
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(dynamic, 1)
    for (int ch = 0; ch < chains; ch++) {
        uint64_t s = seed * 1000003ULL + ch;
        v320 *F = malloc(sizeof(v320) * NB * 4); int *buf = malloc(sizeof(int) * 4096);
        smrm_t cur, best;
        do random_smrm(&cur, k, &s); while (smrm_depth(&cur) > DMAX);   /* lay mau loai bo: khoi dau hop le */
        score_t sc = evaluate(&cur, Bmin, F, F + NB, F + 2 * NB, F + 3 * NB, buf), bsc = sc;
        best = cur;
        long stall = 0;
        for (long it = 0; it < iters; it++) {
            smrm_t cand = cur;
            int nm = 1 + rnd(&s) % 2;
            for (int t = 0; t < nm; t++) mutate(&cand, &s);
            score_t c = evaluate(&cand, Bmin, F, F + NB, F + 2 * NB, F + 3 * NB, buf);
            if (better_eq(c, sc)) { cur = cand; sc = c; }
            if (better_eq(c, bsc) && !(key1(c) == key1(bsc) && c.wd + c.wl == bsc.wd + bsc.wl)) { best = cand; bsc = c; stall = 0; }
            else if (++stall > iters / 3) { do random_smrm(&cur, k, &s); while (smrm_depth(&cur) > DMAX); sc = evaluate(&cur, Bmin, F, F + NB, F + 2 * NB, F + 3 * NB, buf); stall = 0; }
        }
        #pragma omp critical
        {
            printf("BEST k=%d chain=%d m=%d Wd=%d Wl=%d depth=%d ", k, ch, bsc.m2, bsc.wd, bsc.wl, smrm_depth(&best));
            print_smrm(stdout, &best); printf("\n"); fflush(stdout);
        }
        free(F); free(buf);
    }
    printf("time %.1fs\n", omp_get_wtime() - t0);
    return 0;
}
