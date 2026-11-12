/*
 * ascon_tool.c -- chuong trinh demo / kiem thu cho ascon_aead.c
 *
 *   ascon_tool kat  <tep_KAT> [tang]          kiem tra ma hoa, giai ma, tu choi the gia tren moi vector cua tep
 *   ascon_tool gen  <tep_ra>  [tang]          sinh tep KAT (luoi PT 0..32 x AD 0..32, khoa/nonce nhu tep chinh thuc)
 *   ascon_tool demo [tang] [ban_ro] [AD]      minh hoa ma hoa -> giai ma -> thu sua du lieu
 *
 * tang: ascon (mac dinh) | smrm:<ops>|<d> | TEN trong results/02_finalists/finalists.txt (vd. L_Ascon2)
 * Tep finalists: tim theo duong dan cua chuong trinh (../../results/02_finalists/finalists.txt), co the doi bang
 * bien moi truong ASCON_FINALISTS.
 */
#include "ascon_aead.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_finalists[1024];

static void locate_finalists(const char *argv0)
{
    const char *env = getenv("ASCON_FINALISTS");
    if (env) { snprintf(g_finalists, sizeof g_finalists, "%s", env); return; }
    char dir[900];
    snprintf(dir, sizeof dir, "%s", argv0);
    char *p = strrchr(dir, '/'), *q = strrchr(dir, '\\');
    if (q > p) p = q;
    if (p) *p = 0; else snprintf(dir, sizeof dir, ".");
    snprintf(g_finalists, sizeof g_finalists, "%s/../../results/02_finalists/finalists.txt", dir);
}

static void load_layer(linear_layer *L, const char *spec)
{
    if (layer_parse(L, spec, g_finalists)) {
        fprintf(stderr, "khong doc duoc tang '%s' (finalists: %s)\n", spec, g_finalists);
        exit(2);
    }
}

static void describe(const linear_layer *L)
{
    if (L->is_ascon) { printf("Tang tuyen tinh: ascon (p_L goc, 10 XOR 64 bit = 2.0 XOR/bit)\n"); return; }
    printf("Tang tuyen tinh: %s = smrm:", L->name);
    for (int l = 0; l < L->k; l++) printf("%d,%d,%d%s", L->i[l], L->j[l], L->r[l], l + 1 < L->k ? ";" : "");
    printf("|%d,%d,%d,%d,%d  (%d XOR 64 bit = %.1f XOR/bit)\n", L->d[0], L->d[1], L->d[2], L->d[3], L->d[4],
           2 * L->k, 2 * L->k * 64 / 320.0);
}

static void hex(const char *label, const uint8_t *b, size_t n)
{
    printf("%s", label);
    for (size_t t = 0; t < n; t++) printf("%02X", b[t]);
    printf("%s\n", n ? "" : "(rong)");
}

static size_t unhex(uint8_t *out, const char *s)
{
    size_t n = 0;
    unsigned v;
    while (s[0] && s[1] && sscanf(s, "%2x", &v) == 1) { out[n++] = (uint8_t)v; s += 2; }
    return n;
}

