/*
 * led_signal.h -- bao hieu bang MOT LED (LED xanh duong co san tren board, GPIO2), KHONG chan (khong dung delay()).
 * Thuan C, khong phu thuoc Arduino: firmware (ascon_esp32.ino) va kiem thu tren PC (host/test_led.c) dung chung.
 * (LED do tren board la den NGUON, noi thang 3V3: luon sang khi cam, phan mem khong dieu khien duoc.)
 *
 * Trang thai NGHI: LED TAT. Moi su kien chay mot chuoi buoc (sang/tat, thoi gian); het chuoi -> tu ve NGHI (tat).
 *   Nhan goi tu PC   : 2 chop ngan (80 ms), roi tat 300 ms                 ("da nhan goi, dang giai ma")
 *   Hop le           : tiep theo SANG LIEN TUC 1,5 s, roi tat              ("giai ma + xac thuc DUNG")
 *   Bi tu choi       : tiep theo NHAY NHANH 6 lan (100/100 ms), roi tat    ("xac thuc SAI / phat lai / sai dinh dang")
 *   Doi tang (BOOT)  : sang dai 0,4 s: 2 lan = L_Ascon2, 1 lan = Ascon goc
 *   Khoi dong        : tu kiem tra PASS -> sang 1 s; co loi -> nhay nhanh 3 s
 * Goi moi den khi chuoi cu chua xong: chuoi cu bi thay bang chuoi moi (goi moi nhat luon duoc bao).
 * Giai ma thuc te chi mat vai chuc micro giay; buoc "nhan goi" duoc keo dai de mat nguoi kip nhin thay.
 */
#ifndef LED_SIGNAL_H
#define LED_SIGNAL_H

#include <stdint.h>

typedef struct { uint8_t on; uint16_t ms; } led_step_t;

#define LED_RX_STEPS {1, 80}, {0, 120}, {1, 80}, {0, 300}
#define LED_BLINK_100 {1, 100}, {0, 100}
static const led_step_t LED_SEQ_OK[] = { LED_RX_STEPS, {1, 1500} };
static const led_step_t LED_SEQ_FAIL[] = { LED_RX_STEPS,
    LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, {1, 100} };
static const led_step_t LED_SEQ_LAYER_SMRM[] = { {1, 400}, {0, 300}, {1, 400} };
static const led_step_t LED_SEQ_LAYER_ASCON[] = { {1, 400} };
static const led_step_t LED_SEQ_BOOT_OK[] = { {1, 1000} };
static const led_step_t LED_SEQ_BOOT_FAIL[] = { LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100,
    LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100,
    LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, LED_BLINK_100, {1, 100} };
#define LED_SEQ_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

typedef struct {
    const led_step_t *steps;
    int n, i;            /* so buoc, buoc hien tai */
    uint32_t t0;         /* thoi diem (ms) bat dau buoc hien tai */
    int active;          /* 0 = NGHI */
} led_seq_t;

static inline void led_seq_start(led_seq_t *s, const led_step_t *steps, int n, uint32_t now_ms)
{
    s->steps = steps; s->n = n; s->i = 0; s->t0 = now_ms; s->active = n > 0;
}

/* Goi thuong xuyen (moi vong loop). Tra 1 = LED sang, 0 = tat. Phep tru uint32_t dung ca khi millis() tran so. */
static inline int led_seq_update(led_seq_t *s, uint32_t now_ms)
{
    while (s->active && (uint32_t)(now_ms - s->t0) >= s->steps[s->i].ms) {
        s->t0 += s->steps[s->i].ms;
        if (++s->i >= s->n) s->active = 0;
    }
    return s->active ? s->steps[s->i].on : 0;
}

#endif
