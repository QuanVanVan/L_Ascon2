/*
DATN extension of AsconTrailTool: exhaustive generation of 2-round trail cores for ANY linear
layer that commutes with the simultaneous rotation of the 5 rows (operation "generic2r").

Why a new generator: the tree search of the original tool (treeCanonical.cpp, asconColScore.cpp)
is specific to Ascon: it relies on p_L acting on every row separately with 3-term polynomials
(stable bits, "alternative representation", per-row static masks). For layers that mix rows
(MRM, SMRM, ...) the image of a single bit touches 3-5 rows with a span of ~55 columns, so those
pruning techniques give no stable bits and the original code is simply not applicable.

Method (rigorous, exhaustive). A 2-round trail core is (a, b) with b = lambda(a), where
lambda = p_L (differential) or p_L^T (linear), weight w0 = wrev(a), w1 = w(b). Given the weighting
(ca, cb) in {(1,1), (1,2), (2,1)} and the target T we want ALL canonical cores with
ca*w0 + cb*w1 <= T. Every active column contributes at least 2 to w0 (resp. w1), hence the numbers
of active columns (na, nb) satisfy 2*(ca*na + cb*nb) <= T: a finite region R.
We choose hA, hB such that every point of R has na <= hA or nb <= hB.
  side A: for every canonical set S of na <= hA columns (S contains 0 and is the lexicographically
          smallest of its translates containing 0) we look for all a with support exactly S and
          nb <= t := max{nb : (na, nb) in R}. Split the 64 columns of b into t+1 groups. Since b has
          at most t active columns, at least one group of b is entirely zero (pigeonhole), so a lies
          in the kernel of the linear map "a -> b restricted to that group". For every group we
          compute the kernel (Gaussian elimination over F2, unknowns = the 5*na bits of a on S) and
          enumerate it; a solution is reported only for the FIRST zero group of its b (no duplicates).
  side B: the same with the roles of a and b exchanged (unknowns = b on S, a = lambda^{-1}(b)),
          restricted to na > hA so that the two sides are disjoint.
Each solution is checked against the exact weights (tables of the original tool), and translates
by the stabilizer of S are removed, so every class of cores modulo column rotation is output
exactly once. The output format is the one of the original tool, hence the files can be extended
with the unchanged extension code (extendWW / extend).
Correctness is validated by comparing with the original tree search on Ascon (scripts/validate_*).
*/

#include "generic2R.h"
#include "genericLayer.h"
#include "trailExtension.h"

#include <algorithm>
#include <cstdlib>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <omp.h>

using namespace std;

