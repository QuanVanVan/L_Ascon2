/*
 * ascon_esp32.ino -- demo Ascon-AEAD128 voi tang tuyen tinh SMRM tren ESP32 (Arduino IDE / PlatformIO).
 *
 * Khi khoi dong: tu kiem tra (KAT + doi chieu 64/32 bit) -> phat Wi-Fi AP -> nhan/gui goi tin ma hoa voi PC qua UDP.
 *   Wi-Fi  : SSID "DATN-SMRM", mat khau "datn2026", ESP32 = 192.168.4.1, cong UDP 4210 (giao thuc: GIAO_THUC.md)
 *   PC     : chay pc/pc_peer_gui.py (giai ma bang cai dat Python DOC LAP)
 *   Nut BOOT (GPIO0): doi tang Ascon goc <-> L_Ascon2.
 *   LED xanh duong tren board (GPIO2) bao hieu (chi tiet: led_signal.h). NGHI: TAT.
 *     nhan goi tu PC  -> 2 chop ngan
 *     hop le          -> roi SANG LIEN TUC 1,5 s   -> tu tat (ve NGHI)
 *     bi tu choi      -> roi NHAY NHANH 6 lan      -> tu tat (ve NGHI)
 *     doi tang        -> sang dai 2 lan = L_Ascon2, 1 lan = Ascon goc
 *   LED do tren board = den NGUON (noi thang 3V3), luon sang khi cam; phan mem khong dieu khien.
 *   Serial 115200: /kat /bench /list /layer <ten> /help, hoac ban ro bat ky (demo tai cho).
 * Toan bo phan mat ma nam trong demo_core.c (dung chung voi ban kiem thu tren PC: esp32_demo/host/).
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "demo_core.h"                                   /* ham plat_* cho ESP32: platform_esp32.cpp */
#include "led_signal.h"

#define LED_ON_LEVEL HIGH                               /* LED GPIO2 tren board: muc HIGH = sang */

static const char *AP_SSID = "DATN-SMRM";
static const char *AP_PASS = "datn2026";                /* WPA2: toi thieu 8 ky tu */
static const uint16_t UDP_PORT = 4210;
static const uint32_t TELEMETRY_MS = 3000;
static const int BTN_PIN = 0;                           /* nut BOOT */
#ifdef LED_BUILTIN
static const int LED_PIN = LED_BUILTIN;
#else
static const int LED_PIN = 2;
#endif

static WiFiUDP udp;
static IPAddress peer_ip;
static uint16_t peer_port = 0;
static net_peer_t pc_state = { 0 };                     /* chong phat lai goi tu PC */
static uint8_t session[7];                              /* ngau nhien moi lan khoi dong */
static uint32_t tx_seq = 0;
static uint32_t n_ok = 0, n_rej = 0;
static uint32_t last_tel = 0;

static int idx_ascon, idx_smrm;

/* ------------------------------------------------------------------ LED / nut */
/* LED: chuoi khong chan, cap nhat trong loop(); het chuoi tu ve NGHI (tat) */
static led_seq_t led_seq = { 0 };

static void led_out(int on) { digitalWrite(LED_PIN, on ? LED_ON_LEVEL : !LED_ON_LEVEL); }

static void led_signal(const led_step_t *steps, int n)
{
    led_seq_start(&led_seq, steps, n, millis());
    led_out(led_seq_update(&led_seq, millis()));        /* sang ngay, khong doi vong loop ke */
}

static void poll_leds(void) { led_out(led_seq_update(&led_seq, millis())); }

/* bao tang moi bang LED: 2 lan sang dai = L_Ascon2 (SMRM), 1 lan = Ascon goc */
static void led_layer_changed(void)
{
    if (demo_layer_index() == idx_ascon) led_signal(LED_SEQ_LAYER_ASCON, LED_SEQ_LEN(LED_SEQ_LAYER_ASCON));
    else led_signal(LED_SEQ_LAYER_SMRM, LED_SEQ_LEN(LED_SEQ_LAYER_SMRM));
}

static void poll_button(void)
{
    static int last = HIGH;
    static uint32_t t_change = 0;
    int v = digitalRead(BTN_PIN);
    if (v != last && millis() - t_change > 50) {
        t_change = millis();
        last = v;
        if (v == LOW) {                                 /* vua nhan */
            Serial.print("\n[NUT BOOT] ");
            demo_set_layer(demo_layer_index() == idx_smrm ? idx_ascon : idx_smrm);
            led_layer_changed();
        }
    }
}

/* ------------------------------------------------------------------ gui goi */
static void send_sealed(int type, const char *text)
{
    if (!peer_port) return;
    uint8_t pkt[NET_MAX_PKT];
    size_t mlen = strlen(text);
    if (mlen > NET_MAX_MSG) mlen = NET_MAX_MSG;
    size_t n = net_seal(pkt, type, demo_layer_index(), ++tx_seq, session, NET_DIR_ESP_TO_PC, (const uint8_t *)text, mlen);
    udp.beginPacket(peer_ip, peer_port);
    udp.write(pkt, n);
    udp.endPacket();
}

static void send_nack(uint32_t seq, const char *reason)
{
    uint8_t pkt[NET_HDR + 128];
    pkt[0] = 'S'; pkt[1] = 'M'; pkt[2] = 1; pkt[3] = NET_NACK; pkt[4] = (uint8_t)demo_layer_index();
    for (int t = 0; t < 4; t++) pkt[5 + t] = (uint8_t)(seq >> (8 * t));
    size_t r = strlen(reason);
    if (r > 120) r = 120;
    memcpy(pkt + NET_HDR, reason, r);
    udp.beginPacket(peer_ip, peer_port);
    udp.write(pkt, NET_HDR + r);
    udp.endPacket();
}

