/*
DATN extension of AsconTrailTool: run-time selectable linear layers (see genericLayer.h).
*/

#include "genericLayer.h"
#include "asconParts.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <array>
#include <vector>

GenericLayer g_layer;

static inline uint64_t rotl64(uint64_t x, int r)
{
    r &= 63;
    return r ? ((x << r) | (x >> (64 - r))) : x;
}

uint64_t polyMul64(uint64_t a, uint64_t b)
{
    uint64_t res = 0;
    for (int e = 0; e < 64; e++)
        if ((a >> e) & 1) res ^= rotl64(b, e);
    return res;
}

uint64_t polyInv64(uint64_t a)
{
    if ((__builtin_popcountll(a) & 1) == 0)
        throw std::runtime_error("ROWPOLY with even weight is not invertible");
    // a^{-1} = a^{63} = a * a^2 * a^4 * ... * a^32  (Frobenius, X^64+1 = (X+1)^64)
    uint64_t res = 1, p = a;
    for (int k = 0; k < 6; k++) { res = polyMul64(res, p); p = polyMul64(p, p); }
    if (polyMul64(res, a) != 1) throw std::runtime_error("polynomial inverse failed");
    return res;
}

static uint64_t polyReverse(uint64_t a)        // P(X) -> P(X^{-1})
{
    uint64_t res = 0;
    for (int e = 0; e < 64; e++)
        if ((a >> e) & 1) res |= (uint64_t)1 << ((64 - e) & 63);
    return res;
}

static void applyOps(const std::vector<LayerOp>& ops, State& s)
{
    for (const LayerOp& op : ops) {
        switch (op.type) {
        case LayerOp::XORROT:
            s.setRow(op.i, s.getRow(op.i) ^ rotl64(s.getRow(op.j), op.r));
            break;
        case LayerOp::ROWROT:
            s.setRow(op.i, rotl64(s.getRow(op.i), op.r));
            break;
        case LayerOp::ROWPOLY: {
            uint64_t x = s.getRow(op.i), y = 0, p = op.poly;
            while (p) { int e = __builtin_ctzll(p); p &= p - 1; y ^= rotl64(x, e); }
            s.setRow(op.i, y);
            break; }
        }
    }
}

static void applyPoly(const uint64_t P[5][5], State& s)
{
    uint64_t x[5], y[5] = { 0, 0, 0, 0, 0 };
    for (int i = 0; i < 5; i++) x[i] = s.getRow(i);
    for (int k = 0; k < 5; k++)
        for (int i = 0; i < 5; i++) {
            uint64_t p = P[k][i];
            while (p) { int e = __builtin_ctzll(p); p &= p - 1; y[k] ^= rotl64(x[i], e); }
        }
    for (int k = 0; k < 5; k++) s.setRow(k, y[k]);
}

