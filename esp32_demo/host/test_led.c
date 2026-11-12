/*
 * test_led.c -- kiem thu tren PC cho ascon_esp32/led_signal.h (bao hieu bang 1 LED GPIO2 cua firmware).
 * Mo phong thoi gian (ms), kiem tra: NGHI = tat; dung thu tu/thoi gian; LUON tu ve NGHI; cac kieu bao hieu phan biet
 * duoc (so lan chop, thoi gian sang dai nhat); goi moi chen ngang; millis() tran so; vong loop bi chan lau (/bench).
 * Bien dich + chay: build_host.sh (in "TEST LED: TAT CA PASS").
 */
#include <stdio.h>
#include "../ascon_esp32/led_signal.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void expect(led_seq_t *s, uint32_t t, int on, const char *what)
{
    int v = led_seq_update(s, t);
    CHECK(v == on, "%s: t=%lu ms -> LED=%d, mong doi %d", what, (unsigned long)t, v, on);
}

typedef struct { uint32_t end_ms, on_ms, longest_on; int pulses; } stats_t;

/* chay voi buoc 1 ms; dem so lan bat, tong thoi gian sang, lan sang dai nhat; end_ms = luc ve NGHI */
static stats_t run(const led_step_t *steps, int n)
{
    led_seq_t s = { 0 };
    stats_t r = { 0, 0, 0, 0 };
    uint32_t cur = 0;
    int last = 0;
    led_seq_start(&s, steps, n, 0);
    for (uint32_t t = 0;; t++) {
        int v = led_seq_update(&s, t);
        if (v && !last) { r.pulses++; cur = 0; }
        if (v) { r.on_ms++; cur++; if (cur > r.longest_on) r.longest_on = cur; }
        last = v;
        if (!s.active) { r.end_ms = t; return r; }
        if (t > 60000) { r.end_ms = 0xFFFFFFFFu; return r; }     /* khong bao gio ve NGHI */
    }
}

