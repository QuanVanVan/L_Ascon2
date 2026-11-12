/* Tang tuyen tinh L_Ascon2: L = B * D * B^bT, B = E6...E1 (Chuong 2).
 * Mot buoc "x[j] ^= ROL(x[i], r)" la mot phep bien doi so cap E(i,j,r).
 * ROL(x,r) = ROR(x, 64-r). Tong cong 12 XOR + 16 phep xoay tren 5 tu 64 bit. */
#define ROL64(x, r) ROR((x), 64 - (r))
#define L_ASCON2_LAYER(x)               \
  do {                                \
    /* B^bT: E1^T ... E6^T */         \
    (x)[4] ^= ROL64((x)[1], 21);      \
    (x)[1] ^= ROL64((x)[0], 22);      \
    (x)[0] ^= ROL64((x)[2], 15);      \
    (x)[2] ^= ROL64((x)[3], 23);      \
    (x)[3] ^= ROL64((x)[4], 31);      \
    (x)[4] ^= ROL64((x)[1], 45);      \
    /* D = diag(1, X^39, X^2, X^16, X^7) */ \
    (x)[1] = ROL64((x)[1], 39);       \
    (x)[2] = ROL64((x)[2], 2);        \
    (x)[3] = ROL64((x)[3], 16);       \
    (x)[4] = ROL64((x)[4], 7);        \
    /* B: E6 ... E1 */                \
    (x)[1] ^= ROL64((x)[4], 45);      \
    (x)[4] ^= ROL64((x)[3], 31);      \
    (x)[3] ^= ROL64((x)[2], 23);      \
    (x)[2] ^= ROL64((x)[0], 15);      \
    (x)[0] ^= ROL64((x)[1], 22);      \
    (x)[1] ^= ROL64((x)[4], 21);      \
  } while (0)