/* matrix:<file> : 320 columns x 5 uint64. Column w*64+c = image of bit (row w, column c). */
static void loadMatrixLayer(GenericLayer& L, const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open matrix file " + path);
    std::vector<std::array<uint64_t, 5>> col(320);
    for (int j = 0; j < 320; j++)
        for (int w = 0; w < 5; w++) {
            unsigned char b[8];
            if (!f.read((char*)b, 8)) throw std::runtime_error("matrix file too short: " + path);
            uint64_t v = 0; for (int q = 7; q >= 0; q--) v = (v << 8) | b[q];
            col[j][w] = v;
        }
    // rotation invariance and polynomial form of L
    for (int i = 0; i < 5; i++) {
        for (int k = 0; k < 5; k++) L.poly[0][k][i] = col[i * 64][k];
        for (int c = 1; c < 64; c++)
            for (int k = 0; k < 5; k++)
                if (col[i * 64 + c][k] != rotl64(col[i * 64][k], c))
                    throw std::runtime_error("matrix is not invariant under column rotation: " + path);
    }
    // exact inverse by Gauss-Jordan on rows: rows[r] = row r of L (320 bits), aug = identity
    auto get = [](const std::array<uint64_t, 5>& v, int k) { return (int)((v[k / 64] >> (k % 64)) & 1); };
    std::vector<std::array<uint64_t, 5>> rows(320), aug(320);
    for (int r = 0; r < 320; r++) { rows[r].fill(0); aug[r].fill(0); aug[r][r / 64] |= (uint64_t)1 << (r % 64); }
    for (int j = 0; j < 320; j++)
        for (int r = 0; r < 320; r++)
            if (get(col[j], r)) rows[r][j / 64] |= (uint64_t)1 << (j % 64);
    for (int c = 0, r = 0; c < 320; c++, r++) {
        int piv = -1;
        for (int i = r; i < 320; i++) if (get(rows[i], c)) { piv = i; break; }
        if (piv < 0) throw std::runtime_error("matrix is singular: " + path);
        std::swap(rows[r], rows[piv]); std::swap(aug[r], aug[piv]);
        for (int i = 0; i < 320; i++)
            if (i != r && get(rows[i], c))
                for (int w = 0; w < 5; w++) { rows[i][w] ^= rows[r][w]; aug[i][w] ^= aug[r][w]; }
    }
    // aug row r = row r of L^{-1}; image of bit (i,0) under L^{-1} = column i*64 of L^{-1}
    for (int i = 0; i < 5; i++)
        for (int k = 0; k < 5; k++) {
            uint64_t v = 0;
            for (int e = 0; e < 64; e++) if (get(aug[k * 64 + e], i * 64)) v |= (uint64_t)1 << e;
            L.poly[1][k][i] = v;
        }
    for (int k = 0; k < 5; k++)
        for (int i = 0; i < 5; i++) {
            L.poly[2][k][i] = polyReverse(L.poly[0][i][k]);
            L.poly[3][k][i] = polyReverse(L.poly[1][i][k]);
        }
}

void GenericLayer::deriveOthers()
{
    inverse.clear(); transpose.clear(); inverseTranspose.clear();
    for (int k = (int)forward.size() - 1; k >= 0; k--) {
        LayerOp op = forward[k], inv = op, tr = op;
        switch (op.type) {
        case LayerOp::XORROT:
            if (op.i == op.j) throw std::runtime_error("XORROT with i == j");
            tr.i = op.j; tr.j = op.i; tr.r = (64 - op.r) & 63;
            break;
        case LayerOp::ROWROT:
            inv.r = (64 - op.r) & 63; tr.r = inv.r;
            break;
        case LayerOp::ROWPOLY:
            inv.poly = polyInv64(op.poly); tr.poly = polyReverse(op.poly);
            break;
        }
        inverse.push_back(inv);
        transpose.push_back(tr);
    }
    // (L^{-1})^T = (L^T)^{-1}: reverse the transposed program and invert every op
    for (int k = (int)transpose.size() - 1; k >= 0; k--) {
        LayerOp op = transpose[k], inv = op;
        if (op.type == LayerOp::ROWROT) inv.r = (64 - op.r) & 63;
        if (op.type == LayerOp::ROWPOLY) inv.poly = polyInv64(op.poly);
        inverseTranspose.push_back(inv);
    }
}

static std::vector<int> splitInts(const std::string& s, char sep)
{
    std::vector<int> v;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, sep)) if (!tok.empty()) v.push_back(std::stoi(tok));
    return v;
}

static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

