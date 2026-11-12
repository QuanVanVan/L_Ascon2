"""
test_kat.py -- Kiem tra Ascon-AEAD128 (p_L goc) voi test vector CHINH THUC (kat/LWC_AEAD_KAT_128_128.txt, 1089 vector).

Voi moi vector: (1) ma hoa cho dung CT||T; (2) giai ma cho dung PT; (3) lat 1 bit cua the -> giai ma phai tu choi.
Them: tep KAT do write_kat() sinh lai phai trung TUNG BYTE voi tep chinh thuc (kiem tra ca bo sinh KAT dung cho SMRM).
Chay: python test_kat.py
"""
import filecmp
import os
import sys

from ascon_aead import AsconAEAD128, read_kat, write_kat

HERE = os.path.dirname(os.path.abspath(__file__))
KAT = os.path.join(HERE, "..", "kat", "LWC_AEAD_KAT_128_128.txt")
OUT = os.path.join(HERE, "..", "results")


def main():
    aead = AsconAEAD128("ascon")
    vecs = read_kat(KAT)
    bad = 0
    for v in vecs:
        ct = aead.encrypt(v["Key"], v["Nonce"], v["AD"], v["PT"])
        pt = aead.decrypt(v["Key"], v["Nonce"], v["AD"], v["CT"])
        forged = bytearray(v["CT"]); forged[-1] ^= 0x01
        rej = aead.decrypt(v["Key"], v["Nonce"], v["AD"], bytes(forged))
        if ct != v["CT"] or pt != v["PT"] or rej is not None:
            bad += 1
            print("FAIL Count = %d" % v["Count"])
    print("Ascon-AEAD128 (Python): %d/%d vector dung (ma hoa, giai ma, tu choi the gia)" % (len(vecs) - bad, len(vecs)))

    os.makedirs(OUT, exist_ok=True)
    regen = os.path.join(OUT, "py_ascon_KAT_regen.txt")
    write_kat(regen, aead)
    same = filecmp.cmp(regen, KAT, shallow=False)
    print("Tep KAT sinh lai %s tep chinh thuc" % ("TRUNG TUNG BYTE voi" if same else "KHAC"))
    ok = bad == 0 and same
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
