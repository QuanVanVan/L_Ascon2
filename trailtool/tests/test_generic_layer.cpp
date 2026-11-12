#include "../src/genericLayer.h"
#include "../src/asconParts.h"
#include <fstream>
#include <sstream>
#include <map>
#include <random>
#include <cstdio>
int main(){
  int bad = 0, n = 0;
  GenericLayer G = GenericLayer::fromSpec("ascon-generic");
  std::mt19937_64 rng(7);
  for (int t = 0; t < 100000; t++) {
    State x; for (int i = 0; i < 5; i++) x.setRow(i, rng());
    State a = x, b = x; asconLinearLayer(a); G.applyForward(b);
    State c = x, d = x; asconInverseLinearLayer(c); G.applyInverse(d);
    State e = x, f = x; asconTransposeLinearLayer(e); G.applyTranspose(f);
    State g = x, h = x; asconInverseTransposeLinearLayer(g); G.applyInverseTranspose(h);
    for (int i = 0; i < 5; i++) if (a.getRow(i)!=b.getRow(i)||c.getRow(i)!=d.getRow(i)||e.getRow(i)!=f.getRow(i)||g.getRow(i)!=h.getRow(i)) { bad++; break; }
  }
  printf("ascon-generic vs hard-coded (L, L^-1, L^T, L^-T): %d mismatches / 100000\n", bad);
  std::ifstream fin("smrm_vectors.txt"); std::string line; int bad2 = 0;
  std::map<std::string, GenericLayer> cache;
  while (std::getline(fin, line)) {
    std::stringstream ss(line); std::string name; ss >> name;
    if (!cache.count(name)) cache[name] = GenericLayer::fromSpec("file:../../results/02_finalists/finalists.txt#" + name);
    uint64_t v[15]; for (int k = 0; k < 15; k++) ss >> std::hex >> v[k];
    State x, y; for (int i = 0; i < 5; i++) { x.setRow(i, v[i]); }
    State z = x; y = x; cache[name].applyForward(y); cache[name].applyInverse(z);
    for (int i = 0; i < 5; i++) if (y.getRow(i) != v[5+i] || z.getRow(i) != v[10+i]) { bad2++; break; }
    n++;
  }
  printf("SMRM C++ vs Python layers.py (forward and inverse): %d mismatches / %d vectors (%zu layers)\n", bad2, n, cache.size());
  // matrix:<file.bin> (polynomial form, inverse by Gauss-Jordan) == program form, all four maps
  int bad3 = 0, nm = 0;
  const char* names[] = { "ascon", "SMRM-5", "SMRM-6", "SMRM-7", "SMRM-8", "L_Ascon2", "SMRM-8L" };
  for (const char* nmname : names) {
    std::string spec = std::string(nmname) == "ascon" ? "ascon" : "file:../../results/02_finalists/finalists.txt#" + std::string(nmname);
    GenericLayer P = GenericLayer::fromSpec(spec);
    GenericLayer M = GenericLayer::fromSpec("matrix:../../results/mat/" + std::string(nmname) + ".bin");
    for (int t = 0; t < 2000; t++) {
      State x; for (int i = 0; i < 5; i++) x.setRow(i, rng());
      State a = x, b = x, c = x, d = x, e = x, f = x, g = x, h = x;
      P.applyForward(a); M.applyForward(b); P.applyInverse(c); M.applyInverse(d);
      P.applyTranspose(e); M.applyTranspose(f); P.applyInverseTranspose(g); M.applyInverseTranspose(h);
      for (int i = 0; i < 5; i++)
        if (a.getRow(i) != b.getRow(i) || c.getRow(i) != d.getRow(i) || e.getRow(i) != f.getRow(i) || g.getRow(i) != h.getRow(i)) { bad3++; break; }
    }
    nm++;
  }
  printf("matrix:<bin> layer vs program layer (L, L^-1, L^T, L^-T): %d mismatches (%d layers x 2000 vectors)\n", bad3, nm);
  return bad || bad2 || bad3;
}
