"""
gen_layers.py -- Sinh ascon_esp32/layers_gen.h cho firmware ESP32 (va ban kiem thu tren PC).

Nguon tham so DUY NHAT: results/02_finalists/finalists.txt (+ p_L goc cua Ascon).
Voi moi tang sinh ma chuyen biet (hang so cung, bien thanh ghi):
  pl_<id>      (uint64_t s[5])             chi tang tuyen tinh p_L
  perm64_<id>  (uint64_t s[5], int rounds) hoan vi Ascon-p, tu 64 bit
  perm32_<id>  (uint32_t s[10], int rounds) hoan vi Ascon-p, bit-interleaved 32 bit (s[2w] = bit chan, s[2w+1] = bit le
                                            cua tu w) -- cach cai dat Ascon tham chieu dung cho CPU 32 bit
Gia tri KAT mong doi = SHA-256 cua ascon_aead_demo/results/c_<TEN>_KAT.txt; script KIEM TRA tep C trung tep Python,
va voi Ascon trung tep KAT chinh thuc, roi moi ghi.

Chay: python gen_layers.py      (tu thu muc bat ky)
"""
import hashlib
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FINALISTS = os.path.join(ROOT, "results", "02_finalists", "finalists.txt")
AEAD_RES = os.path.join(ROOT, "ascon_aead_demo", "results")
OFFICIAL_KAT = os.path.join(ROOT, "ascon_aead_demo", "kat", "LWC_AEAD_KAT_128_128.txt")
OUT = os.path.join(HERE, "ascon_esp32", "layers_gen.h")
PY_AEAD = os.path.join(ROOT, "ascon_aead_demo", "python", "ascon_aead.py")
PC_AEAD = os.path.join(HERE, "pc", "ascon_aead.py")
PC_SPEC = os.path.join(HERE, "pc", "layers_spec.py")

ASCON_ROT = [(19, 28), (61, 39), (1, 6), (10, 17), (7, 41)]   # x_i ^= (x_i >>> a) ^ (x_i >>> b)


