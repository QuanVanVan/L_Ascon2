"""
cost.py -- Phan tich chi phi trien khai tang tuyen tinh (khong phu thuoc may do):
  * XOR2      : so cong XOR 2 dau vao trong phan cung (xoay = doi day, mien phi).
  * depth     : do sau logic = so tang XOR2 tren duong gang dai nhat (quyet dinh tre to hop).
  * x86-64    : so lenh (BMI2: RORX 1 lenh, XOR 1 lenh; xoay 0 bo qua).
  * ARMv8     : so lenh (EOR voi toan hang thu hai xoay bang barrel shifter = 1 lenh; ROR = 1 lenh).
  * XOR/bit   : so XOR2 / 320.
Moi chuong trinh duoc mo ta bang danh sach phep "dst <- src1 ^ rot(src2, r)" tren cac thanh ghi 64 bit.
"""
import sys


def analyze(name, prog, n_out_rot=0, xor3=None):
    """prog: list (dst, a, b, r) nghia la dst = a ^ rot(b, r); thanh ghi la chuoi.
    n_out_rot: so lenh xoay doc lap (khong gop duoc vao EOR)."""
    depth = {}
    for (dst, a, b, r) in prog:
        depth[dst] = max(depth.get(a, 0), depth.get(b, 0)) + 1
    xor2 = len(prog) * 64
    x86 = sum(1 + (1 if r % 64 else 0) for (_, _, _, r) in prog) + n_out_rot
    arm = len(prog) + n_out_rot
    d = max(depth.values()) if depth else 0
    return {"name": name, "XOR2": xor2, "XOR/bit": xor2 / 320, "depth": d, "x86_instr": x86, "armv8_instr": arm}


def ascon():
    R = [(19, 28), (61, 39), (1, 6), (10, 17), (7, 41)]
    prog = []
    for i, (a, b) in enumerate(R):
        # t = x ^ rot(x,a) ; y = t ^ rot(x,b)  (do sau 2; x86: 2 RORX + 2 XOR)
        prog.append((f"t{i}", f"x{i}", f"x{i}", 64 - a))
        prog.append((f"y{i}", f"t{i}", f"x{i}", 64 - b))
    return analyze("Ascon p_L", prog)


def hsilu():
    n1 = [0, 3, 1, 0, 5, 5, 7, 11, 12, 12, 13, 14, 15]
    n2 = [4, 4, 2, 1, 2, 3, 6, 8, 9, 6, 10, 11, 9]
    m = [47, 51, 54, 30, 4, 55, 0, 14, 59, 4, 10, 45, 53]
    prog = [(f"x{5 + t}", f"x{n1[t]}", f"x{n2[t]}", m[t]) for t in range(13)]
    return analyze("Hsilu MRM", prog, n_out_rot=4)


def smrm(ops, d, name="SMRM"):
    """Chuong trinh SMRM: B^bT (dao thu tu, x_j ^= rot(x_i)), D, roi B (x_i ^= rot(x_j)).
    Theo doi phien ban thanh ghi de tinh do sau chinh xac (in-place)."""
    ver = {f"x{w}": f"x{w}" for w in range(5)}
    prog = []
    cnt = [0]

    def new(w):
        cnt[0] += 1
        v = f"x{w}_{cnt[0]}"
        return v
    for (i, j, r) in reversed(ops):
        dst = new(j)
        prog.append((dst, ver[f"x{j}"], ver[f"x{i}"], r))
        ver[f"x{j}"] = dst
    nrot = sum(1 for w in range(5) if d[w] % 64)
    # D: xoay mien phi trong phan cung; trong phan mem co the gop vao phep ke tiep, o day tinh rieng (bi quan)
    for (i, j, r) in ops:
        dst = new(i)
        prog.append((dst, ver[f"x{i}"], ver[f"x{j}"], r))
        ver[f"x{i}"] = dst
    return analyze(name, prog, n_out_rot=nrot)


def parse(ops_s, d_s):
    return [tuple(map(int, o.split(","))) for o in ops_s.split(";")], list(map(int, d_s.split(",")))


if __name__ == "__main__":
    rows = [ascon(), hsilu()]
    for arg in sys.argv[1:]:
        nm, o, d = arg.split("|")
        ops, dd = parse(o, d)
        rows.append(smrm(ops, dd, nm))
    print(f"{'layer':22s} {'XOR2':>6s} {'XOR/bit':>8s} {'depth':>6s} {'x86':>5s} {'ARMv8':>6s}")
    for r in rows:
        print(f"{r['name']:22s} {r['XOR2']:6d} {r['XOR/bit']:8.2f} {r['depth']:6d} {r['x86_instr']:5d} {r['armv8_instr']:6d}")
