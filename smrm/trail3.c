/*
 * trail3.c -- Vet vi sai / tuyen tinh 3 vong TOT NHAT trong lop vet co a1 <= h S-box hoat dong o vong
 *             giua, cho ham vong R = L o S (S-box Ascon tren 64 cot), mo hinh S-box CHINH XAC (DDT/LAT).
 *
 * Vet 3 vong: (X0 -S-> Y0) -L-> (X1 -S-> Y1) -L-> (X2 -S-> Y2).
 *   Vi sai : Y0 = L^{-1} X1 ,  X2 = L Y1 .  Trong so = sum w(X0->Y0) + w(X1->Y1) + w(X2->Y2).
 *            X0 tu do -> moi cot dong gop Win[y] = min_x w(x->y); Y2 tu do -> Wout[x] = min_y w(x->y).
 *   Tuyen tinh (mat na u vao S, v ra S): v0 = L^T u1 ; u2 = L^{-T} v1 (vi v1 = L^T u2).
 *            Win[v] = min_u w(u->v), Wout[u] = min_v w(u->v), w = -log2(corr^2).
 * Voi moi tap cot C (|C| = a, chua cot 0 nho bat bien xoay):
 *   f[X1] = sum_cot Win( Back X1 ),  g[Y1] = sum_cot Wout( Fwd Y1 )   (liet ke Gray 2^{5a})
 *   min_{X1,Y1} f[X1] + sum_t w(X1_t -> Y1_t) + g[Y1]  bang min-plus theo tung toa do (32^{a+1} moi buoc).
 * Ket qua: gia tri NHO NHAT CHINH XAC tren lop vet co a1 <= h  => CAN TREN cua trong so toi thieu 3 vong.
 * Hai muc tieu: (W) trong so ; (A) so S-box hoat dong (chi phi 1 moi S-box hoat dong, 0/inf cho chuyen tiep).
 *
 * Su dung: trail3 matrix.bin diff|lin h [ascon|chi]
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>

#define NW 5
#define WL 64
#define NB 320
#define INF 1000000

typedef struct { uint64_t w[NW]; } v320;
static inline uint64_t rotl64(uint64_t x, int r) { r &= 63; return r ? (x << r) | (x >> (64 - r)) : x; }
static inline int getbit(const v320 *v, int k) { return (v->w[k / WL] >> (k % WL)) & 1; }
static inline void setbit(v320 *v, int k) { v->w[k / WL] |= 1ULL << (k % WL); }

static void transpose(const v320 *cols, v320 *out) {
    memset(out, 0, sizeof(v320) * NB);
    for (int j = 0; j < NB; j++) for (int i = 0; i < NB; i++) if (getbit(&cols[j], i)) setbit(&out[i], j);
}
static int inverse(const v320 *cols, v320 *inv_cols) {
    static v320 rows[NB], aug[NB];
    transpose(cols, rows);
    memset(aug, 0, sizeof aug);
    for (int i = 0; i < NB; i++) setbit(&aug[i], i);
    int r = 0;
    for (int c = 0; c < NB; c++) {
        int piv = -1;
        for (int i = r; i < NB; i++) if (getbit(&rows[i], c)) { piv = i; break; }
        if (piv < 0) return 0;
        v320 t = rows[r]; rows[r] = rows[piv]; rows[piv] = t;
        t = aug[r]; aug[r] = aug[piv]; aug[piv] = t;
        for (int i = 0; i < NB; i++)
            if (i != r && getbit(&rows[i], c))
                for (int w = 0; w < NW; w++) { rows[i].w[w] ^= rows[r].w[w]; aug[i].w[w] ^= aug[r].w[w]; }
        r++;
    }
    transpose(aug, inv_cols);
    return 1;
}

/* ---------------- S-box Ascon ---------------- */
static int SB[32];
static void make_sbox_chi(void) {      /* chi cua Keccak tren 5 bit (Hsilu): y_j = x_j ^ (~x_{j+1} & x_{j+2}) */
    for (int v = 0; v < 32; v++) {
        int x[5], y[5], o = 0;
        for (int w = 0; w < 5; w++) x[w] = (v >> (4 - w)) & 1;
        for (int j = 0; j < 5; j++) y[j] = x[j] ^ ((!x[(j + 1) % 5]) & x[(j + 2) % 5]);
        for (int w = 0; w < 5; w++) o |= y[w] << (4 - w);
        SB[v] = o;
    }
}
static void make_sbox(void) {
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
static int wmid[32][32], Win[32], Wout[32];      /* trong so; INF = khong kha thi */
static void make_tables(int lin, int countmode) {
    for (int a = 0; a < 32; a++) for (int b = 0; b < 32; b++) {
        int w = INF;
        if (!lin) {
            int c = 0; for (int x = 0; x < 32; x++) c += (SB[x] ^ SB[x ^ a]) == b;
            if (c) { int l = 0; while ((1 << (l + 1)) <= c) l++; w = 5 - l; }      /* c = 2,4,8,32 -> 4,3,2,0 */
        } else {
            int c = 0; for (int x = 0; x < 32; x++) c += (__builtin_popcount(a & x) + __builtin_popcount(b & SB[x])) % 2 == 0;
            int l = c - 16; if (l < 0) l = -l;
            if (l) { int e = 0; while ((1 << (e + 1)) <= l) e++; w = 2 * (4 - e); }  /* |LAT| = 16,8,4,2 -> 0,2,4,6 */
        }
        if (countmode && w < INF) w = (a != 0);
        wmid[a][b] = w;
    }
    for (int y = 0; y < 32; y++) { Win[y] = INF; for (int x = 0; x < 32; x++) if (wmid[x][y] < Win[y]) Win[y] = wmid[x][y]; }
    for (int x = 0; x < 32; x++) { Wout[x] = INF; for (int y = 0; y < 32; y++) if (wmid[x][y] < Wout[x]) Wout[x] = wmid[x][y]; }
}
/* tong chi phi theo cot cua mot trang thai: bang[gia tri cot] */
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

typedef struct { int best; int a; int cs[3]; uint32_t X, Y; } res_t;

static void run(const v320 *Back, const v320 *Fwd, int h, res_t *out) {
    out->best = INF;
    for (int a = 1; a <= h; a++) {
        int nsets = 0; int (*sets)[3] = malloc(sizeof(int[3]) * 2100);
        int c[3] = {0, 1, 2};
        for (;;) {
            for (int t = 0; t < 3; t++) sets[nsets][t] = c[t];
            nsets++;
            if (a == 1) break;
            int t = a - 1;
            while (t >= 1 && c[t] == WL - a + t) t--;
            if (t < 1) break;
            c[t]++;
            for (int u = t + 1; u < a; u++) c[u] = c[u - 1] + 1;
        }
        uint32_t size = 1u << (5 * a);
        #pragma omp parallel
        {
            int *f = malloc(sizeof(int) * size), *g = malloc(sizeof(int) * size);
            int *h1 = malloc(sizeof(int) * size), *h2 = malloc(sizeof(int) * size);
            res_t loc; loc.best = INF;
            #pragma omp for schedule(dynamic, 8)
            for (int si = 0; si < nsets; si++) {
                const int *cs = sets[si];
                v320 bb[15], bf[15];
                for (int t = 0; t < a; t++) for (int i = 0; i < NW; i++) {
                    /* bit i cua cot t trong chi so: vi tri 5t + (4 - i) (bit 4 = tu 0) */
                    bb[5 * t + (4 - i)] = Back[i * WL + cs[t]];
                    bf[5 * t + (4 - i)] = Fwd[i * WL + cs[t]];
                }
                v320 yb, yf; memset(&yb, 0, sizeof yb); memset(&yf, 0, sizeof yf);
                f[0] = g[0] = 0;
                uint32_t gc = 0;
                for (uint32_t n = 1; n < size; n++) {
                    int q = __builtin_ctz(n); gc ^= 1u << q;
                    for (int w = 0; w < NW; w++) { yb.w[w] ^= bb[q].w[w]; yf.w[w] ^= bf[q].w[w]; }
                    f[gc] = colcost(&yb, Win);
                    g[gc] = colcost(&yf, Wout);
                }
                /* min-plus theo toa do: thay dan Y_t bang X_t */
                int *src = g, *dst = h1;
                for (int t = 0; t < a; t++) {
                    int sh = 5 * t;
                    for (uint32_t idx = 0; idx < size; idx++) {
                        if ((idx >> sh) & 31) continue;              /* idx voi toa do t = 0: duyet ca 32x32 */
                        for (int x = 0; x < 32; x++) {
                            int m = INF;
                            for (int y = 0; y < 32; y++) {
                                int v = src[idx | ((uint32_t)y << sh)] + wmid[x][y];
                                if (v < m) m = v;
                            }
                            dst[idx | ((uint32_t)x << sh)] = m;
                        }
                    }
                    src = dst; dst = (dst == h1) ? h2 : h1;
                }
                for (uint32_t X = 0; X < size; X++) {
                    int ok = 1;
                    for (int t = 0; t < a; t++) if (!((X >> (5 * t)) & 31)) { ok = 0; break; }
                    if (!ok) continue;
                    int v = f[X] + src[X];
                    if (v < loc.best) {
                        loc.best = v; loc.a = a; loc.X = X;
                        for (int t = 0; t < 3; t++) loc.cs[t] = cs[t];
                    }
                }
            }
            #pragma omp critical
            if (loc.best < out->best) *out = loc;
            free(f); free(g); free(h1); free(h2);
        }
        free(sets);
    }
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s matrix.bin diff|lin h [ascon|chi]\n", argv[0]); return 1; }
    int chi = argc > 4 && !strcmp(argv[4], "chi");
    int lin =!strcmp(argv[2], "lin"), h = atoi(argv[3]);
    static v320 M[NB], Mi[NB], Mt[NB], Mit[NB];
    FILE *fp = fopen(argv[1], "rb");
    if (!fp || fread(M, sizeof(v320), NB, fp) != NB) { fprintf(stderr, "read error\n"); return 1; }
    fclose(fp);
    if (!inverse(M, Mi)) { printf("SINGULAR\n"); return 2; }
    transpose(M, Mt); transpose(Mi, Mit);
    const v320 *Back = lin ? Mt : Mi, *Fwd = lin ? Mit : M;
    if (chi) make_sbox_chi(); else make_sbox();
    printf("S-box: %s\n", chi ?"chi (Keccak/Hsilu)" : "Ascon");
    double t0 = omp_get_wtime();
    for (int cm = 1; cm >= 0; cm--) {
        make_tables(lin, cm);
        res_t r; run(Back, Fwd, h, &r);
        printf("%s %s: min over trails with a1<=%d = %d  (attained at a1=%d, cols=%d,%d,%d, X1=%05x)\n",
               lin ? "LIN " : "DIFF", cm ? "active-sboxes" : "weight       ", h, r.best, r.a,
               r.cs[0], r.a > 1 ? r.cs[1] : -1, r.a > 2 ? r.cs[2] : -1, r.X);
        fflush(stdout);
    }
    printf("time %.1fs\n", omp_get_wtime() - t0);
    return 0;
}
