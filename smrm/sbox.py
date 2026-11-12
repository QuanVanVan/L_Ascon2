"""S-box 5 bit cua Ascon (sinh tu mo ta bitslice trong dac ta) va cac bang DDT / LAT."""


def ascon_sbox_bitsliced(x0, x1, x2, x3, x4, mask=1):
    x0 ^= x4; x4 ^= x3; x2 ^= x1
    t0 = (~x0 & mask) & x1; t1 = (~x1 & mask) & x2; t2 = (~x2 & mask) & x3
    t3 = (~x3 & mask) & x4; t4 = (~x4 & mask) & x0
    x0 ^= t1; x1 ^= t2; x2 ^= t3; x3 ^= t4; x4 ^= t0
    x1 ^= x0; x0 ^= x4; x3 ^= x2; x2 = ~x2 & mask
    return x0, x1, x2, x3, x4


def _table():
    """Gia tri 5 bit: bit 4 (MSB) = x0, ..., bit 0 = x4 (quy uoc cua dac ta Ascon)."""
    S = []
    for v in range(32):
        bits = [(v >> (4 - w)) & 1 for w in range(5)]
        o = ascon_sbox_bitsliced(*bits)
        S.append(sum(o[w] << (4 - w) for w in range(5)))
    return S


SBOX = _table()
# Bang tra cuu trong dac ta Ascon v1.2 (dung de doi chieu)
SBOX_SPEC = [0x04, 0x0b, 0x1f, 0x14, 0x1a, 0x15, 0x09, 0x02, 0x1b, 0x05, 0x08, 0x12, 0x1d, 0x03, 0x06, 0x1c,
             0x1e, 0x13, 0x07, 0x0e, 0x00, 0x0d, 0x11, 0x18, 0x10, 0x0c, 0x01, 0x19, 0x16, 0x0a, 0x0f, 0x17]


def ddt(S=SBOX):
    T = [[0] * 32 for _ in range(32)]
    for x in range(32):
        for a in range(32):
            T[a][S[x] ^ S[x ^ a]] += 1
    return T


def lat(S=SBOX):
    """LAT[a][b] = #{x : a.x = b.S(x)} - 16."""
    T = [[0] * 32 for _ in range(32)]
    for a in range(32):
        for b in range(32):
            c = sum(1 for x in range(32) if (bin(a & x).count("1") + bin(b & S[x]).count("1")) % 2 == 0)
            T[a][b] = c - 16
    return T


if __name__ == "__main__":
    assert SBOX == SBOX_SPEC, "S-box bitslice khong khop bang dac ta"
    D, L = ddt(), lat()
    du = max(D[a][b] for a in range(1, 32) for b in range(32))
    li = max(abs(L[a][b]) for a in range(32) for b in range(1, 32))
    bd = min(bin(a).count("1") + bin(b).count("1") for a in range(1, 32) for b in range(32) if D[a][b])
    bl = min(bin(a).count("1") + bin(b).count("1") for a in range(1, 32) for b in range(32) if L[a][b])
    print("S-box khop dac ta: OK")
    print("do dong deu vi sai =", du, "-> trong so toi thieu", 5 - du.bit_length() + 1)
    print("max |LAT| =", li, "-> tuong quan toi da", li / 16)
    print("so nhanh bit (vi sai, tuyen tinh) =", bd, bl)
    print("so cap (din,dout) kha thi =", sum(1 for a in range(32) for b in range(32) if D[a][b]))
    print("so cap (u,v) LAT != 0 =", sum(1 for a in range(32) for b in range(32) if L[a][b]))
