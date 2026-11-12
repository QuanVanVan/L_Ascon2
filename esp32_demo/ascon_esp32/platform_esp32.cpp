/*
 * platform_esp32.cpp -- 5 ham plat_* ma demo_core.c can, cho ESP32 (Arduino).
 * De rieng tep .cpp (khong de trong .ino): bo tien xu ly .ino chen khai bao setup()/loop() vao truoc ham dau tien,
 * neu ham dau tien nam trong khoi extern "C" thi setup()/loop() bi gan nham lien ket C va loi bien dich.
 */
#include <Arduino.h>
#include "demo_core.h"

extern "C" {
uint32_t plat_cycles(void) { return ESP.getCycleCount(); }
uint32_t plat_cpu_mhz(void) { return getCpuFrequencyMhz(); }
const char *plat_name(void) { return ESP.getChipModel(); }
void plat_print(const char *s) { Serial.print(s); }
void plat_yield(void) { delay(1); }
}
