"""
test_smrm_aead.py -- Thay p_L cua Ascon bang mot tang SMRM (mac dinh L_Ascon2) va kiem tra che do AEAD van dung.

Khong co test vector chinh thuc cho Ascon-SMRM, nen tinh dung duoc kiem tra theo 4 lop:
  A. Tang tuyen tinh: cai dat o day == smrm/layers.py (cai dat cua du an) == ma tran 320x320 results/mat/<TEN>.bin,
     tren 10^4 trang thai ngau nhien (seed co dinh); L kha nghich (L^-1(L(x)) = x).
  B. AEAD: giai_ma(ma_hoa(m)) = m tren toan luoi PT 0..32 x AD 0..32 byte + 300 thong diep ngau nhien den 1000 byte.
  C. Xac thuc: moi thay doi (1 bit cua the, cua ban ma, cua AD, cua nonce, cua khoa; cat/them byte) deu bi TU CHOI.
  D. Tang da thuc su duoc thay: dau ra khac Ascon goc tren moi vector; hieu ung tuyet lo cua the (~64/128 bit).
Ghi tep KAT tu sinh results/py_<TEN>_KAT.txt de doi chieu voi cai dat C (run_all.sh).
Chay: python test_smrm_aead.py [TEN=L_Ascon2]
"""
import os
import random
import struct
import sys

from ascon_aead import AsconAEAD128, make_layer, write_kat, finalist_spec

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
OUT = os.path.join(HERE, "..", "results")
SEED = 20261030

sys.path.insert(0, os.path.join(ROOT, "smrm"))
import layers as prj                                  # cai dat tang tuyen tinh cua du an (smrm/layers.py)


def load_matrix(path):
    cols = []
    with open(path, "rb") as f:
        data = f.read()
    assert len(data) == 320 * 5 * 8, "kich thuoc ma tran sai"
    for k in range(320):
        cols.append(struct.unpack_from("<5Q", data, k * 40))
    return cols


def apply_matrix(cols, s):
    y = [0] * 5
    for w in range(5):
        for p in range(64):
            if (s[w] >> p) & 1:
                c = cols[w * 64 + p]
                for t in range(5):
                    y[t] ^= c[t]
    return y


