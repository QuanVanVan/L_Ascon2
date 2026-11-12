/*
DATN extension of AsconTrailTool: run-time selectable linear layers.

A linear layer on the 5 x 64 state is given as a straight-line program of elementary
operations, all of which commute with the simultaneous rotation of the 5 rows (so the
whole layer is translation invariant along the columns, as required by the canonicity
arguments of the trail search):

    XORROT  i j r : x_i ^= rotl(x_j, r)          (i != j)
    ROWPOLY i P   : x_i  = sum_{e in P} rotl(x_i, e)   (P = 64-bit mask of rotation amounts,
                                                       odd weight <=> invertible)
    ROWROT  i r   : x_i  = rotl(x_i, r)

From the program we derive exactly
    inverse   : reversed program, every op inverted (XORROT is an involution, ROWPOLY uses the
                polynomial inverse f^{-1} = f^{63} in F2[X]/(X^64+1), ROWROT uses -r);
    transpose : reversed program, every op transposed (XORROT i j r -> XORROT j i -r,
                ROWPOLY P -> P(X^{-1}), ROWROT r -> -r).

Supported specifications (string given with --layer=...):
    ascon                       hard-coded functions of the original tool (default)
    ascon-generic               Ascon p_L expressed with ROWPOLY ops (used to validate this module)
    smrm:<ops>|<d>              SMRM  L = B . D . B^bT of the DATN project, e.g.
                                smrm:1,4,45;4,3,31;3,2,23;2,0,15;0,1,22;1,4,21|0,39,2,16,7
                                (same convention as src/layers.py: for l=k..1: x[j]^=rotl(x[i],r);
                                 x[w]=rotl(x[w],d[w]); for l=1..k: x[i]^=rotl(x[j],r))
    file:<path>                 first line of <path> of the form NAME|ops|d (results/02_finalists/finalists.txt)
                                or NAME=<spec>; NAME selects the line when given as file:<path>#NAME
    matrix:<path.bin>           ANY rotation-invariant layer given by its 320x320 binary matrix (320 columns,
                                each 5 little-endian uint64; format of smrm/bn.c, lowxor/). The layer is stored
                                as 5x5 polynomials P[k][i] (row k of the image of bit (i,0)); the inverse is
                                computed exactly by Gauss-Jordan elimination at load time, transposes by
                                P^T[k][i] = P[i][k](X^{-1}). Loading fails if the matrix is singular or not
                                rotation invariant.
*/

#ifndef GENERIC_LAYER_H
#define GENERIC_LAYER_H

#include <cstdint>
#include <string>
#include <vector>

class State;

struct LayerOp {
    enum Type { XORROT, ROWPOLY, ROWROT } type;
    int i, j, r;
    uint64_t poly;
};

class GenericLayer {
public:
    std::string name;
    std::string spec;
    bool hardCodedAscon;               // true: use the original asconXxxLinearLayer functions
    std::vector<LayerOp> forward, inverse, transpose, inverseTranspose;
    bool isMatrix;                     // true: polynomial-matrix representation (matrix:<file>)
    uint64_t poly[4][5][5];            // [0]=L, [1]=L^-1, [2]=L^T, [3]=L^-T ; y_k = sum_i poly[k][i] * x_i
public:
    GenericLayer() : name("ascon"), spec("ascon"), hardCodedAscon(true), isMatrix(false), poly{} {}
    static GenericLayer fromSpec(const std::string& spec);   // throws std::runtime_error

    void applyForward(State& s) const;
    void applyInverse(State& s) const;
    void applyTranspose(State& s) const;
    void applyInverseTranspose(State& s) const;
    std::string describe() const;
private:
    void deriveOthers();
};

uint64_t polyMul64(uint64_t a, uint64_t b);    // product in F2[X]/(X^64+1), bit e = coefficient of X^e
uint64_t polyInv64(uint64_t a);                // inverse (a must have odd weight)

/* The layer used by the whole tool (set once in main before any computation). */
extern GenericLayer g_layer;

#endif
