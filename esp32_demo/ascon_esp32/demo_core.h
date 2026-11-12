/*
 * demo_core.h -- loi demo Ascon-AEAD128 / Ascon-SMRM, khong phu thuoc nen tang (ESP32 hoac PC).
 * Nen tang phai cung cap 5 ham plat_* ben duoi (ascon_esp32.ino cho ESP32, host/host_main.c cho PC).
 */
#ifndef DEMO_CORE_H
#define DEMO_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;                       /* "ascon", "L_Ascon2", ... */
    const char *spec;                       /* "ascon" | "smrm:<ops>|<d>" (giong ascon_aead_demo) */
    int xor64;                              /* so XOR 64 bit cua p_L */
    void (*pl)(uint64_t *s);                /* chi p_L */
    void (*perm64)(uint64_t *s, int rounds);
    void (*perm32)(uint32_t *s, int rounds);/* bit-interleaved */
    const char *kat_sha256;                 /* SHA-256 cua tep KAT do PC sinh (C = Python) */
} layer_t;

/* ---- nen tang cung cap */
uint32_t plat_cycles(void);                 /* bo dem chu ky CPU (tran so 32 bit duoc phep) */
uint32_t plat_cpu_mhz(void);
const char *plat_name(void);
void plat_print(const char *s);
void plat_yield(void);                      /* nhuong CPU (tranh watchdog) */

/* ---- loi */
int demo_selftest(void);                    /* KAT + doi chieu perm64/perm32; tra so loi */
void demo_bench(void);
/* Xu ly mot dong lenh tu cong noi tiep: "/kat", "/bench", "/layer <ten>", "/list", "/help", hoac ban ro tuy y. */
void demo_command(const char *line);
void demo_banner(void);

/* ---- chon tang (chi so = thu tu trong LAYERS: 0 = ascon, roi finalists.txt; PC dung cung thu tu, xem pc/layers_spec.py) */
int demo_nlayers(void);
const layer_t *demo_layer(int idx);
int demo_find_layer(const char *name);
int demo_layer_index(void);                 /* tang hien tai */
void demo_set_layer(int idx);

/* ---- giao thuc goi tin ESP32 <-> PC (UDP), dac ta: esp32_demo/GIAO_THUC.md; ban Python: pc/smrm_net.py
 * goi = header 9 byte | nonce 16 | C | T 16;  header = 'S' 'M' ver type layer seq(uint32 LE), dung lam AD
 * nonce = dir | session(7) | seq(uint32 LE) | 00 00 00 00                                                     */
#define NET_HDR 9
#define NET_MAX_MSG 200
#define NET_MAX_PKT (NET_HDR + 16 + NET_MAX_MSG + 16)
enum { NET_TEXT = 1, NET_TELEMETRY = 2, NET_ACK = 3, NET_NACK = 4 };          /* NACK: van ban ro, KHONG xac thuc */
enum { NET_DIR_PC_TO_ESP = 1, NET_DIR_ESP_TO_PC = 2 };
enum { NET_OK = 0, NET_ERR_FORMAT = -1, NET_ERR_DIR = -2, NET_ERR_AUTH = -3, NET_ERR_REPLAY = -4 };

typedef struct { uint8_t type, layer; uint32_t seq; uint8_t session[7]; } net_hdr_t;
typedef struct { int valid; uint8_t session[7]; uint32_t last_seq; } net_peer_t;   /* chong phat lai */

/* Dong goi (ma hoa bang tang layer_idx); tra do dai goi, 0 neu loi tham so. pkt can NET_MAX_PKT byte. */
size_t net_seal(uint8_t *pkt, int type, int layer_idx, uint32_t seq, const uint8_t session[7], uint8_t dir,
                const uint8_t *msg, size_t mlen);
/* Mo goi bang tang CUA BEN NHAN (layer_idx), khong tin truong layer trong header. peer = NULL: bo qua chong phat lai.
 * Tra NET_OK hoac NET_ERR_*; msg can NET_MAX_MSG byte. */
int net_open(const uint8_t *pkt, size_t n, uint8_t dir, int layer_idx, net_peer_t *peer, net_hdr_t *h,
             uint8_t *msg, size_t *mlen);
const char *net_strerror(int code);

#ifdef __cplusplus
}
#endif
#endif
