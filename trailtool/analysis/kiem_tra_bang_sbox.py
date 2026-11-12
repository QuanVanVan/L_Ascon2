# Kiem tra bang trong so (DDT/LAT) viet cung trong AsconTrailTool, tinh lai tu S-box Ascon (dac ta bitslice).
import math
def S(v):
    x=[(v>>(4-i))&1 for i in range(5)]
    x0,x1,x2,x3,x4=x
    x0^=x4; x4^=x3; x2^=x1
    t=[(1-x[i])&0 for i in range(5)]
    y=[x0,x1,x2,x3,x4]
    t=[(1^y[i])&y[(i+1)%5] for i in range(5)]
    y=[y[i]^t[(i+1)%5] for i in range(5)]
    y[1]^=y[0]; y[0]^=y[4]; y[3]^=y[2]; y[2]^=1
    return sum(y[i]<<(4-i) for i in range(5))
sb=[S(v) for v in range(32)]
print([hex(s) for s in sb[:8]])  # expect 4,b,1f,14,1a,15,9,2
ddt=[[0]*32 for _ in range(32)]
for x in range(32):
  for d in range(32): ddt[d][sb[x]^sb[x^d]]+=1
wd=[0]+[int(-math.log2(max(ddt[d][1:])/32)) if False else None for d in range(1,32)]
# weight of input diff d = log2(32/ #nonzero DP equal) : degree 2 => all nonzero entries equal
wd=[0]+[int(math.log2(32/max(ddt[d][1:]))) for d in range(1,32)]
WD=[0,3,3,3,2,3,4,3,3,4,4,4,2,4,3,3,2,2,4,2,3,3,4,3,4,3,4,4,3,3,4,3]
print("WD ok",wd==WD)
mr=[0]+[min(wd[d] for d in range(1,32) if ddt[d][a]) for a in range(1,32)]
print("minrevD ok", mr==[0,2,2,3,2,3,2,3,2,2,2,2,2,3,2,3,2,2,3,2,3,2,2,2,2,2,2,3,3,3,2,3])
par=lambda v:bin(v).count('1')&1
lat=[[sum((-1)**(par(a&x)^par(b&sb[x])) for x in range(32)) for b in range(32)] for a in range(32)]
# linear: weight of output mask b = -log2 C^2 ; C=lat/32
wl=[0]+[int(round(-2*math.log2(max(abs(lat[a][b]) for a in range(32))/32))) for b in range(1,32)]
print("WL ok", wl==[0,2,2,4,2,4,2,2,2,4,4,4,4,4,4,4,2,2,4,4,4,4,4,4,4,2,4,4,4,2,4,4])
mrl=[0]+[min(wl[b] for b in range(1,32) if lat[a][b]) for a in range(1,32)]
print("minrevL ok", mrl==[0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,2,2,2,2,2,2,2,2,2,2,2])