namespace {

inline uint64_t rotl64(uint64_t x, int r) { r &= 63; return r ? ((x << r) | (x >> (64 - r))) : x; }

struct Images {                      // imgs[i][k] = row k of lambda(e_{i,0})
    uint64_t f[5][5];
    uint64_t inv[5][5];
};

Images computeImages()
{
    Images im;
    for (int i = 0; i < 5; i++) {
        State e; e.setRow(i, 1);
        State a = e, b = e;
#if DTS == 1
        g_layer.applyForward(a);
        g_layer.applyInverse(b);
#else
        g_layer.applyTranspose(a);
        g_layer.applyInverseTranspose(b);
#endif
        for (int k = 0; k < 5; k++) { im.f[i][k] = a.getRow(k); im.inv[i][k] = b.getRow(k); }
    }
    // translation invariance check: lambda(rot_t(e_i)) == rot_t(lambda(e_i)) for all t
    for (int i = 0; i < 5; i++)
        for (int t = 1; t < 64; t++) {
            State e; e.setRow(i, (uint64_t)1 << t);
#if DTS == 1
            g_layer.applyForward(e);
#else
            g_layer.applyTranspose(e);
#endif
            for (int k = 0; k < 5; k++)
                if (e.getRow(k) != rotl64(im.f[i][k], t))
                    throw runtime_error("linear layer is not invariant under column rotation");
        }
    return im;
}

inline unsigned colValue(const uint64_t r[5], int c)  // MSB = row 0, as in State::getColumn
{
    return (unsigned)((((r[0] >> c) & 1) << 4) | (((r[1] >> c) & 1) << 3) | (((r[2] >> c) & 1) << 2) | (((r[3] >> c) & 1) << 1) | ((r[4] >> c) & 1));
}

struct Weights {
    unsigned w[32], minRev[32];
};

struct Config {
    int ca, cb, T;
    int hA, hB;
    int maxNa, maxNb;
    int tA(int na) const { int v = (T - 2 * ca * na); return v < 0 ? -1 : v / (2 * cb); }   // max nb
    int tB(int nb) const { int v = (T - 2 * cb * nb); return v < 0 ? -1 : v / (2 * ca); }   // max na
};

struct Stats {
    atomic<long long> sets{0}, systems{0}, kernelVectors{0}, cores{0};
    atomic<int> maxKernelDim{0};
};

// all canonical column sets of size n (contain 0, smallest translate containing 0), with stabilizer
void canonicalCheck(const int* S, int n, bool& canonical, vector<int>& stab)
{
    canonical = true; stab.clear(); stab.push_back(0);
    int tmp[64];
    for (int idx = 1; idx < n; idx++) {
        int t = S[idx];
        for (int q = 0; q < n; q++) tmp[q] = (S[q] - t) & 63;
        sort(tmp, tmp + n);
        int cmp = 0;
        for (int q = 0; q < n && cmp == 0; q++) cmp = (tmp[q] < S[q]) ? -1 : (tmp[q] > S[q] ? 1 : 0);
        if (cmp < 0) { canonical = false; return; }
        if (cmp == 0) stab.push_back(t);
    }
}

// Gaussian elimination: eq = equations over nu unknowns (bit masks). Returns kernel basis.
int kernelBasis(vector<uint64_t>& eq, int nu, uint64_t* basis)
{
    int rank = 0;
    int pivotCol[64];
    for (int col = 0; col < nu && rank < (int)eq.size(); col++) {
        uint64_t bit = (uint64_t)1 << col;
        int p = -1;
        for (int r = rank; r < (int)eq.size(); r++) if (eq[r] & bit) { p = r; break; }
        if (p < 0) continue;
        swap(eq[rank], eq[p]);
        for (int r = 0; r < (int)eq.size(); r++) if (r != rank && (eq[r] & bit)) eq[r] ^= eq[rank];
        pivotCol[rank] = col;
        rank++;
    }
    uint64_t pivotMask = 0;
    for (int r = 0; r < rank; r++) pivotMask |= (uint64_t)1 << pivotCol[r];
    int d = 0;
    for (int f = 0; f < nu; f++) {
        if ((pivotMask >> f) & 1) continue;
        uint64_t v = (uint64_t)1 << f;
        for (int r = 0; r < rank; r++) if (eq[r] & ((uint64_t)1 << f)) v |= (uint64_t)1 << pivotCol[r];
        basis[d++] = v;
    }
    return d;
}

class Worker {
public:
    const Images& im;
    const Weights& wt;
    const Config& cf;
    Stats& st;
    ostringstream out;
    Worker(const Images& a, const Weights& b, const Config& c, Stats& s) : im(a), wt(b), cf(c), st(s)
    {
        const char* e = getenv("GEN2R_D0");
        if (e) D0 = atoi(e);
    }

private:
    // current column set
    const int* S; int n; const vector<int>* stab; int side; int t;
    struct Level { int g; uint64_t groupMask[64]; int chosen; };
    vector<Level> levels;
    int D0 = 10;                     // enumerate directly when the subspace has dimension <= D0 (env GEN2R_D0)
    static const int MAXLEVELS = 8;