int main(void)
{
    led_seq_t s = { 0 };
    expect(&s, 0, 0, "chua co goi (NGHI)");
    expect(&s, 123456, 0, "chua co goi (NGHI)");

    /* hop le: 2 chop ngan, lang 300 ms, sang 1500 ms, tat */
    led_seq_start(&s, LED_SEQ_OK, LED_SEQ_LEN(LED_SEQ_OK), 1000);
    expect(&s, 1000, 1, "OK: chop 1");
    expect(&s, 1080, 0, "OK: giua 2 chop");
    expect(&s, 1200, 1, "OK: chop 2");
    expect(&s, 1280, 0, "OK: khoang lang");
    expect(&s, 1579, 0, "OK: khoang lang");
    expect(&s, 1580, 1, "OK: sang lien tuc");
    expect(&s, 3079, 1, "OK: sang lien tuc");
    expect(&s, 3080, 0, "OK: ve NGHI");
    CHECK(!s.active, "OK: chua ve NGHI");
    expect(&s, 999999, 0, "OK: van NGHI ve sau");

    stats_t ok = run(LED_SEQ_OK, LED_SEQ_LEN(LED_SEQ_OK));
    stats_t fail = run(LED_SEQ_FAIL, LED_SEQ_LEN(LED_SEQ_FAIL));
    stats_t ls = run(LED_SEQ_LAYER_SMRM, LED_SEQ_LEN(LED_SEQ_LAYER_SMRM));
    stats_t la = run(LED_SEQ_LAYER_ASCON, LED_SEQ_LEN(LED_SEQ_LAYER_ASCON));
    stats_t bo = run(LED_SEQ_BOOT_OK, LED_SEQ_LEN(LED_SEQ_BOOT_OK));
    stats_t bf = run(LED_SEQ_BOOT_FAIL, LED_SEQ_LEN(LED_SEQ_BOOT_FAIL));
    printf("chuoi        ve NGHI   so chop  sang dai nhat\n");
    printf("hop le       %5lu ms  %5d    %5lu ms\n", (unsigned long)ok.end_ms, ok.pulses, (unsigned long)ok.longest_on);
    printf("tu choi      %5lu ms  %5d    %5lu ms\n", (unsigned long)fail.end_ms, fail.pulses, (unsigned long)fail.longest_on);
    printf("tang SMRM    %5lu ms  %5d    %5lu ms\n", (unsigned long)ls.end_ms, ls.pulses, (unsigned long)ls.longest_on);
    printf("tang Ascon   %5lu ms  %5d    %5lu ms\n", (unsigned long)la.end_ms, la.pulses, (unsigned long)la.longest_on);
    printf("khoi dong OK %5lu ms  %5d    %5lu ms\n", (unsigned long)bo.end_ms, bo.pulses, (unsigned long)bo.longest_on);
    printf("khoi dong LOI%5lu ms  %5d    %5lu ms\n", (unsigned long)bf.end_ms, bf.pulses, (unsigned long)bf.longest_on);

    CHECK(ok.end_ms == 2080 && ok.pulses == 3 && ok.longest_on == 1500, "OK: sai thoi gian/so chop");
    CHECK(fail.end_ms == 1680 && fail.pulses == 8 && fail.longest_on == 100, "FAIL: sai thoi gian/so chop");
    CHECK(ls.end_ms == 1100 && ls.pulses == 2 && ls.longest_on == 400, "tang SMRM: sai");
    CHECK(la.end_ms == 400 && la.pulses == 1 && la.longest_on == 400, "tang Ascon: sai");
    CHECK(bo.end_ms == 1000 && bo.pulses == 1, "khoi dong OK: sai");
    CHECK(bf.end_ms == 2900 && bf.pulses == 15 && bf.longest_on == 100, "khoi dong LOI: sai");
    /* phan biet duoc bang mat: hop le co lan sang dai >= 1 s; tu choi khong co lan sang nao > 100 ms */
    CHECK(ok.longest_on >= 1000 && fail.longest_on <= 100, "hop le / tu choi khong phan biet duoc");

    /* goi moi chen ngang: hop le dang sang thi den goi hong */
    led_seq_start(&s, LED_SEQ_OK, LED_SEQ_LEN(LED_SEQ_OK), 0);
    expect(&s, 1000, 1, "chen ngang: dang sang (hop le)");
    led_seq_start(&s, LED_SEQ_FAIL, LED_SEQ_LEN(LED_SEQ_FAIL), 1000);
    expect(&s, 1000, 1, "chen ngang: chop 1 cua goi moi");
    expect(&s, 1080, 0, "chen ngang: tat giua 2 chop (khong con sang lien tuc)");
    expect(&s, 2680, 0, "chen ngang: ve NGHI");
    CHECK(!s.active, "chen ngang: chua ve NGHI");

    /* millis() tran so (sau ~49,7 ngay) */
    uint32_t t0 = 0xFFFFFE00u;
    led_seq_start(&s, LED_SEQ_OK, LED_SEQ_LEN(LED_SEQ_OK), t0);
    expect(&s, t0 + 600, 1, "tran so: sang lien tuc (qua moc 0)");
    expect(&s, t0 + 2080, 0, "tran so: ve NGHI");
    CHECK(!s.active, "tran so: chua ve NGHI");

    /* loop bi chan 10 s (lenh /bench) giua chuoi */
    led_seq_start(&s, LED_SEQ_FAIL, LED_SEQ_LEN(LED_SEQ_FAIL), 5000);
    expect(&s, 15000, 0, "loop bi chan: cap nhat lai -> NGHI ngay");
    CHECK(!s.active, "loop bi chan: chua ve NGHI");

    printf(fails ? "TEST LED: CO %d LOI\n" : "TEST LED: TAT CA PASS\n", fails);
    return fails != 0;
}