/* ------------------------------------------------------------------ kat */
static int cmd_kat(const char *path, const linear_layer *L)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 2; }
    char line[4096];
    uint8_t key[16], nonce[16], pt[256], ad[256], ct[512], out[512], back[256];
    size_t lp = 0, la = 0, lc = 0;
    int count = 0, total = 0, bad = 0, have = 0;
    for (;;) {
        char *got = fgets(line, sizeof line, f);
        line[strcspn(line, "\r\n")] = 0;
        if (!got || !line[0]) {
            if (have) {
                total++;
                size_t n = ascon_aead_encrypt(L, out, pt, lp, ad, la, nonce, key);
                int ok = n == lc && !memcmp(out, ct, lc);
                ok &= ascon_aead_decrypt(L, back, ct, lc, ad, la, nonce, key) == 0 && !memcmp(back, pt, lp);
                ct[lc - 1] ^= 1;
                ok &= ascon_aead_decrypt(L, back, ct, lc, ad, la, nonce, key) != 0;
                if (!ok) { bad++; printf("FAIL Count = %d\n", count); }
                have = 0;
            }
            if (!got) break;
            continue;
        }
        char *eq = strstr(line, " = ");
        const char *v = eq ? eq + 3 : "";
        if (!strncmp(line, "Count", 5)) { count = atoi(v); have = 1; }
        else if (!strncmp(line, "Key", 3)) unhex(key, v);
        else if (!strncmp(line, "Nonce", 5)) unhex(nonce, v);
        else if (!strncmp(line, "PT", 2)) lp = unhex(pt, v);
        else if (!strncmp(line, "AD", 2)) la = unhex(ad, v);
        else if (!strncmp(line, "CT", 2)) lc = unhex(ct, v);
    }
    fclose(f);
    printf("%s (C): %d/%d vector dung (ma hoa, giai ma, tu choi the gia) -- %s\n",
           L->is_ascon ? "Ascon-AEAD128" : L->name, total - bad, total, path);
    printf("%s\n", bad == 0 && total > 0 ? "PASS" : "FAIL");
    return bad == 0 && total > 0 ? 0 : 1;
}

/* ------------------------------------------------------------------ gen */
static int cmd_gen(const char *path, const linear_layer *L)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return 2; }
    uint8_t key[16], nonce[16], pt[32], ad[32], ct[48];
    for (int t = 0; t < 16; t++) { key[t] = (uint8_t)t; nonce[t] = (uint8_t)(16 + t); }
    for (int t = 0; t < 32; t++) { pt[t] = (uint8_t)(0x20 + t); ad[t] = (uint8_t)(0x30 + t); }
    int count = 1;
    for (int lp = 0; lp <= 32; lp++)
        for (int la = 0; la <= 32; la++) {
            size_t n = ascon_aead_encrypt(L, ct, pt, lp, ad, la, nonce, key);
            fprintf(f, "Count = %d\nKey = ", count++);
            for (int t = 0; t < 16; t++) fprintf(f, "%02X", key[t]);
            fprintf(f, "\nNonce = ");
            for (int t = 0; t < 16; t++) fprintf(f, "%02X", nonce[t]);
            fprintf(f, "\nPT = ");
            for (int t = 0; t < lp; t++) fprintf(f, "%02X", pt[t]);
            fprintf(f, "\nAD = ");
            for (int t = 0; t < la; t++) fprintf(f, "%02X", ad[t]);
            fprintf(f, "\nCT = ");
            for (size_t t = 0; t < n; t++) fprintf(f, "%02X", ct[t]);
            fprintf(f, "\n\n");
        }
    fclose(f);
    printf("Da ghi %d vector vao %s\n", count - 1, path);
    return 0;
}

