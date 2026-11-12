# compare_cores.py fileA fileB : so sanh hai tap loi vet (dinh dang AsconTrailTool) modulo tinh tien cot
import sys
M = (1 << 64) - 1
rot = lambda x, r: ((x >> r) | (x << (64 - r))) & M if r else x
def canon(rows):
    return min(tuple(rot(v, r) for v in rows) for r in range(64))
def load(fn):
    s = {}
    for line in open(fn):
        t = line.split()
        if not t: continue
        nr = int(t[0]); key = tuple(int(v) for v in t[1:3 + nr - 1])
        states = [int(v, 16) for v in t[3 + nr - 1:]]
        assert len(states) == 5 * (nr - 1)
        # canonical over the whole sequence of states (same rotation for all)
        c = min(tuple(rot(v, r) for v in states) for r in range(64))
        k = key + c
        s[k] = s.get(k, 0) + 1
    return s
A, B = load(sys.argv[1]), load(sys.argv[2])
dupA = sum(v - 1 for v in A.values()); dupB = sum(v - 1 for v in B.values())
onlyA = set(A) - set(B); onlyB = set(B) - set(A)
print(f"A: {sum(A.values())} lines ({len(A)} classes, {dupA} duplicates) | B: {sum(B.values())} lines ({len(B)} classes, {dupB} duplicates)")
print(f"only in A: {len(onlyA)}  only in B: {len(onlyB)}  -> {'IDENTICAL SETS' if not onlyA and not onlyB and not dupA and not dupB else 'DIFFERENT'}")
for k in list(onlyA)[:3]: print('  A-only', k[:3])
for k in list(onlyB)[:3]: print('  B-only', k[:3])
