/*
 * ascon_aead.h -- Ascon-AEAD128 (NIST SP 800-232) voi tang tuyen tinh p_L thay the duoc (cai dat C, doc lap voi Python).
 * Quy uoc giong python/ascon_aead.py: trang thai 5 tu 64 bit, rotl = xoay trai, du lieu little-endian.
 */
#ifndef ASCON_AEAD_H
#define ASCON_AEAD_H

#include <stddef.h>
#include <stdint.h>

#define ASCON_KEY_BYTES 16
#define ASCON_NONCE_BYTES 16
#define ASCON_TAG_BYTES 16
#define SMRM_MAX_OPS 16

typedef struct {
    int is_ascon;                       /* 1: p_L goc cua Ascon; 0: SMRM */
    int k;                              /* so phep transvection cua B */
    int i[SMRM_MAX_OPS], j[SMRM_MAX_OPS], r[SMRM_MAX_OPS];
    int d[5];
    char name[64];
} linear_layer;

/* spec: "ascon" | "smrm:<ops>|<d>" | <TEN> (tra trong tep finalists). Tra 0 neu thanh cong. */
int layer_parse(linear_layer *L, const char *spec, const char *finalists_path);
void layer_apply(const linear_layer *L, uint64_t s[5]);
void ascon_permutation(uint64_t s[5], int rounds, const linear_layer *L);

/* c phai co cho mlen + 16 byte; tra ve do dai c. */
size_t ascon_aead_encrypt(const linear_layer *L, uint8_t *c, const uint8_t *m, size_t mlen,
                          const uint8_t *ad, size_t adlen, const uint8_t *nonce, const uint8_t *key);
/* Tra 0 va ghi m (clen - 16 byte) neu the dung; tra -1 va xoa m neu the sai. */
int ascon_aead_decrypt(const linear_layer *L, uint8_t *m, const uint8_t *c, size_t clen,
                       const uint8_t *ad, size_t adlen, const uint8_t *nonce, const uint8_t *key);

#endif
