"""
ascon_aead.py -- Cai dat tham chieu Ascon-AEAD128 (NIST SP 800-232) voi tang tuyen tinh p_L THAY THE DUOC.

Muc dich: (1) kiem tra cai dat khop test vector chinh thuc cua Ascon-AEAD128;
          (2) giu nguyen che do AEAD, so vong (p^12 / p^8), hang so vong, S-box, chi thay p_L bang SMRM.

Quy uoc (trung voi toan du an, xem smrm/layers.py):
  * Trang thai = 5 tu 64 bit S[0..4]; cot j = (S0[j], ..., S4[j]) la dau vao S-box thu j.
  * rotl(x, r) = xoay TRAI r bit; Ascon goc dung xoay PHAI: Sigma_i(x) = x ^ (x >>> a_i) ^ (x >>> b_i).
  * SP 800-232: khoa, nonce, du lieu, the doc/ghi theo LITTLE-ENDIAN tung tu 64 bit.

Tang tuyen tinh:
  * "ascon"                p_L goc.
  * "smrm:<ops>|<d>"       SMRM L = B . D . B^bT:
                               for l = k..1: x[j_l] ^= rotl(x[i_l], r_l)
                               x[w] = rotl(x[w], d_w)
                               for l = 1..k: x[i_l] ^= rotl(x[j_l], r_l)
  * "<TEN>"                ten trong results/02_finalists/finalists.txt (vd. L_Ascon2).
"""
import hmac
import os

MASK64 = (1 << 64) - 1
HERE = os.path.dirname(os.path.abspath(__file__))
FINALISTS = os.path.join(HERE, "..", "..", "results", "02_finalists", "finalists.txt")


def rotl(x, r):
    r %= 64
    return ((x << r) | (x >> (64 - r))) & MASK64 if r else x


def rotr(x, r):
    return rotl(x, 64 - (r % 64))


# ---------------------------------------------------------------- tang tuyen tinh
ASCON_ROT = [(19, 28), (61, 39), (1, 6), (10, 17), (7, 41)]


class AsconLinear:
    name = "ascon"
    xor_count = 10                                   # so phep XOR 64 bit

    def __call__(self, s):
        return [s[i] ^ rotr(s[i], a) ^ rotr(s[i], b) for i, (a, b) in enumerate(ASCON_ROT)]


class SMRMLinear:
    def __init__(self, ops, d, name="SMRM"):
        self.ops = [tuple(o) for o in ops]           # (i, j, r)
        self.d = list(d)
        self.name = name
        self.xor_count = 2 * len(self.ops)
        for (i, j, r) in self.ops:
            assert 0 <= i < 5 and 0 <= j < 5 and i != j and 0 <= r < 64
        assert len(self.d) == 5

    def __call__(self, s):
        x = list(s)
        for (i, j, r) in reversed(self.ops):
            x[j] ^= rotl(x[i], r)
        x = [rotl(x[w], self.d[w]) for w in range(5)]
        for (i, j, r) in self.ops:
            x[i] ^= rotl(x[j], r)
        return x

    def spec(self):
        return "smrm:" + ";".join("%d,%d,%d" % o for o in self.ops) + "|" + ",".join(map(str, self.d))


def parse_smrm(body, name="SMRM"):
    ops_s, d_s = body.split("|")
    ops = [tuple(int(v) for v in t.split(",")) for t in ops_s.split(";") if t]
    return SMRMLinear(ops, [int(v) for v in d_s.split(",")], name)


def finalist_spec(name, path=FINALISTS):
    """Tra ve chuoi 'smrm:<ops>|<d>' cua ung vien <name> trong finalists.txt."""
    with open(path) as f:
        for line in f:
            parts = line.strip().split("|")
            if len(parts) == 3 and parts[0] == name:
                return "smrm:" + parts[1] + "|" + parts[2]
    raise KeyError("khong co ung vien %r trong %s" % (name, path))


def make_layer(spec):
    if spec == "ascon":
        return AsconLinear()
    if spec.startswith("smrm:"):
        return parse_smrm(spec[5:])
    layer = parse_smrm(finalist_spec(spec)[5:], spec)
    return layer


# ---------------------------------------------------------------- hoan vi Ascon-p
RC = [0xF0 - 0x0F * i for i in range(12)]            # 0xf0, 0xe1, ..., 0x4b


def sbox(s):
    x0, x1, x2, x3, x4 = s
    x0 ^= x4; x4 ^= x3; x2 ^= x1
    t0 = ~x0 & x1; t1 = ~x1 & x2; t2 = ~x2 & x3; t3 = ~x3 & x4; t4 = ~x4 & x0
    x0 ^= t1; x1 ^= t2; x2 ^= t3; x3 ^= t4; x4 ^= t0
    x1 ^= x0; x0 ^= x4; x3 ^= x2; x2 = ~x2
    return [v & MASK64 for v in (x0, x1, x2, x3, x4)]


def permutation(s, rounds, linear):
    """p^rounds: dung cac hang so vong cuoi (12 - rounds .. 11), nhu Ascon."""
    for r in range(12 - rounds, 12):
        s[2] ^= RC[r]                                # p_C
        s = sbox(s)                                  # p_S
        s = linear(s)                                # p_L (thay the duoc)
    return s