    // Recursive refinement. The subspace is spanned by xs[0..d-1] (vectors over the unknowns) whose
    // images on the other side are ims[0..d-1]; all its vectors have zero image on the columns Z.
    // Pigeonhole: the at most t active columns of the image lie in U (touched columns not in Z);
    // splitting U into t+1 groups, at least one group is zero. A vector is kept in the branch of the
    // FIRST zero group of every level, so it is produced exactly once.
    void refine(int d, const uint64_t* xs, const uint64_t (*ims)[5], uint64_t Z)
    {
        if (d == 0) return;
        uint64_t U = 0;
        for (int j = 0; j < d; j++) U |= ims[j][0] | ims[j][1] | ims[j][2] | ims[j][3] | ims[j][4];
        U &= ~Z;
        int nU = __builtin_popcountll(U);
        if (d <= D0 || nU < t + 1 || (int)levels.size() >= MAXLEVELS) { enumerate(d, xs, ims); return; }
        Level L; L.g = t + 1; L.chosen = -1;
        {   // contiguous groups in the increasing order of the columns of U
            int cols[64], m = 0; uint64_t u = U;
            while (u) { cols[m++] = __builtin_ctzll(u); u &= u - 1; }
            for (int q = 0; q < L.g; q++) {
                L.groupMask[q] = 0;
                for (int z = (q * m) / L.g; z < ((q + 1) * m) / L.g; z++) L.groupMask[q] |= (uint64_t)1 << cols[z];
            }
        }
        levels.push_back(L);
        vector<uint64_t> eq;
        uint64_t coef[64];
        uint64_t nxs[64], nims[64][5];
        for (int q = 0; q < L.g; q++) {
            levels.back().chosen = q;
            eq.clear();
            uint64_t gm = levels.back().groupMask[q];
            while (gm) {
                int c = __builtin_ctzll(gm); gm &= gm - 1;
                for (int k = 0; k < 5; k++) {
                    uint64_t e = 0;
                    for (int j = 0; j < d; j++) e |= ((ims[j][k] >> c) & 1) << j;
                    if (e) eq.push_back(e);
                }
            }
            int nd = kernelBasis(eq, d, coef);
            st.systems++;
            for (int e = 0; e < nd; e++) {
                nxs[e] = 0; for (int k = 0; k < 5; k++) nims[e][k] = 0;
                uint64_t cc = coef[e];
                while (cc) { int j = __builtin_ctzll(cc); cc &= cc - 1; nxs[e] ^= xs[j]; for (int k = 0; k < 5; k++) nims[e][k] ^= ims[j][k]; }
            }
            refine(nd, nxs, nims, Z | levels.back().groupMask[q]);
        }
        levels.pop_back();
    }

