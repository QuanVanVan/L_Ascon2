/*
 * bn.c -- Tinh CHINH XAC so nhanh vi sai / tuyen tinh (theo so S-box hoat dong = so cot khac 0)
 *         va mau hoat dong N_min[d] cua mot anh xa tuyen tinh M tren trang thai 5 x 64 bit.
 *
 * Dau vao: file nhi phan gom 320 cot, moi cot 5 tu uint64 little-endian (layers.write_matrix).
 * Gia thiet cau truc (duoc kiem tra lai trong chuong trinh): M giao hoan voi phep xoay dong thoi
 * ca 5 tu (moi khoi 64x64 la ma tran vong).
 *
 * THUAT TOAN (chung minh tinh dung xem docs/methods/README_methods.md, Muc "bn"):
 *  1. Do tinh bat bien xoay, moi x != 0 tuong duong (cung col(x), col(Mx)) voi mot x' co cot 0
 *     hoat dong. Vi vay chi duyet cac tap cot {0 = c0 < c1 < ... < c_{a-1}}.
 *  2. Voi moi tap cot, duyet TAT CA 2^{5a} gia tri tren 5a bit do bang ma Gray: moi buoc chi
 *     XOR mot vector anh M e_k (5 tu) -> O(1) moi buoc. Chi ghi nhan cac x co DUNG a cot khac 0.
 *     => N_min[a] = min { col(Mx) : col(x) = a } chinh xac voi a <= h.
 *  3. Bo de cat:  neu col(x) + col(Mx) < b thi col(x) < b/2 hoac col(Mx) < b/2.
 *     Dat m = min( min_a<=h (a + N_min^M[a]),  min_a<=h (a + N_min^{M^-1}[a]) ).
 *     Khi do  min(m, 2h+2) <= B(M) <= m ; neu m <= 2h+2 thi B(M) = m (chinh xac).
 *  4. Linear branch number: ap dung y het cho M^T va (M^T)^{-1} = (M^{-1})^T.
 *
 * Bien dich:  gcc -O3 -march=native -fopenmp bn.c -o bn
 * Su dung:    bn matrix.bin h
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>

#define NW 5
#define WL 64
#define NB 320
#define MAXH 5

typedef struct { uint64_t w[NW]; } v320;

static inline uint64_t rotl64(uint64_t x, int r) { r &= 63; return r ? (x << r) | (x >> (64 - r)) : x; }
static inline int colw(const v320 *v) {
    return __builtin_popcountll(v->w[0] | v->w[1] | v->w[2] | v->w[3] | v->w[4]);
}
static inline int getbit(const v320 *v, int k) { return (v->w[k / WL] >> (k % WL)) & 1; }
static inline void setbit(v320 *v, int k) { v->w[k / WL] |= 1ULL << (k % WL); }

/* ---------------- dai so tuyen tinh ---------------- */
static void transpose(const v320 *cols, v320 *out) {
    memset(out, 0, sizeof(v320) * NB);
    for (int j = 0; j < NB; j++)
        for (int i = 0; i < NB; i++)
            if (getbit(&cols[j], i)) setbit(&out[i], j);   /* cot i cua M^T = hang i cua M */
}

/* nghich dao: lam viec tren hang. Tra ve 0 neu suy bien. */
static int inverse(const v320 *cols, v320 *inv_cols) {
    static v320 rows[NB], aug[NB], tmp[NB];
    transpose(cols, rows);                       /* rows[i] = hang i cua M */
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
    /* aug chua cac HANG cua M^{-1}; chuyen sang cot */
    transpose(aug, tmp);
    memcpy(inv_cols, tmp, sizeof tmp);
    return 1;
}

static int check_rotation_invariance(const v320 *cols) {
    /* M e_{w, c} phai bang rot_c( M e_{w, 0} ) cho moi w, c */
    for (int w = 0; w < NW; w++)
        for (int c = 0; c < WL; c++) {
            const v320 *a = &cols[w * WL + c], *b = &cols[w * WL];
            for (int q = 0; q < NW; q++)
                if (a->w[q] != rotl64(b->w[q], c)) return 0;
        }
    return 1;
}

/* ---------------- liet ke theo tap cot ---------------- */
typedef struct { int nmin[MAXH + 1]; int nmax[MAXH + 1]; v320 wit[MAXH + 1]; } pattern_t;

