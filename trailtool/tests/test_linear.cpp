// Kiem tra: L(Linv(x)) = x, LT(LinvT(x)) = x, <L x, y> = <x, LT y> tren vector ngau nhien (seed co dinh)
#include "../upstream/reference/C++/src/asconParts.h"   // ham tuyen tinh GOC
#include <random>
#include <cstdio>
static int dot(const State& a, const State& b){ int p=0; for(int i=0;i<5;i++) p ^= __builtin_popcountll(a.getRow(i)&b.getRow(i))&1; return p; }
int main(){
  std::mt19937_64 rng(12345); int bad1=0,bad2=0,bad3=0,bad4=0;
  for(int t=0;t<100000;t++){
    State x,y; for(int i=0;i<5;i++){ x.setRow(i,rng()); y.setRow(i,rng()); }
    State a=x; asconInverseLinearLayer(a); asconLinearLayer(a); for(int i=0;i<5;i++) if(a.getRow(i)!=x.getRow(i)) {bad1++;break;}
    State b=x; asconInverseTransposeLinearLayer(b); asconTransposeLinearLayer(b); for(int i=0;i<5;i++) if(b.getRow(i)!=x.getRow(i)) {bad2++;break;}
    State lx=x; asconLinearLayer(lx); State lty=y; asconTransposeLinearLayer(lty); if(dot(lx,y)!=dot(x,lty)) bad3++;
    State c=x; asconLinearLayer(c); asconInverseLinearLayer(c); for(int i=0;i<5;i++) if(c.getRow(i)!=x.getRow(i)) {bad4++;break;}
  }
  printf("L*Linv!=I: %d  LT*LinvT!=I: %d  <Lx,y>!=<x,LTy>: %d  Linv*L!=I: %d  (of 100000)\n",bad1,bad2,bad3,bad4);
  return (bad1|bad2|bad3|bad4)!=0;
}
