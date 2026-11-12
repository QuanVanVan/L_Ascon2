# Kiem tra cac ham biggerThanTwo/Three(LTS) la can duoi hop le cua trong so cot khi cac hang sau con tu do.
import itertools
WD=[0,3,3,3,2,3,4,3,3,4,4,4,2,4,3,3,2,2,4,2,3,3,4,3,4,3,4,4,3,3,4,3]
WL=[0,2,2,4,2,4,2,2,2,4,4,4,4,4,4,4,2,2,4,4,4,4,4,4,4,2,4,4,4,2,4,4]
def bits(c): return [(c>>(4-r))&1 for r in range(5)]  # r=row, row0=MSB
def f2d(x): return ((1-x[0])&x[4])^((1-x[0])&x[3])^(x[0]&x[2])^(x[0]&x[1])
def f3d(x): return (x[1]&(1-x[2])&x[3])^((1-x[0])&x[1]&(1-x[3])&x[4])
def f2l(x):
    n=lambda v:1-v
    return (x[1]&x[3])^(n(x[2])&x[3]&x[4])^(n(x[0])&x[1]&x[4])^(n(x[0])&x[1]&x[2])^(x[0]&n(x[1])&x[2])^(n(x[0])&x[2]&n(x[3])&x[4])
def val(x): return sum(x[r]<<(4-r) for r in range(5))
for name,W,fs in [("DTS",WD,[f2d,f3d]),("LTS",WL,[f2l])]:
  bad=0
  # free set: rows >= i free (row i partially = free), rows < i fixed
  for i in range(5):
    for incl_i_fixed in (True,False):
      fixed=list(range(i+1)) if incl_i_fixed else list(range(i))
      free=[r for r in range(5) if r not in fixed]
      for fv in itertools.product([0,1],repeat=len(fixed)):
        x=[0]*5
        for r,v in zip(fixed,fv): x[r]=v
        if sum(x)==0: continue
        lb=2+sum(f(x) for f in fs)
        m=min(W[val([fv[fixed.index(r)] if r in fixed else c[free.index(r)] for r in range(5)])] for c in itertools.product([0,1],repeat=len(free)))
        if lb>m: bad+=1; print(name,i,incl_i_fixed,x,lb,m)
  print(name,"violations",bad)
