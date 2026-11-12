/*
 * lscommon.h -- Ham dung chung: trang thai 5x64, dai so tuyen tinh 320x320, S-box Ascon/chi,
 *               bang trong so DDT/LAT, ho SMRM, tim vet 3 vong voi a1 <= h (min-plus theo toa do).
 * Moi ham deu da duoc kiem chung chi tiet qua bn.c / trail3.c / verify_trail.py (docs/methods/README_methods.md).
 */
#ifndef LSCOMMON_H
#define LSCOMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define NW 5
#define WL 64
#define NB 320
#define MAXK 16
#define INF 1000000

typedef struct { uint64_t w[NW]; } v320;
typedef struct { int k; int i[MAXK], j[MAXK], r[MAXK]; int d[NW]; } smrm_t;

static inline uint64_t rotl64(uint64_t x, int r) { r &= 63; return r ? (x << r) | (x >> (64 - r)) : x; }
static inline int colw(const v320 *v) { return __builtin_popcountll(v->w[0] | v->w[1] | v->w[2] | v->w[3] | v->w[4]); }
static inline int getbit(const v320 *v, int k) { return (v->w[k / WL] >> (k % WL)) & 1; }
static inline void setbit(v320 *v, int k) { v->w[k / WL] |= 1ULL << (k % WL); }

static void transpose320(const v320 *cols, v320 *out) {
    memset(out, 0, sizeof(v320) * NB);
    for (int j = 0; j < NB; j++) for (int i = 0; i < NB; i++) if (getbit(&cols[j], i)) setbit(&out[i], j);
}

/* ---------------- SMRM ---------------- */
static void smrm_apply(const smrm_t *p, v320 *x) {
    for (int l = p->k - 1; l >= 0; l--) x->w[p->j[l]] ^= rotl64(x->w[p->i[l]], p->r[l]);
    for (int w = 0; w < NW; w++) x->w[w] = rotl64(x->w[w], p->d[w]);
    for (int l = 0; l < p->k; l++) x->w[p->i[l]] ^= rotl64(x->w[p->j[l]], p->r[l]);
}
static void smrm_apply_inv(const smrm_t *p, v320 *x) {
    for (int l = p->k - 1; l >= 0; l--) x->w[p->i[l]] ^= rotl64(x->w[p->j[l]], p->r[l]);
    for (int w = 0; w < NW; w++) x->w[w] = rotl64(x->w[w], 64 - p->d[w]);
    for (int l = 0; l < p->k; l++) x->w[p->j[l]] ^= rotl64(x->w[p->i[l]], p->r[l]);
}
static void smrm_cols(const smrm_t *p, v320 *cols, int inv) {
    for (int w = 0; w < NW; w++) {
        v320 x; memset(&x, 0, sizeof x);
        x.w[w] = 1;
        if (inv) smrm_apply_inv(p, &x); else smrm_apply(p, &x);
        for (int c = 0; c < WL; c++)
            for (int q = 0; q < NW; q++) cols[w * WL + c].w[q] = rotl64(x.w[q], c);
    }
}
static void parse_smrm(smrm_t *p, const char *ops, const char *d) {
    char buf[1024]; strncpy(buf, ops, sizeof buf - 1); buf[sizeof buf - 1] = 0;
    p->k = 0;
    for (char *tok = strtok(buf, ";"); tok; tok = strtok(NULL, ";")) {
        sscanf(tok, "%d,%d,%d", &p->i[p->k], &p->j[p->k], &p->r[p->k]); p->k++;
    }
    sscanf(d, "%d,%d,%d,%d,%d", &p->d[0], &p->d[1], &p->d[2], &p->d[3], &p->d[4]);
}
static void print_smrm(FILE *f, const smrm_t *p) {
    fprintf(f, "ops=");
    for (int l = 0; l < p->k; l++) fprintf(f, "%d,%d,%d%s", p->i[l], p->j[l], p->r[l], l < p->k - 1 ? ";" : "");
    fprintf(f, " d=%d,%d,%d,%d,%d", p->d[0], p->d[1], p->d[2], p->d[3], p->d[4]);
}

/* ---------------- so S-box hoat dong toi thieu theo so cot dau vao ---------------- */
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
            if (cw < best) { best = cw; if (best < stop) return best; }   /* chi dung de LOAI, khong de cham diem */
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

