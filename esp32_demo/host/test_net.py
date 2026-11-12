"""
test_net.py -- doi chieu giao thuc goi tin giua ban C (demo_core.c, qua host_test.exe --seal/--open) va ban Python
(pc/smrm_net.py). Tat dinh (seed co dinh). Phai in "TEST GIAO THUC: TAT CA PASS" truoc khi nap ESP32.

Kiem tra, cho MOI tang (ascon + 6 SMRM):
  A. C dong goi == Python dong goi (trung tung byte), do dai ban ro 0..200
  B. Python mo goi cua C, C mo goi cua Python
  C. lat 1 bit o header / nonce / ban ma / the -> CA HAI ben tu choi
  D. mo bang tang khac -> tu choi; sai chieu (dir) -> tu choi
  E. (Python) chong phat lai: goi cu bi tu choi sau khi da nhan goi moi hon
"""
import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "pc"))
import smrm_net as N  # noqa: E402

EXE = os.path.join(HERE, "host_test.exe")
fails = 0


def c_seal(ptype, layer, seq, session, d, msg):
    r = subprocess.run([EXE, "--seal", str(ptype), N.LAYERS[layer][0], str(seq), session.hex(), str(d), msg.hex()],
                       capture_output=True, text=True, check=True)
    return bytes.fromhex(r.stdout.strip())


def c_open(d, layer, pkt):
    r = subprocess.run([EXE, "--open", str(d), N.LAYERS[layer][0], pkt.hex()], capture_output=True, text=True,
                       check=True).stdout.split()
    if r[0] == "ERR":
        return int(r[1]), None
    return 0, (int(r[1]), int(r[2]), int(r[3]), bytes.fromhex(r[4]), bytes.fromhex(r[5]) if len(r) > 5 else b"")


def py_open(d, layer, pkt):
    try:
        t, l, s, ses, m = N.open_packet(pkt, d, layer)
        return 0, (t, l, s, ses, m)
    except N.NetError as e:
        return e.code, None


def check(cond, what):
    global fails
    if not cond:
        fails += 1
        print("  FAIL:", what)


def main():
    rng = random.Random(20261109)
    for L in range(len(N.LAYERS)):
        other = (L + 1) % len(N.LAYERS)
        n_ok = 0
        for case in range(6):
            mlen = [0, 1, 15, 16, 77, N.MAX_MSG][case]
            msg = bytes(rng.randrange(256) for _ in range(mlen))
            session = bytes(rng.randrange(256) for _ in range(7))
            seq, ptype = rng.randrange(1, 1 << 32), rng.choice([N.TEXT, N.TELEMETRY, N.ACK])
            d = rng.choice([N.DIR_PC_TO_ESP, N.DIR_ESP_TO_PC])
            pc, pp = c_seal(ptype, L, seq, session, d, msg), N.seal(ptype, L, seq, session, d, msg)
            check(pc == pp, f"{N.LAYERS[L][0]} A: goi C != goi Python (mlen={mlen})")
            want = (ptype, L, seq, session, msg)
            check(py_open(d, L, pc) == (0, want), f"{N.LAYERS[L][0]} B: Python khong mo duoc goi C")
            check(c_open(d, L, pp) == (0, want), f"{N.LAYERS[L][0]} B: C khong mo duoc goi Python")
            for pos, part in [(5, "header/seq"), (4, "header/layer"), (N.HDR + 3, "nonce"),
                              (N.HDR + 16, "ban ma"), (len(pp) - 1, "the")]:
                if part == "ban ma" and mlen == 0:
                    continue
                bad = bytearray(pp)
                bad[pos] ^= 1 << rng.randrange(8)
                bad = bytes(bad)
                check(py_open(d, L, bad)[0] != 0, f"{N.LAYERS[L][0]} C: Python chap nhan goi sua {part}")
                check(c_open(d, L, bad)[0] != 0, f"{N.LAYERS[L][0]} C: C chap nhan goi sua {part}")
            check(py_open(d, other, pp)[0] == N.ERR_AUTH and c_open(d, other, pp)[0] == N.ERR_AUTH,
                  f"{N.LAYERS[L][0]} D: mo duoc bang tang {N.LAYERS[other][0]}")
            check(py_open(3 - d, L, pp)[0] == N.ERR_DIR and c_open(3 - d, L, pp)[0] == N.ERR_DIR,
                  f"{N.LAYERS[L][0]} D: khong phat hien sai chieu")
            n_ok += 1
        peer, ses = N.Peer(), bytes(7)
        p1, p2 = (N.seal(N.TEXT, L, s, ses, 1, b"x") for s in (1, 2))
        N.open_packet(p1, 1, L, peer)
        N.open_packet(p2, 1, L, peer)
        try:
            N.open_packet(p1, 1, L, peer)
            check(False, f"{N.LAYERS[L][0]} E: chap nhan goi phat lai")
        except N.NetError as e:
            check(e.code == N.ERR_REPLAY, f"{N.LAYERS[L][0]} E: sai ma loi phat lai")
        print(f"  {N.LAYERS[L][0]:8s} {n_ok} truong hop x (A,B,C,D) + E")
    print("TEST GIAO THUC:", "TAT CA PASS" if fails == 0 else f"{fails} LOI")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
