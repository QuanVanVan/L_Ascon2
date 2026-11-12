# make_vectors.py -- sinh vector kiem thu SMRM tu ban cai dat Python THAM CHIEU (smrm/layers.py)
# cho 6 ung vien chung ket; test_generic_layer.cpp doi chieu ban C++ (trailtool/src/genericLayer.cpp).
# Dau ra: tests/smrm_vectors.txt, moi dong: NAME x0..x4 L(x)0..4 Linv(x)0..4 (hex). Seed co dinh 2026.
import os, sys, random
HERE = os.path.dirname(os.path.abspath(__file__))
PRJ = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(PRJ, 'smrm'))
from layers import SMRM
rng = random.Random(2026)
with open(os.path.join(HERE, 'smrm_vectors.txt'), 'w') as f:
    for line in open(os.path.join(PRJ, 'results', '02_finalists', 'finalists.txt')):
        n, ops, d = line.strip().split('|')
        L = SMRM([tuple(map(int, o.split(','))) for o in ops.split(';')], list(map(int, d.split(','))), n)
        for _ in range(200):
            x = [rng.getrandbits(64) for _ in range(5)]
            f.write(n + ' ' + ' '.join('%x' % v for v in x + L(x) + L.inverse(x)) + '\n')
print('written', os.path.join(HERE, 'smrm_vectors.txt'))