static void activity(const v320 *cols, int h, pattern_t *res) {
    for (int a = 0; a <= MAXH; a++) { res->nmin[a] = 1 << 20; res->nmax[a] = -1; memset(&res->wit[a], 0, sizeof(v320)); }
    for (int a = 1; a <= h; a++) {
        /* sinh danh sach tap cot {0 < c1 < ... < c_{a-1}} */
        int nsets = 0;
        long long cap = 1;                       /* C(63, a-1) tap cot */
        for (int t = 1; t < a; t++) cap = cap * (WL - t) / t;
        int *sets = (int *)malloc(sizeof(int) * cap * MAXH);
        int c[MAXH]; c[0] = 0;
        if (a == 1) { sets[0] = 0; nsets = 1; }
        else {
            for (int t = 1; t < a; t++) c[t] = t;
            while (1) {
                for (int t = 0; t < a; t++) sets[nsets * MAXH + t] = c[t];
                nsets++;
                int t = a - 1;
                while (t >= 1 && c[t] == WL - a + t) t--;
                if (t < 1) break;
                c[t]++;
                for (int u = t + 1; u < a; u++) c[u] = c[u - 1] + 1;
            }
        }
        int gmin = 1 << 20, gmax = -1; v320 gwit; memset(&gwit, 0, sizeof gwit);
        #pragma omp parallel
        {
            int lmin = 1 << 20, lmax = -1; v320 lwit; memset(&lwit, 0, sizeof lwit);
            #pragma omp for schedule(dynamic, 16)
            for (int si = 0; si < nsets; si++) {
                const int *cs = &sets[si * MAXH];
                v320 basis[5 * MAXH];
                for (int t = 0; t < a; t++)
                    for (int i = 0; i < NW; i++) basis[5 * t + i] = cols[i * WL + cs[t]];
                v320 y; memset(&y, 0, sizeof y);
                unsigned grp[MAXH] = {0};
                int nz = 0;
                uint32_t g = 0;                       /* gia tri Gray hien tai */
                uint32_t lim = 1u << (5 * a);
                for (uint32_t n = 1; n < lim; n++) {
                    int q = __builtin_ctz(n);
                    g ^= 1u << q;
                    for (int w = 0; w < NW; w++) y.w[w] ^= basis[q].w[w];
                    int t = q / 5;
                    unsigned before = grp[t];
                    grp[t] ^= 1u << (q % 5);
                    nz += (grp[t] != 0) - (before != 0);
                    if (nz != a) continue;
                    int cw = colw(&y);
                    if (cw < lmin) {
                        lmin = cw;
                        memset(&lwit, 0, sizeof lwit);
                        for (int tt = 0; tt < a; tt++)
                            for (int i = 0; i < NW; i++)
                                if ((g >> (5 * tt + i)) & 1) lwit.w[i] |= 1ULL << cs[tt];
                    }
                    if (cw > lmax) lmax = cw;
                }
            }
            #pragma omp critical
            {
                if (lmin < gmin) { gmin = lmin; gwit = lwit; }
                if (lmax > gmax) gmax = lmax;
            }
        }
        res->nmin[a] = gmin; res->nmax[a] = gmax; res->wit[a] = gwit;
        free(sets);
    }
}

static void print_pattern(const char *name, const pattern_t *p, int h) {
    printf("%s Nmin:", name);
    for (int a = 1; a <= h; a++) printf(" %d", p->nmin[a]);
    printf("  Nmax:");
    for (int a = 1; a <= h; a++) printf(" %d", p->nmax[a]);
    printf("\n");
}

static void branch(const char *label, const pattern_t *f, const pattern_t *b, int h) {
    int m = 1 << 20, arg = 0, dir = 0;
    for (int a = 1; a <= h; a++) {
        if (a + f->nmin[a] < m) { m = a + f->nmin[a]; arg = a; dir = 0; }
        if (a + b->nmin[a] < m) { m = a + b->nmin[a]; arg = a; dir = 1; }
    }
    int lb = m < 2 * h + 2 ? m : 2 * h + 2;
    const v320 *wv = dir ? &b->wit[arg] : &f->wit[arg];
    printf("%s: %s %d  (bound interval [%d, %d]; witness side=%s, a=%d, x=",
           label, (lb == m ? "EXACT" : ">="), lb, lb, m, dir ? "output" : "input", arg);
    for (int w = 0; w < NW; w++) printf("%016llx%s", (unsigned long long)wv->w[w], w < NW - 1 ? ":" : ")\n");
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s matrix.bin h\n", argv[0]); return 1; }
    int h = atoi(argv[2]);
    if (h < 1 || h > MAXH) { fprintf(stderr, "h in [1,%d]\n", MAXH); return 1; }
    static v320 M[NB], Mi[NB], Mt[NB], Mti[NB];
    FILE *fp = fopen(argv[1], "rb");
    if (!fp || fread(M, sizeof(v320), NB, fp) != NB) { fprintf(stderr, "read error\n"); return 1; }
    fclose(fp);
    if (!check_rotation_invariance(M)) { fprintf(stderr, "ERROR: matrix is not rotation-invariant\n"); return 2; }
    if (!inverse(M, Mi)) { printf("SINGULAR\n"); return 3; }
    transpose(M, Mt);
    transpose(Mi, Mti);
    if (!check_rotation_invariance(Mi) || !check_rotation_invariance(Mt)) { fprintf(stderr, "ERROR: derived not rot-inv\n"); return 2; }
    pattern_t pM, pMi, pMt, pMti;
    double t0 = omp_get_wtime();
    activity(M, h, &pM); activity(Mi, h, &pMi);
    activity(Mt, h, &pMt); activity(Mti, h, &pMti);
    print_pattern("M      ", &pM, h);
    print_pattern("M^-1   ", &pMi, h);
    print_pattern("M^T    ", &pMt, h);
    print_pattern("M^-T   ", &pMti, h);
    branch("B_d", &pM, &pMi, h);
    branch("B_l", &pMt, &pMti, h);
    printf("time %.1fs\n", omp_get_wtime() - t0);
    return 0;
}