/* ---------------- S-box & bang trong so ---------------- */
static int SB[32];
static void make_sbox_ascon(void) {
    for (int v = 0; v < 32; v++) {
        int x[5]; for (int w = 0; w < 5; w++) x[w] = (v >> (4 - w)) & 1;
        x[0] ^= x[4]; x[4] ^= x[3]; x[2] ^= x[1];
        int t0 = (!x[0]) & x[1], t1 = (!x[1]) & x[2], t2 = (!x[2]) & x[3], t3 = (!x[3]) & x[4], t4 = (!x[4]) & x[0];
        x[0] ^= t1; x[1] ^= t2; x[2] ^= t3; x[3] ^= t4; x[4] ^= t0;
        x[1] ^= x[0]; x[0] ^= x[4]; x[3] ^= x[2]; x[2] = !x[2];
        int o = 0; for (int w = 0; w < 5; w++) o |= x[w] << (4 - w);
        SB[v] = o;
    }
}
static void make_sbox_chi(void) {
    for (int v = 0; v < 32; v++) {
        int x[5], y[5], o = 0;
        for (int w = 0; w < 5; w++) x[w] = (v >> (4 - w)) & 1;
        for (int j = 0; j < 5; j++) y[j] = x[j] ^ ((!x[(j + 1) % 5]) & x[(j + 2) % 5]);
        for (int w = 0; w < 5; w++) o |= y[w] << (4 - w);
        SB[v] = o;
    }
}
typedef struct { int mid[32][32], in[32], out[32]; } wtab_t;
static void make_wtab(wtab_t *T, int lin) {
    for (int a = 0; a < 32; a++) for (int b = 0; b < 32; b++) {
        int w = INF;
        if (!lin) {
            int c = 0; for (int x = 0; x < 32; x++) c += (SB[x] ^ SB[x ^ a]) == b;
            if (c) { int l = 0; while ((1 << (l + 1)) <= c) l++; w = 5 - l; }
        } else {
            int c = 0; for (int x = 0; x < 32; x++) c += (__builtin_popcount(a & x) + __builtin_popcount(b & SB[x])) % 2 == 0;
            int l = c - 16; if (l < 0) l = -l;
            if (l) { int e = 0; while ((1 << (e + 1)) <= l) e++; w = 2 * (4 - e); }
        }
        T->mid[a][b] = w;
    }
    for (int y = 0; y < 32; y++) { T->in[y] = INF; for (int x = 0; x < 32; x++) if (T->mid[x][y] < T->in[y]) T->in[y] = T->mid[x][y]; }
    for (int x = 0; x < 32; x++) { T->out[x] = INF; for (int y = 0; y < 32; y++) if (T->mid[x][y] < T->out[x]) T->out[x] = T->mid[x][y]; }
}
static inline int colcost(const v320 *v, const int *tab) {
    uint64_t any = v->w[0] | v->w[1] | v->w[2] | v->w[3] | v->w[4];
    int s = 0;
    while (any) {
        int p = __builtin_ctzll(any); any &= any - 1;
        int val = 0;
        for (int w = 0; w < 5; w++) val |= ((v->w[w] >> p) & 1) << (4 - w);
        s += tab[val];
    }
    return s;
}

/* Vet 3 vong toi uu voi a1 <= h (h <= 2 trong ham nay de nhanh; trail3.c ho tro h = 3).
 * Back: anh xa tu dau vao S vong giua ve dau ra S vong 0; Fwd: tu dau ra S vong giua toi dau vao S vong 2.
 * Bo nho dem f,g,h1,h2 kich thuoc >= 2^{5h}. */
static int best3(const v320 *Back, const v320 *Fwd, const wtab_t *T, int h, int *buf) {
    int best = INF;
    int *f = buf, *g = buf + 1024, *h1 = buf + 2048, *h2 = buf + 3072;
    for (int a = 1; a <= h; a++) {
        uint32_t size = 1u << (5 * a);
        for (int c1 = (a == 1 ? 0 : 1); c1 < (a == 1 ? 1 : WL); c1++) {
            int cs[2] = {0, c1};
            v320 bb[10], bf[10];
            for (int t = 0; t < a; t++) for (int i = 0; i < NW; i++) {
                bb[5 * t + (4 - i)] = Back[i * WL + cs[t]];
                bf[5 * t + (4 - i)] = Fwd[i * WL + cs[t]];
            }
            v320 yb, yf; memset(&yb, 0, sizeof yb); memset(&yf, 0, sizeof yf);
            f[0] = g[0] = 0;
            uint32_t gc = 0;
            for (uint32_t n = 1; n < size; n++) {
                int q = __builtin_ctz(n); gc ^= 1u << q;
                for (int w = 0; w < NW; w++) { yb.w[w] ^= bb[q].w[w]; yf.w[w] ^= bf[q].w[w]; }
                f[gc] = colcost(&yb, T->in);
                g[gc] = colcost(&yf, T->out);
            }
            int *src = g, *dst = h1;
            for (int t = 0; t < a; t++) {
                int sh = 5 * t;
                for (uint32_t idx = 0; idx < size; idx++) {
                    if ((idx >> sh) & 31) continue;
                    for (int x = 0; x < 32; x++) {
                        int m = INF;
                        for (int y = 0; y < 32; y++) {
                            int v = src[idx | ((uint32_t)y << sh)] + T->mid[x][y];
                            if (v < m) m = v;
                        }
                        dst[idx | ((uint32_t)x << sh)] = m;
                    }
                }
                src = dst; dst = (dst == h1) ? h2 : h1;
            }
            for (uint32_t X = 0; X < size; X++) {
                if (!(X & 31)) continue;
                if (a == 2 && !((X >> 5) & 31)) continue;
                int v = f[X] + src[X];
                if (v < best) best = v;
            }
        }
    }
    return best;
}
#endif