/* ------------------------------------------------------------------ demo */
static int cmd_demo(const linear_layer *L, const char *msg, const char *adtext)
{
    uint8_t key[16], nonce[16];
    for (int t = 0; t < 16; t++) { key[t] = (uint8_t)t; nonce[t] = (uint8_t)(16 + t); }
    const uint8_t *m = (const uint8_t *)msg, *ad = (const uint8_t *)adtext;
    size_t mlen = strlen(msg), adlen = strlen(adtext);
    uint8_t *c = malloc(mlen + 16), *back = malloc(mlen + 1), *bad = malloc(mlen + 16);
    int fails = 0;

    describe(L);
    hex("Khoa  K = ", key, 16);
    hex("Nonce N = ", nonce, 16);
    printf("AD      = \"%s\"\n", adtext);
    printf("Ban ro  = \"%s\" (%zu byte)\n", msg, mlen);

    size_t clen = ascon_aead_encrypt(L, c, m, mlen, ad, adlen, nonce, key);
    printf("\n[1] Ma hoa\n");
    hex("    Ban ma C = ", c, mlen);
    hex("    The    T = ", c + mlen, 16);

    printf("[2] Giai ma voi dung K, N, AD\n");
    int rc = ascon_aead_decrypt(L, back, c, clen, ad, adlen, nonce, key);
    back[mlen] = 0;
    int same = rc == 0 && !memcmp(back, m, mlen);
    printf("    the hop le: %s; ban ro thu duoc: \"%s\" -> %s\n", rc == 0 ? "CO" : "KHONG", rc == 0 ? (char *)back : "",
           same ? "TRUNG ban ro goc" : "SAI");
    fails += !same;

    printf("[3] Thu sua du lieu (moi truong hop phai bi TU CHOI)\n");
    struct { const char *what; int pos; } cases[] = { { "lat bit 0 cua byte dau ban ma", 0 },
                                                      { "lat bit 0 cua byte cuoi cua the", (int)clen - 1 } };
    for (int t = 0; t < 2; t++) {
        if (cases[t].pos >= (int)clen || (t == 0 && mlen == 0)) continue;
        memcpy(bad, c, clen); bad[cases[t].pos] ^= 1;
        rc = ascon_aead_decrypt(L, back, bad, clen, ad, adlen, nonce, key);
        printf("    %-38s -> %s\n", cases[t].what, rc ? "tu choi (dung)" : "CHAP NHAN (SAI!)");
        fails += rc == 0;
    }
    rc = ascon_aead_decrypt(L, back, c, clen, (const uint8_t *)"AD khac", 7, nonce, key);
    printf("    %-38s -> %s\n", "doi AD", rc ? "tu choi (dung)" : "CHAP NHAN (SAI!)");
    fails += rc == 0;
    nonce[15] ^= 0x80;
    rc = ascon_aead_decrypt(L, back, c, clen, ad, adlen, nonce, key);
    printf("    %-38s -> %s\n", "doi 1 bit nonce", rc ? "tu choi (dung)" : "CHAP NHAN (SAI!)");
    fails += rc == 0;
    nonce[15] ^= 0x80;
    linear_layer other;
    layer_parse(&other, "ascon", NULL);
    if (!L->is_ascon) {
        rc = ascon_aead_decrypt(&other, back, c, clen, ad, adlen, nonce, key);
        printf("    %-38s -> %s\n", "giai ma bang Ascon goc", rc ? "tu choi (dung)" : "CHAP NHAN (SAI!)");
        fails += rc == 0;
    }
    printf("\n%s\n", fails ? "DEMO: CO LOI" : "DEMO: ma hoa / giai ma / xac thuc DUNG");
    free(c); free(back); free(bad);
    return fails ? 1 : 0;
}

int main(int argc, char **argv)
{
    locate_finalists(argv[0]);
    if (argc < 2) {
        fprintf(stderr, "dung: %s kat <tep> [tang] | gen <tep> [tang] | demo [tang] [ban_ro] [AD]\n", argv[0]);
        return 2;
    }
    linear_layer L;
    if (!strcmp(argv[1], "kat") && argc >= 3) { load_layer(&L, argc > 3 ? argv[3] : "ascon"); return cmd_kat(argv[2], &L); }
    if (!strcmp(argv[1], "gen") && argc >= 3) { load_layer(&L, argc > 3 ? argv[3] : "ascon"); return cmd_gen(argv[2], &L); }
    if (!strcmp(argv[1], "demo")) {
        load_layer(&L, argc > 2 ? argv[2] : "ascon");
        return cmd_demo(&L, argc > 3 ? argv[3] : "Xin chao, day la thong diep thu nghiem cho Ascon-AEAD128!",
                        argc > 4 ? argv[4] : "header: DATN 2026");
    }
    fprintf(stderr, "lenh khong hop le: %s\n", argv[1]);
    return 2;
}