# ---------------------------------------------------------------- Ascon-AEAD128
IV = 0x00001000808C0001                              # SP 800-232, Ascon-AEAD128
RATE = 16
DSEP = 1 << 63                                       # tach mien sau AD: bit cuoi cua trang thai


def u64(b):
    return int.from_bytes(b, "little")


def b64(x):
    return (x & MASK64).to_bytes(8, "little")


def pad(block):
    """Them byte 0x01 roi cac byte 0 den du 16 byte (len(block) < 16)."""
    return block + b"\x01" + b"\x00" * (RATE - 1 - len(block))


class AsconAEAD128:
    KEY_BYTES = 16
    NONCE_BYTES = 16
    TAG_BYTES = 16

    def __init__(self, layer="ascon", rounds_a=12, rounds_b=8):
        self.linear = make_layer(layer) if isinstance(layer, str) else layer
        self.ra, self.rb = rounds_a, rounds_b

    def _p(self, s, r):
        return permutation(s, r, self.linear)

    def _init(self, key, nonce, ad):
        assert len(key) == 16 and len(nonce) == 16
        k0, k1 = u64(key[:8]), u64(key[8:])
        s = [IV, k0, k1, u64(nonce[:8]), u64(nonce[8:])]
        s = self._p(s, self.ra)
        s[3] ^= k0; s[4] ^= k1
        if ad:
            a = ad[: len(ad) - len(ad) % RATE] + pad(ad[len(ad) - len(ad) % RATE:])
            for i in range(0, len(a), RATE):
                s[0] ^= u64(a[i:i + 8]); s[1] ^= u64(a[i + 8:i + 16])
                s = self._p(s, self.rb)
        s[4] ^= DSEP
        return s, k0, k1

    def _final(self, s, k0, k1):
        s[2] ^= k0; s[3] ^= k1
        s = self._p(s, self.ra)
        return b64(s[3] ^ k0) + b64(s[4] ^ k1)

    def encrypt(self, key, nonce, ad, pt):
        """Tra ve C || T (len(pt) + 16 byte)."""
        s, k0, k1 = self._init(key, nonce, ad)
        out = bytearray()
        full = len(pt) - len(pt) % RATE
        for i in range(0, full, RATE):
            s[0] ^= u64(pt[i:i + 8]); s[1] ^= u64(pt[i + 8:i + 16])
            out += b64(s[0]) + b64(s[1])
            s = self._p(s, self.rb)
        last = pad(pt[full:])
        s[0] ^= u64(last[:8]); s[1] ^= u64(last[8:])
        out += (b64(s[0]) + b64(s[1]))[: len(pt) - full]
        return bytes(out) + self._final(s, k0, k1)

    def decrypt(self, key, nonce, ad, ct_tag):
        """Tra ve ban ro, hoac None neu the xac thuc sai (khong tra ban ro khi xac thuc that bai)."""
        if len(ct_tag) < self.TAG_BYTES:
            return None
        ct, tag = ct_tag[:-self.TAG_BYTES], ct_tag[-self.TAG_BYTES:]
        s, k0, k1 = self._init(key, nonce, ad)
        out = bytearray()
        full = len(ct) - len(ct) % RATE
        for i in range(0, full, RATE):
            c0, c1 = u64(ct[i:i + 8]), u64(ct[i + 8:i + 16])
            out += b64(s[0] ^ c0) + b64(s[1] ^ c1)
            s[0], s[1] = c0, c1
            s = self._p(s, self.rb)
        l = len(ct) - full
        st = b64(s[0]) + b64(s[1])
        c = ct[full:]
        out += bytes(st[i] ^ c[i] for i in range(l))
        st = c + bytes([st[l] ^ 0x01]) + st[l + 1:]
        s[0], s[1] = u64(st[:8]), u64(st[8:])
        if not hmac.compare_digest(self._final(s, k0, k1), tag):
            return None
        return bytes(out)


# ---------------------------------------------------------------- doc / ghi tep KAT (dinh dang LWC)
def read_kat(path):
    vecs, cur = [], {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                if cur:
                    vecs.append(cur); cur = {}
                continue
            k, _, v = line.partition("=")
            k, v = k.strip(), v.strip()
            cur[k] = int(v) if k == "Count" else bytes.fromhex(v)
    if cur:
        vecs.append(cur)
    return vecs


def write_kat(path, aead, max_pt=32, max_ad=32):
    """Sinh tep KAT cung luoi (PT 0..32 x AD 0..32 byte) va cung khoa/nonce voi tep chinh thuc."""
    key, nonce = bytes(range(16)), bytes(range(16, 32))
    with open(path, "w", newline="\n") as f:
        count = 1
        for lp in range(max_pt + 1):
            for la in range(max_ad + 1):
                pt = bytes(range(0x20, 0x20 + lp)) if lp else b""
                ad = bytes(range(0x30, 0x30 + la)) if la else b""
                ct = aead.encrypt(key, nonce, ad, pt)
                f.write("Count = %d\nKey = %s\nNonce = %s\nPT = %s\nAD = %s\nCT = %s\n\n"
                        % (count, key.hex().upper(), nonce.hex().upper(), pt.hex().upper(),
                           ad.hex().upper(), ct.hex().upper()))
                count += 1
    return count - 1