GenericLayer GenericLayer::fromSpec(const std::string& specIn)
{
    std::string spec = trim(specIn);
    GenericLayer L;
    L.spec = spec;
    if (spec == "ascon") return L;
    L.hardCodedAscon = false;
    if (spec == "ascon-generic") {
        const int rot[5][2] = { {19, 28}, {61, 39}, {1, 6}, {10, 17}, {7, 41} };   // rotations to the RIGHT
        L.name = "ascon-generic";
        for (int i = 0; i < 5; i++) {
            LayerOp op; op.type = LayerOp::ROWPOLY; op.i = i; op.j = i; op.r = 0;
            op.poly = 1 | ((uint64_t)1 << ((64 - rot[i][0]) & 63)) | ((uint64_t)1 << ((64 - rot[i][1]) & 63));
            L.forward.push_back(op);
        }
        L.deriveOthers();
        return L;
    }
    if (spec.rfind("matrix:", 0) == 0) {
        L.isMatrix = true;
        L.name = "matrix";
        loadMatrixLayer(L, spec.substr(7));
        return L;
    }
    if (spec.rfind("file:", 0) == 0) {
        std::string path = spec.substr(5), want;
        size_t h = path.find('#');
        if (h != std::string::npos) { want = path.substr(h + 1); path = path.substr(0, h); }
        std::ifstream f(path);
        if (!f) throw std::runtime_error("cannot open layer file " + path);
        std::string line;
        while (std::getline(f, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            std::string nm, rest;
            size_t eq = line.find('=');
            size_t bar = line.find('|');
            if (eq != std::string::npos && (bar == std::string::npos || eq < bar)) { nm = line.substr(0, eq); rest = line.substr(eq + 1); }
            else { nm = line.substr(0, bar); rest = "smrm:" + line.substr(bar + 1); }
            if (!want.empty() && trim(nm) != want) continue;
            GenericLayer G = fromSpec(rest);
            G.name = trim(nm);
            return G;
        }
        throw std::runtime_error("layer " + want + " not found in " + path);
    }
    if (spec.rfind("smrm:", 0) == 0) {
        std::string body = spec.substr(5);
        size_t bar = body.find('|');
        if (bar == std::string::npos) throw std::runtime_error("smrm spec needs ops|d");
        std::string opsStr = body.substr(0, bar), dStr = body.substr(bar + 1);
        std::vector<std::vector<int>> ops;
        std::stringstream ss(opsStr);
        std::string tok;
        while (std::getline(ss, tok, ';')) {
            std::vector<int> v = splitInts(tok, ',');
            if (v.size() != 3) throw std::runtime_error("bad smrm op " + tok);
            if (v[0] == v[1] || v[0] < 0 || v[0] > 4 || v[1] < 0 || v[1] > 4) throw std::runtime_error("bad smrm rows " + tok);
            ops.push_back(v);
        }
        std::vector<int> d = splitInts(dStr, ',');
        if (d.size() != 5) throw std::runtime_error("smrm d needs 5 values");
        L.name = "smrm";
        // for l = k..1 : x[j] ^= rotl(x[i], r)
        for (int l = (int)ops.size() - 1; l >= 0; l--) {
            LayerOp op; op.type = LayerOp::XORROT; op.i = ops[l][1]; op.j = ops[l][0]; op.r = ops[l][2] & 63; op.poly = 0;
            L.forward.push_back(op);
        }
        for (int w = 0; w < 5; w++) {
            if ((d[w] & 63) == 0) continue;
            LayerOp op; op.type = LayerOp::ROWROT; op.i = w; op.j = w; op.r = d[w] & 63; op.poly = 0;
            L.forward.push_back(op);
        }
        // for l = 1..k : x[i] ^= rotl(x[j], r)
        for (size_t l = 0; l < ops.size(); l++) {
            LayerOp op; op.type = LayerOp::XORROT; op.i = ops[l][0]; op.j = ops[l][1]; op.r = ops[l][2] & 63; op.poly = 0;
            L.forward.push_back(op);
        }
        L.deriveOthers();
        return L;
    }
    throw std::runtime_error("unknown layer specification: " + spec);
}

void GenericLayer::applyForward(State& s) const
{
    if (isMatrix) applyPoly(poly[0], s); else if (hardCodedAscon) asconLinearLayer(s); else applyOps(forward, s);
}

void GenericLayer::applyInverse(State& s) const
{
    if (isMatrix) applyPoly(poly[1], s); else if (hardCodedAscon) asconInverseLinearLayer(s); else applyOps(inverse, s);
}

void GenericLayer::applyTranspose(State& s) const
{
    if (isMatrix) applyPoly(poly[2], s); else if (hardCodedAscon) asconTransposeLinearLayer(s); else applyOps(transpose, s);
}

void GenericLayer::applyInverseTranspose(State& s) const
{
    if (isMatrix) applyPoly(poly[3], s); else if (hardCodedAscon) asconInverseTransposeLinearLayer(s); else applyOps(inverseTranspose, s);
}

std::string GenericLayer::describe() const
{
    std::ostringstream o;
    o << name << " [" << spec << "]";
    if (isMatrix) o << " (polynomial matrix)";
    else if (!hardCodedAscon) o << " (" << forward.size() << " ops)";
    return o.str();
}