def sha256(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def expected_hash(name):
    tag = "ascon" if name == "ascon" else name
    suffix = "_KAT_regen.txt" if name == "ascon" else "_KAT.txt"
    hc = sha256(os.path.join(AEAD_RES, f"c_{tag}{suffix}"))
    hp = sha256(os.path.join(AEAD_RES, f"py_{tag}{suffix}"))
    if hc != hp:
        sys.exit(f"LOI: KAT C va Python cua {name} khac nhau")
    if name == "ascon" and hc != sha256(OFFICIAL_KAT):
        sys.exit("LOI: KAT Ascon sinh lai khac tep chinh thuc")
    return hc


# ------------------------------------------------------------------ bieu thuc bit-interleaved
def ror32(v, k):
    k %= 32
    return v if k == 0 else f"ROR32({v}, {k})"


def ror64_bi(e, o, r):
    """(x >>> r) tren cap (e = bit chan, o = bit le) -> (bieu thuc e', bieu thuc o')."""
    r %= 64
    k = r // 2
    if r % 2 == 0:
        return ror32(e, k), ror32(o, k)
    return ror32(o, k), ror32(e, k + 1)


def rol64_bi(e, o, r):
    return ror64_bi(e, o, (64 - r) % 64)


def interleave_byte(c):
    ce = co = 0
    for b in range(8):
        if (c >> b) & 1:
            if b % 2 == 0:
                ce |= 1 << (b // 2)
            else:
                co |= 1 << (b // 2)
    return ce, co


# ------------------------------------------------------------------ than tang tuyen tinh
def body64(layer):
    if layer["ascon"]:
        return [f"x{i} ^= ROR64(x{i}, {a}) ^ ROR64(x{i}, {b});" for i, (a, b) in enumerate(ASCON_ROT)]
    L = []
    for (i, j, r) in reversed(layer["ops"]):
        L.append(f"x{j} ^= ROL64(x{i}, {r});")
    for w in range(5):
        if layer["d"][w] % 64:
            L.append(f"x{w} = ROL64(x{w}, {layer['d'][w]});")
    for (i, j, r) in layer["ops"]:
        L.append(f"x{i} ^= ROL64(x{j}, {r});")
    return L


def body32(layer):
    if layer["ascon"]:
        L = []
        for i, (a, b) in enumerate(ASCON_ROT):
            ae, ao = ror64_bi("te", "to", a)
            be, bo = ror64_bi("te", "to", b)
            L.append(f"{{ uint32_t te = e{i}, to = o{i}; e{i} = te ^ {ae} ^ {be}; o{i} = to ^ {ao} ^ {bo}; }}")
        return L
    L = []
    for (i, j, r) in reversed(layer["ops"]):
        E, O = rol64_bi(f"e{i}", f"o{i}", r)
        L.append(f"e{j} ^= {E}; o{j} ^= {O};")
    for w in range(5):
        if layer["d"][w] % 64:
            E, O = rol64_bi("te", "to", layer["d"][w])
            L.append(f"{{ uint32_t te = e{w}, to = o{w}; e{w} = {E}; o{w} = {O}; }}")
    for (i, j, r) in layer["ops"]:
        E, O = rol64_bi(f"e{j}", f"o{j}", r)
        L.append(f"e{i} ^= {E}; o{i} ^= {O};")
    return L


def gen_layer(layer):
    cid = layer["name"].replace("-", "_")
    b64 = "\n".join("    " + s for s in body64(layer))
    b64r = "\n".join("        " + s for s in body64(layer))
    b32r = "\n".join("        " + s for s in body32(layer))
    load64 = "    uint64_t x0 = s[0], x1 = s[1], x2 = s[2], x3 = s[3], x4 = s[4];"
    store64 = "    s[0] = x0; s[1] = x1; s[2] = x2; s[3] = x3; s[4] = x4;"
    return f"""
/* ---- {layer['name']}: {layer['spec']} */
static PERM_ATTR void pl_{cid}(uint64_t *s)
{{
{load64}
{b64}
{store64}
}}
static PERM_ATTR void perm64_{cid}(uint64_t *s, int rounds)
{{
{load64}
    for (int r = 12 - rounds; r < 12; r++) {{
        x2 ^= (uint64_t)(0xF0 - 0x0F * r);
        SBOX(x0, x1, x2, x3, x4, uint64_t);
{b64r}
    }}
{store64}
}}
static PERM_ATTR void perm32_{cid}(uint32_t *s, int rounds)
{{
    uint32_t e0 = s[0], o0 = s[1], e1 = s[2], o1 = s[3], e2 = s[4], o2 = s[5], e3 = s[6], o3 = s[7], e4 = s[8], o4 = s[9];
    for (int r = 12 - rounds; r < 12; r++) {{
        e2 ^= RC32[r][0]; o2 ^= RC32[r][1];
        SBOX(e0, e1, e2, e3, e4, uint32_t);
        SBOX(o0, o1, o2, o3, o4, uint32_t);
{b32r}
    }}
    s[0] = e0; s[1] = o0; s[2] = e1; s[3] = o1; s[4] = e2; s[5] = o2; s[6] = e3; s[7] = o3; s[8] = e4; s[9] = o4;
}}
"""


def main():
    layers = [{"name": "ascon", "spec": "ascon", "ascon": True, "xor64": 10}]
    for line in open(FINALISTS):
        line = line.strip()
        if not line:
            continue
        nm, o, d = line.split("|")
        ops = [tuple(map(int, t.split(","))) for t in o.split(";")]
        layers.append({"name": nm, "spec": f"smrm:{o}|{d}", "ascon": False, "ops": ops,
                       "d": list(map(int, d.split(","))), "xor64": 2 * len(ops)})
    for L in layers:
        L["sha"] = expected_hash(L["name"])

    out = ["/* SINH TU DONG boi esp32_demo/gen_layers.py tu results/02_finalists/finalists.txt -- KHONG SUA TAY */",
           "#ifndef LAYERS_GEN_H", "#define LAYERS_GEN_H", "",
           "/* p_S cua Ascon (bitslice), dung chung cho tu 64 bit va nua 32 bit */",
           "#define SBOX(a0, a1, a2, a3, a4, T) do { T t0_, t1_, t2_, t3_, t4_; \\",
           "    a0 ^= a4; a4 ^= a3; a2 ^= a1; \\",
           "    t0_ = ~a0 & a1; t1_ = ~a1 & a2; t2_ = ~a2 & a3; t3_ = ~a3 & a4; t4_ = ~a4 & a0; \\",
           "    a0 ^= t1_; a1 ^= t2_; a2 ^= t3_; a3 ^= t4_; a4 ^= t0_; \\",
           "    a1 ^= a0; a0 ^= a4; a3 ^= a2; a2 = ~a2; } while (0)", "",
           "/* hang so vong 0xF0 - 0x0F*r tach bit chan / le */",
           "static const uint32_t RC32[12][2] = {"]
    out.append("    " + ", ".join("{0x%x, 0x%x}" % interleave_byte(0xF0 - 0x0F * r) for r in range(12)))
    out.append("};")
    for L in layers:
        out.append(gen_layer(L))
    out.append("static const layer_t LAYERS[] = {")
    for L in layers:
        cid = L["name"].replace("-", "_")
        out.append(f'    {{"{L["name"]}", "{L["spec"]}", {L["xor64"]}, pl_{cid}, perm64_{cid}, perm32_{cid},\n'
                   f'     "{L["sha"]}"}},')
    out.append("};")
    out.append(f"#define NLAYERS {len(layers)}")
    out.append("\n#endif")
    with open(OUT, "w", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    print(f"da ghi {OUT}: {len(layers)} tang ({', '.join(L['name'] for L in layers)})")

    # phia PC: cung thu tu tang (chi so tang trong goi tin) + ban sao cai dat Python da kiem chung KAT
    with open(PC_SPEC, "w", newline="\n") as f:
        f.write('"""SINH TU DONG boi esp32_demo/gen_layers.py -- KHONG SUA TAY. Chi so = chi so tang trong goi tin,\n'
                'trung voi LAYERS[] trong ascon_esp32/layers_gen.h."""\n')
        f.write("LAYERS = [\n" + "".join(f'    ("{L["name"]}", "{L["spec"]}"),\n' for L in layers) + "]\n")
    shutil.copyfile(PY_AEAD, PC_AEAD)
    print(f"da ghi {PC_SPEC} va chep {PY_AEAD} -> {PC_AEAD}")


if __name__ == "__main__":
    main()