    void enumerate(int d, const uint64_t* xs, const uint64_t (*ims)[5])
    {
        if (d > 40) throw runtime_error("kernel dimension too large");
        int prev = st.maxKernelDim.load();
        while (d > prev && !st.maxKernelDim.compare_exchange_weak(prev, d)) {}
        uint64_t x = 0, other[5] = { 0, 0, 0, 0, 0 };
        long long cnt = 0;
        for (uint64_t gidx = 1; gidx < ((uint64_t)1 << d); gidx++) {
            int j = __builtin_ctzll(gidx);
            x ^= xs[j];
            for (int k = 0; k < 5; k++) other[k] ^= ims[j][k];
            cnt++;
            bool full = true;
            for (int s = 0; s < n && full; s++) if (((x >> (5 * s)) & 31) == 0) full = false;
            if (!full) continue;
            uint64_t act = other[0] | other[1] | other[2] | other[3] | other[4];
            int nOther = __builtin_popcountll(act);
            if (nOther > t) continue;
            bool path = true;           // first zero group at every level = chosen group
            for (const Level& L : levels) {
                int fz = -1;
                for (int q = 0; q < L.g; q++) if ((act & L.groupMask[q]) == 0) { fz = q; break; }
                if (fz != L.chosen) { path = false; break; }
            }
            if (!path) continue;
            if (side == 1 && nOther <= cf.hA) continue;        // covered by side A
            uint64_t self[5] = { 0, 0, 0, 0, 0 };
            for (int s = 0; s < n; s++)
                for (int i = 0; i < 5; i++)
                    if ((x >> (5 * s + i)) & 1) self[i] |= (uint64_t)1 << S[s];
            const uint64_t* A = (side == 0) ? self : other;
            const uint64_t* B = (side == 0) ? other : self;
            unsigned w0 = 0, w1 = 0;
            uint64_t ma = A[0] | A[1] | A[2] | A[3] | A[4], mb = B[0] | B[1] | B[2] | B[3] | B[4];
            while (ma) { int c = __builtin_ctzll(ma); ma &= ma - 1; w0 += wt.minRev[colValue(A, c)]; }
            while (mb) { int c = __builtin_ctzll(mb); mb &= mb - 1; w1 += wt.w[colValue(B, c)]; }
            if ((int)(cf.ca * w0 + cf.cb * w1) > cf.T) continue;
            // remove translates by the stabilizer of S (keep the smallest "self" value)
            bool keep = true;
            for (size_t z = 1; z < stab->size() && keep; z++) {
                int sh = 64 - (*stab)[z];
                for (int k = 0; k < 5; k++) {
                    uint64_t r = rotl64(self[k], sh);
                    if (r != self[k]) { if (r < self[k]) keep = false; break; }
                }
            }
            if (!keep) continue;
            st.cores++;
            out << dec << 2 << " " << (w0 + w1) << " " << w0 << " " << w1 << " "
                << hex << B[0] << " " << B[1] << " " << B[2] << " " << B[3] << " " << B[4] << "\n";
        }
        st.kernelVectors += cnt;
    }

public:
    // side = 0 (unknowns at a, other = b) or 1 (unknowns at b, other = a)
    void processSet(const int* S_, int n_, const vector<int>& stab_, int side_)
    {
        S = S_; n = n_; stab = &stab_; side = side_;
        t = (side == 0) ? cf.tA(n) : cf.tB(n);
        if (t < 1) return;
        const int nu = 5 * n;
        if (nu > 64) throw runtime_error("too many unknowns (at most 12 columns per side)");
        uint64_t xs[64], ims[64][5];
        for (int s = 0; s < n; s++)
            for (int i = 0; i < 5; i++) {
                xs[5 * s + i] = (uint64_t)1 << (5 * s + i);
                for (int k = 0; k < 5; k++)
                    ims[5 * s + i][k] = rotl64(side == 0 ? im.f[i][k] : im.inv[i][k], S[s]);
            }
        st.sets++;
        levels.clear();
        refine(nu, xs, ims, 0);
    }
};

long long binom(int n, int k)
{
    if (k < 0 || k > n) return 0;
    long long r = 1;
    for (int i = 1; i <= k; i++) r = r * (n - k + i) / i;
    return r;
}

// enumerate all subsets {0} U comb(1..63, n-1) whose second element is "second" (n >= 2) and process them
void enumerateWithSecond(Worker& w, int n, int second, int side)
{
    int S[64];
    S[0] = 0;
    S[1] = second;
    vector<int> stab;
    if (n == 2) {
        bool can; canonicalCheck(S, n, can, stab);
        if (can) w.processSet(S, n, stab, side);
        return;
    }
    // iterative combination over the remaining n-2 elements from (second+1 .. 63)
    int m = n - 2;
    int c[64];
    for (int q = 0; q < m; q++) c[q] = second + 1 + q;
    if (c[m - 1] > 63) return;
    while (true) {
        for (int q = 0; q < m; q++) S[2 + q] = c[q];
        bool can; canonicalCheck(S, n, can, stab);
        if (can) w.processSet(S, n, stab, side);
        int q = m - 1;
        while (q >= 0 && c[q] == 63 - (m - 1 - q)) q--;
        if (q < 0) break;
        c[q]++;
        for (int r = q + 1; r < m; r++) c[r] = c[r - 1] + 1;
    }
}

} // namespace

