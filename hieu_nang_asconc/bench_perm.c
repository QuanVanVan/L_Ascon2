/* Do so chu ky TSC cua mot lan goi Ascon-p[12] va Ascon-p[8] (ban opt64 cua ascon-c). */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <x86intrin.h>
#include "api.h"
#include "ascon.h"
#include "permutations.h"
#ifdef OPT64
#define DO12 P12ROUNDS
#define DO8 P8ROUNDS
#else
#define DO12 P12
#define DO8 P8
#endif
__attribute__((noinline)) static void p12(ascon_state_t*s){DO12(s);}
__attribute__((noinline)) static void p8(ascon_state_t*s){DO8(s);}
#define N 100000
#define RUNS 31
static int cmp(const void*a,const void*b){double x=*(double*)a,y=*(double*)b;return (x>y)-(x<y);}
int main(void){
  ascon_state_t s; for(int i=0;i<5;i++) s.x[i]=0x0123456789abcdefULL*(i+1);
  double r12[RUNS], r8[RUNS];
  for(int r=0;r<RUNS;r++){
    uint64_t t0=__rdtsc(); for(int i=0;i<N;i++) p12(&s); uint64_t t1=__rdtsc();
    r12[r]=(double)(t1-t0)/N;
    t0=__rdtsc(); for(int i=0;i<N;i++) p8(&s); t1=__rdtsc();
    r8[r]=(double)(t1-t0)/N;
  }
  qsort(r12,RUNS,sizeof(double),cmp); qsort(r8,RUNS,sizeof(double),cmp);
  printf("P12: min %.1f  median %.1f cycles/call  (%.2f cycles/round)\n", r12[0], r12[RUNS/2], r12[RUNS/2]/12);
  printf("P8 : min %.1f  median %.1f cycles/call  (%.2f cycles/round)\n", r8[0], r8[RUNS/2], r8[RUNS/2]/8);
  printf("checksum %016llx\n",(unsigned long long)(s.x[0]^s.x[1]^s.x[2]^s.x[3]^s.x[4]));
  return 0;
}