/* ------------------------------------------------------------------ nhan goi */
static void poll_udp(void)
{
    int len = udp.parsePacket();
    if (len <= 0) return;
    static uint8_t pkt[1500];
    uint8_t msg[NET_MAX_MSG + 1];
    int n = udp.read(pkt, sizeof pkt);
    if (n <= 0) return;
    peer_ip = udp.remoteIP();
    peer_port = udp.remotePort();
    net_hdr_t h = { 0 };
    size_t mlen = 0;
    const layer_t *me = demo_layer(demo_layer_index());
    uint32_t c0 = ESP.getCycleCount();
    int rc = net_open(pkt, (size_t)n, NET_DIR_PC_TO_ESP, demo_layer_index(), &pc_state, &h, msg, &mlen);
    uint32_t cyc = ESP.getCycleCount() - c0;
    const layer_t *claimed = rc == NET_ERR_FORMAT ? NULL : demo_layer(h.layer);
    if (rc == NET_OK) {
        n_ok++;
        msg[mlen] = 0;
        Serial.printf("\n[UDP %s] #%lu HOP LE (tang %s, giai ma %lu chu ky = %.1f us): \"%s\"\n",
                      peer_ip.toString().c_str(), (unsigned long)h.seq, me->name, (unsigned long)cyc,
                      (double)cyc / getCpuFrequencyMhz(), (char *)msg);
        char reply[NET_MAX_MSG + 1];
        snprintf(reply, sizeof reply, "ESP32 da nhan #%lu: \"%.120s\" (giai ma %lu chu ky)", (unsigned long)h.seq,
                 (char *)msg, (unsigned long)cyc);
        led_signal(LED_SEQ_OK, LED_SEQ_LEN(LED_SEQ_OK));
        send_sealed(NET_ACK, reply);
    } else {
        n_rej++;
        char why[128];
        snprintf(why, sizeof why, "ESP32 TU CHOI #%lu: %s (ESP32 dung %s, goi ghi %s)", (unsigned long)h.seq,
                 net_strerror(rc), me->name, claimed ? claimed->name : "?");
        Serial.printf("\n[UDP %s] %s\n", peer_ip.toString().c_str(), why);
        led_signal(LED_SEQ_FAIL, LED_SEQ_LEN(LED_SEQ_FAIL));
        send_nack(h.seq, why);
    }
}

static void poll_telemetry(void)
{
    if (!peer_port || millis() - last_tel < TELEMETRY_MS) return;
    last_tel = millis();
    char text[NET_MAX_MSG + 1];
    snprintf(text, sizeof text, "uptime %lus | tang %s | da nhan: %lu hop le, %lu bi tu choi | heap %lu B",
             (unsigned long)(millis() / 1000), demo_layer(demo_layer_index())->name, (unsigned long)n_ok,
             (unsigned long)n_rej, (unsigned long)ESP.getFreeHeap());
    send_sealed(NET_TELEMETRY, text);
}

/* ------------------------------------------------------------------ Serial */
static char line[300];
static size_t nline = 0;

static void poll_serial(void)
{
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\r' || ch == '\n') {
            if (nline) {
                line[nline] = 0;
                int before = demo_layer_index();
                demo_command(line);                     /* vd. "/layer ascon" doi tang */
                nline = 0;
                if (demo_layer_index() != before) led_layer_changed();
            }
        } else if (nline + 1 < sizeof line) {
            line[nline++] = ch;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    led_out(0);
    pinMode(BTN_PIN, INPUT_PULLUP);
    delay(1500);                                        /* cho Serial Monitor ket noi */
    idx_ascon = demo_find_layer("ascon");
    idx_smrm = demo_find_layer("L_Ascon2");
    demo_set_layer(idx_smrm);
    int selftest_errors = demo_selftest();
    /* tu kiem tra dung -> LED sang 1 s; co loi -> nhay nhanh 3 s; sau do tat (NGHI) */
    if (selftest_errors == 0) led_signal(LED_SEQ_BOOT_OK, LED_SEQ_LEN(LED_SEQ_BOOT_OK));
    else led_signal(LED_SEQ_BOOT_FAIL, LED_SEQ_LEN(LED_SEQ_BOOT_FAIL));
    Serial.printf("LED bao hieu: GPIO%d (nghi = tat; nhan goi = 2 chop, hop le = sang 1,5 s, tu choi = nhay nhanh)\n",
                  LED_PIN);

    for (int t = 0; t < 7; t++) session[t] = (uint8_t)esp_random();
    WiFi.mode(WIFI_AP);
    bool ok = WiFi.softAP(AP_SSID, AP_PASS);
    udp.begin(UDP_PORT);
    Serial.printf("\nWi-Fi AP %s: SSID \"%s\", mat khau \"%s\", ESP32 = %s, UDP %u\n", ok ? "DA BAT" : "LOI", AP_SSID,
                  AP_PASS, WiFi.softAPIP().toString().c_str(), UDP_PORT);
    Serial.println("Ket noi laptop vao Wi-Fi tren roi chay pc_peer_gui.py. Nut BOOT: doi Ascon goc <-> L_Ascon2.");
    demo_banner();
}

void loop()
{
    poll_serial();
    poll_button();
    poll_udp();
    poll_telemetry();
    poll_leds();
    delay(2);
}
