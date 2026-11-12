"""
layers.py -- Dinh nghia cac tang tuyen tinh tren trang thai 5 x 64 bit (kieu LS-Design / Ascon)
va cac phep toan ma tran GF(2) 320 x 320.

Quy uoc (dung chung cho TOAN BO du an, ke ca ma C):
  * Trang thai = 5 tu 64 bit (x0..x4). Cot j = (x0[j], ..., x4[j]) la dau vao cua S-box thu j.
  * Vector 320 bit duoc bieu dien bang so nguyen Python; bit chi so k = w*64 + p
    (w = so hieu tu/hang, p = vi tri bit trong tu = so hieu cot).
  * rotl(x, r): xoay trai r bit  <=>  nhan voi X^r trong vanh R = F2[X]/(X^64 + 1)
    (bit p chuyen sang vi tri p + r mod 64).
  * Moi tang tuyen tinh o day "giao hoan voi phep xoay dong thoi ca 5 tu"
    (cac khoi 64x64 deu la ma tran vong) -> tinh chat nay duoc kiem tra trong test.
"""
import random

W = 64                     # do dai tu
NW = 5                     # so tu (= kich thuoc S-box)
N = W * NW                 # 320
MASK = (1 << W) - 1


def rotl(x, r):
    r %= W
    return ((x << r) | (x >> (W - r))) & MASK if r else x


def rotr(x, r):
    return rotl(x, (W - r) % W)


# ---------------------------------------------------------------- chuyen doi
def state_to_int(s):
    v = 0
    for w in range(NW):
        v |= s[w] << (w * W)
    return v


def int_to_state(v):
    return [(v >> (w * W)) & MASK for w in range(NW)]


def col_weight_state(s):
    """So cot khac 0 = so S-box hoat dong."""
    return bin(s[0] | s[1] | s[2] | s[3] | s[4]).count("1")


def col_weight(v):
    return col_weight_state(int_to_state(v))


# ---------------------------------------------------------------- cac tang tuyen tinh
ASCON_ROT = [(19, 28), (61, 39), (1, 6), (10, 17), (7, 41)]   # Ascon v1.2 / SP 800-232, xoay PHAI


def ascon_pL(s):
    """p_L cua Ascon: x_i <- x_i ^ (x_i >>> r1) ^ (x_i >>> r2)."""
    return [s[i] ^ rotr(s[i], a) ^ rotr(s[i], b) for i, (a, b) in enumerate(ASCON_ROT)]


class MRM:
    """Multiple Rows Mixer (Yu & Liu, ToSC 2025, Algorithm 1).
    x_j <- x_{n1[j]} ^ (x_{n2[j]} <<< m_j), j = r..r+t-1 ; sau do x_j <- x_j <<< s_j cho t..r+t-1;
    dau ra (x_t, ..., x_{r+t-1})."""

    def __init__(self, n1, n2, m, s, r=5, name="MRM"):
        assert len(n1) == len(n2) == len(m) and len(s) == r
        self.n1, self.n2, self.m, self.s, self.r, self.t = n1, n2, m, s, r, len(m)
        self.name = name

    def __call__(self, st):
        x = list(st) + [0] * self.t
        for idx in range(self.t):
            j = self.r + idx
            x[j] = x[self.n1[idx]] ^ rotl(x[self.n2[idx]], self.m[idx])
        out = x[self.t:self.t + self.r]
        return [rotl(out[i], self.s[i]) for i in range(self.r)]

    def xor_count(self):
        return self.t * W


# Bang 9 (Hsilu, B_d = 10, B_l = 5 theo bai bao) va Bang 2 (MRM, B_d = 12)
HSILU = MRM(n1=[0, 3, 1, 0, 5, 5, 7, 11, 12, 12, 13, 14, 15],
            n2=[4, 4, 2, 1, 2, 3, 6, 8, 9, 6, 10, 11, 9],
            m=[47, 51, 54, 30, 4, 55, 0, 14, 59, 4, 10, 45, 53],
            s=[0, 57, 13, 31, 20], name="Hsilu-MRM")
MRM12 = MRM(n1=[1, 2, 2, 0, 5, 5, 6, 11, 12, 13, 14, 15, 16],
            n2=[4, 3, 1, 3, 0, 4, 9, 7, 8, 10, 11, 12, 9],
            m=[15, 34, 9, 16, 27, 14, 61, 13, 42, 43, 43, 52, 41],
            s=[0, 57, 13, 15, 22], name="MRM-Table2")