int generateTwoRoundTrailCoresGeneric(int weightedW, int T, const string& oFileName)
{
    Config cf;
    cf.T = T;
    if (weightedW == 11) { cf.ca = 1; cf.cb = 1; }
    else if (weightedW == 12) { cf.ca = 1; cf.cb = 2; }
    else if (weightedW == 21) { cf.ca = 2; cf.cb = 1; }
    else { cout << "weightedWeights must be 11, 12 or 21" << endl; return 0; }
    cf.maxNa = 0; while (cf.tA(cf.maxNa + 1) >= 1) cf.maxNa++;
    cf.maxNb = 0; while (cf.tB(cf.maxNb + 1) >= 1) cf.maxNb++;
    if (cf.maxNa == 0) { ofstream(oFileName).close(); cout << "empty region" << endl; return 1; }
    // choose the cover (hA, hB) of the region with the smallest estimated cost
    double best = 1e300; int bestHA = cf.maxNa;
    for (int hA = 0; hA <= cf.maxNa; hA++) {
        int hB = (hA + 1 <= cf.maxNa) ? cf.tA(hA + 1) : 0;
        double cost = 0;
        for (int n = 1; n <= hA; n++) cost += (double)binom(63, n - 1) * n;
        for (int m = 1; m <= hB; m++) cost += (double)binom(63, m - 1) * m;
        if (cost < best) { best = cost; bestHA = hA; }
    }
    cf.hA = bestHA;
    cf.hB = (cf.hA + 1 <= cf.maxNa) ? cf.tA(cf.hA + 1) : 0;

    Images im = computeImages();
    Weights wt;
    Propagation prop;
    for (int v = 0; v < 32; v++) { wt.w[v] = prop.weightListPerInput[v]; wt.minRev[v] = prop.minRevWeightListPerOutput[v]; }

    cout << "generic 2-round generation, layer " << g_layer.describe() << ", " << ((DTS == 1) ? "DTS" : "LTS")
         << ", condition " << cf.ca << "*w0 + " << cf.cb << "*w1 <= " << T << endl;
    cout << "  region: na <= " << cf.maxNa << ", nb <= " << cf.maxNb << "; cover: side A na <= " << cf.hA
         << ", side B nb <= " << cf.hB << " (with na > " << cf.hA << ")" << endl;

    // work items: (side, n, second element); n = 1 is a single item
    struct Item { int side, n, second; };
    vector<Item> items;
    for (int side = 0; side < 2; side++) {
        int h = (side == 0) ? cf.hA : cf.hB;
        for (int n = 1; n <= h; n++) {
            if (n == 1) { items.push_back({ side, 1, 0 }); continue; }
            for (int second = 1; second <= 63 - (n - 2); second++) items.push_back({ side, n, second });
        }
    }
    // largest items first (better load balance)
    sort(items.begin(), items.end(), [](const Item& x, const Item& y) {
        long long cx = binom(63 - x.second, x.n - 2), cy = binom(63 - y.second, y.n - 2);
        return cx * x.n > cy * y.n;
    });
    Stats st;
    const int nItems = (int)items.size();
    vector<string> outputs(nItems);
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(dynamic, 1)
    for (int it = 0; it < nItems; it++) {
        Worker w(im, wt, cf, st);
        const Item& item = items[it];
        if (item.n == 1) {
            int S[1] = { 0 };
            vector<int> stab(1, 0);
            w.processSet(S, 1, stab, item.side);
        }
        else enumerateWithSecond(w, item.n, item.second, item.side);
        outputs[it] = w.out.str();
    }
    // deterministic output: sort the lines
    vector<string> lines;
    for (auto& o : outputs) {
        stringstream ss(o); string l;
        while (getline(ss, l)) if (!l.empty()) lines.push_back(l);
    }
    sort(lines.begin(), lines.end());
    ofstream fout(oFileName);
    for (auto& l : lines) fout << l << "\n";
    fout.close();
    cout << "  canonical column sets: " << st.sets << ", linear systems: " << st.systems
         << ", kernel vectors: " << st.kernelVectors << ", max kernel dim: " << st.maxKernelDim << endl;
    cout << "number of trails: " << st.cores << " -> " << oFileName << endl;
    cout << "  time " << (omp_get_wtime() - t0) << " s, threads " << omp_get_max_threads() << endl;
    return 1;
}
