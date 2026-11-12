"""
gen_smrm_c.py -- Sinh ma C chuyen biet (hang so cung, bien thanh ghi) cho tung ung vien SMRM,
nhu mot cai dat thuc te. Xuat src/smrm_gen.h gom:
  lin_<ten>(state_t*), lin_<ten>_inv(state_t*)  va bang FUNCS de perm_bench2.c benchmark + kiem tra cheo.
Su dung: python gen_smrm_c.py ../results/02_finalists/finalists.txt
"""
import sys


def gen(name, ops, d):
    cid = name.replace("-", "_")
    L = [f"static inline void lin_{cid}(state_t *s) {{",
         "    uint64_t x0 = s->x[0], x1 = s->x[1], x2 = s->x[2], x3 = s->x[3], x4 = s->x[4];"]
    for (i, j, r) in reversed(ops):
        L.append(f"    x{j} ^= ROTL(x{i}, {r});")
    for w in range(1, 5):
        if d[w] % 64:
            L.append(f"    x{w} = ROTL(x{w}, {d[w]});")
    for (i, j, r) in ops:
        L.append(f"    x{i} ^= ROTL(x{j}, {r});")
    L.append("    s->x[0] = x0; s->x[1] = x1; s->x[2] = x2; s->x[3] = x3; s->x[4] = x4;\n}")
    L.append(f"static inline void lin_{cid}_inv(state_t *s) {{")
    L.append("    uint64_t x0 = s->x[0], x1 = s->x[1], x2 = s->x[2], x3 = s->x[3], x4 = s->x[4];")
    for (i, j, r) in reversed(ops):
        L.append(f"    x{i} ^= ROTL(x{j}, {r});")
    for w in range(1, 5):
        if d[w] % 64:
            L.append(f"    x{w} = ROTL(x{w}, {(64 - d[w]) % 64});")
    for (i, j, r) in ops:
        L.append(f"    x{j} ^= ROTL(x{i}, {r});")
    L.append("    s->x[0] = x0; s->x[1] = x1; s->x[2] = x2; s->x[3] = x3; s->x[4] = x4;\n}")
    return cid, "\n".join(L)


def main():
    rows = [l.strip().split("|") for l in open(sys.argv[1]) if l.strip()]
    out = ["/* SINH TU DONG boi gen_smrm_c.py -- khong sua tay */"]
    ents = []
    for nm, o, d in rows:
        ops = [tuple(map(int, t.split(","))) for t in o.split(";")]
        dd = list(map(int, d.split(",")))
        cid, code = gen(nm, ops, dd)
        out.append(code)
        ents.append(f'    {{"{nm}", lin_{cid}, lin_{cid}_inv, "{o}", "{d}"}},')
    out.append("typedef struct { const char *name; void (*f)(state_t *); void (*fi)(state_t *); const char *ops, *d; } gen_t;")
    out.append("static const gen_t FUNCS[] = {\n" + "\n".join(ents) + "\n};")
    out.append(f"#define NFUNCS {len(ents)}")
    open("smrm_gen.h", "w").write("\n".join(out) + "\n")
    print(f"wrote smrm_gen.h with {len(ents)} layers")


if __name__ == "__main__":
    main()