class SMRM:
    """Tang de xuat: Symmetric Multiple-Row Mixer  L = B . D . B^bT.

    B   = E_k ... E_1 ,  E_l : x_{i_l} <- x_{i_l} ^ (x_{j_l} <<< r_l)   (i_l != j_l, in-place)
    B^bT = E_1^bT ... E_k^bT , E_l^bT : x_{j_l} <- x_{j_l} ^ (x_{i_l} <<< r_l)
    D   = diag(X^{d_0}, ..., X^{d_4})  (chi la xoay tu, khong ton XOR)

    Thu tu thuc thi tren trang thai (ap dung B^bT truoc, roi D, roi B):
        for l = k..1 : x[j_l] ^= rotl(x[i_l], r_l)
        x[w] = rotl(x[w], d_w)
        for l = 1..k : x[i_l] ^= rotl(x[j_l], r_l)
    Chi phi: 2k phep XOR 64 bit = 2k/5 XOR/bit.
    """

    def __init__(self, ops, d, name="SMRM"):
        self.ops = [tuple(o) for o in ops]   # (i, j, r)
        self.d = list(d)
        self.name = name
        for (i, j, r) in self.ops:
            assert i != j and 0 <= i < NW and 0 <= j < NW and 0 <= r < W

    def __call__(self, st):
        x = list(st)
        for (i, j, r) in reversed(self.ops):
            x[j] ^= rotl(x[i], r)
        x = [rotl(x[w], self.d[w]) for w in range(NW)]
        for (i, j, r) in self.ops:
            x[i] ^= rotl(x[j], r)
        return x

    def inverse(self, st):
        x = list(st)
        for (i, j, r) in reversed(self.ops):
            x[i] ^= rotl(x[j], r)
        x = [rotr(x[w], self.d[w]) for w in range(NW)]
        for (i, j, r) in self.ops:
            x[j] ^= rotl(x[i], r)
        return x

    def xor_count(self):
        return 2 * len(self.ops) * W

    def spec(self):
        return {"ops": self.ops, "d": self.d}

    @staticmethod
    def random(k, rng):
        ops = []
        for _ in range(k):
            i, j = rng.sample(range(NW), 2)
            ops.append((i, j, rng.randrange(W)))
        d = [0] + [rng.randrange(W) for _ in range(NW - 1)]
        return SMRM(ops, d)


# ---------------------------------------------------------------- ma tran GF(2)
def matrix_columns(f):
    """cols[k] = f(e_k) duoi dang so nguyen 320 bit."""
    cols = []
    for k in range(N):
        s = [0] * NW
        s[k // W] = 1 << (k % W)
        cols.append(state_to_int(f(s)))
    return cols


def columns_to_rows(cols):
    rows = [0] * N
    for j, c in enumerate(cols):
        while c:
            lb = c & -c
            i = lb.bit_length() - 1
            rows[i] |= 1 << j
            c ^= lb
    return rows


rows_to_columns = columns_to_rows      # chuyen vi la phep doi xung


def transpose_cols(cols):
    """Cot cua M^T = hang cua M."""
    return columns_to_rows(cols)


def inverse_cols(cols):
    """Nghich dao bang Gauss-Jordan; tra ve None neu suy bien."""
    rows = columns_to_rows(cols)
    aug = [(rows[i], 1 << i) for i in range(N)]
    aug = [list(t) for t in aug]
    r = 0
    for c in range(N):
        piv = next((i for i in range(r, N) if (aug[i][0] >> c) & 1), None)
        if piv is None:
            return None
        aug[r], aug[piv] = aug[piv], aug[r]
        for i in range(N):
            if i != r and (aug[i][0] >> c) & 1:
                aug[i][0] ^= aug[r][0]
                aug[i][1] ^= aug[r][1]
        r += 1
    inv_rows = [aug[i][1] for i in range(N)]
    return rows_to_columns(inv_rows)


def apply_cols(cols, v):
    out = 0
    k = 0
    while v:
        if v & 1:
            out ^= cols[k]
        v >>= 1
        k += 1
    return out


def write_matrix(cols, path):
    """Ghi 320 cot, moi cot 5 tu uint64 little-endian (dinh dang doc boi bn.c)."""
    with open(path, "wb") as fh:
        for c in cols:
            for w in int_to_state(c):
                fh.write(w.to_bytes(8, "little"))


def density(cols):
    return sum(bin(c).count("1") for c in cols)