def check(cond, msg, failures):
    print(("  ok   " if cond else "  FAIL ") + msg)
    if not cond:
        failures.append(msg)


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "L_Ascon2"
    rng = random.Random(SEED)
    fails = []
    lay = make_layer(name)
    print("Tang: %s  (%s, %d XOR 64 bit = %.1f XOR/bit)" % (name, lay.spec(), lay.xor_count, lay.xor_count * 64 / 320))

    # ---------------- A. tang tuyen tinh
    print("A. Tang tuyen tinh")
    ref = prj.SMRM(lay.ops, lay.d, name)
    mpath = os.path.join(ROOT, "results", "mat", name + ".bin")
    cols = load_matrix(mpath) if os.path.exists(mpath) else None
    asc = make_layer("ascon")
    n_ref = n_mat = n_inv = n_asc = 0
    for t in range(10000):
        s = [rng.getrandbits(64) for _ in range(5)]
        y = lay(s)
        n_ref += y == ref(s)
        n_inv += ref.inverse(y) == s
        n_asc += asc(s) == prj.ascon_pL(s)
        if cols is not None and t < 1000:
            n_mat += y == apply_matrix(cols, s)
    check(n_ref == 10000, "khop smrm/layers.py tren 10^4 trang thai", fails)
    check(n_inv == 10000, "kha nghich: L^-1(L(x)) = x tren 10^4 trang thai", fails)
    check(n_asc == 10000, "p_L Ascon o day khop smrm/layers.ascon_pL", fails)
    if cols is not None:
        check(n_mat == 1000, "khop ma tran %s tren 10^3 trang thai" % os.path.relpath(mpath, ROOT), fails)
    else:
        print("  (bo qua: khong co %s)" % mpath)

    # ---------------- B. giai ma(ma hoa(m)) = m
    print("B. Giai ma dung ban ro")
    aead = AsconAEAD128(lay)
    ascon = AsconAEAD128("ascon")
    key, nonce = bytes(range(16)), bytes(range(16, 32))
    grid_ok = differ = 0
    for lp in range(33):
        for la in range(33):
            pt, ad = bytes(range(0x20, 0x20 + lp)), bytes(range(0x30, 0x30 + la))
            c = aead.encrypt(key, nonce, ad, pt)
            grid_ok += len(c) == lp + 16 and aead.decrypt(key, nonce, ad, c) == pt
            differ += c != ascon.encrypt(key, nonce, ad, pt)
    check(grid_ok == 1089, "luoi KAT 33 x 33: %d/1089 vector giai ma dung" % grid_ok, fails)
    rnd_ok = 0
    for _ in range(300):
        k, n = rng.randbytes(16), rng.randbytes(16)
        pt, ad = rng.randbytes(rng.randrange(1001)), rng.randbytes(rng.randrange(65))
        rnd_ok += aead.decrypt(k, n, ad, aead.encrypt(k, n, ad, pt)) == pt
    check(rnd_ok == 300, "300 thong diep ngau nhien (0..1000 byte): %d/300 giai ma dung" % rnd_ok, fails)

    # ---------------- C. xac thuc: moi sua doi bi tu choi
    print("C. Tu choi du lieu bi sua")
    pt, ad = b"Ban ro thu nghiem cho Ascon-SMRM (32+ byte)...", b"du lieu lien ket"
    c = aead.encrypt(key, nonce, ad, pt)
    rej = tot = 0
    for bit in range(len(c) * 8):                                   # moi bit cua ban ma va the
        f = bytearray(c); f[bit // 8] ^= 1 << (bit % 8)
        rej += aead.decrypt(key, nonce, ad, bytes(f)) is None; tot += 1
    check(rej == tot, "lat tung bit cua ban ma||the: %d/%d bi tu choi" % (rej, tot), fails)
    rej = tot = 0
    for bit in range(len(ad) * 8):
        f = bytearray(ad); f[bit // 8] ^= 1 << (bit % 8)
        rej += aead.decrypt(key, nonce, bytes(f), c) is None; tot += 1
    for bit in range(128):
        f = bytearray(nonce); f[bit // 8] ^= 1 << (bit % 8)
        rej += aead.decrypt(key, bytes(f), ad, c) is None; tot += 1
        f = bytearray(key); f[bit // 8] ^= 1 << (bit % 8)
        rej += aead.decrypt(bytes(f), nonce, ad, c) is None; tot += 1
    check(rej == tot, "lat tung bit cua AD / nonce / khoa: %d/%d bi tu choi" % (rej, tot), fails)
    others = [c[:-1], c + b"\x00", c[1:], b"", c[:16]]
    check(all(aead.decrypt(key, nonce, ad, x) is None for x in others), "cat / them byte, chuoi rong: deu bi tu choi", fails)
    check(aead.decrypt(key, nonce, b"", c) is None and aead.decrypt(key, nonce, ad + b"\x00", c) is None,
          "bo AD / them byte 0 vao AD: bi tu choi (padding phan biet duoc)", fails)
    check(ascon.decrypt(key, nonce, ad, c) is None, "ban ma Ascon-SMRM KHONG giai ma duoc bang Ascon goc", fails)

    # ---------------- D. tang da duoc thay that + tuyet lo
    print("D. Tang da thuc su duoc thay")
    check(differ == 1089, "dau ra khac Ascon goc tren %d/1089 vector" % differ, fails)
    for label, a in (("Ascon", ascon), (name, aead)):
        t0 = a.encrypt(key, nonce, ad, pt)[-16:]
        dist = []
        for bit in range(128):
            f = bytearray(nonce); f[bit // 8] ^= 1 << (bit % 8)
            t1 = a.encrypt(key, bytes(f), ad, pt)[-16:]
            dist.append(sum(bin(x ^ y).count("1") for x, y in zip(t0, t1)))
        print("  %-8s lat 1 bit nonce -> the doi trung binh %.1f / 128 bit (min %d, max %d)"
              % (label, sum(dist) / 128, min(dist), max(dist)))

    os.makedirs(OUT, exist_ok=True)
    kat = os.path.join(OUT, "py_%s_KAT.txt" % name)
    print("Ghi KAT tu sinh (%d vector): %s" % (write_kat(kat, aead), os.path.relpath(kat, os.path.join(HERE, ".."))))
    print("PASS" if not fails else "FAIL (%d)" % len(fails))
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
