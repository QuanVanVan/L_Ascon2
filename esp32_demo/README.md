# Demo Ascon-AEAD128 / Ascon-SMRM trên ESP32

Firmware chạy **Ascon-AEAD128** (NIST SP 800-232) với `p_L` gốc của Ascon và 6 ứng viên SMRM
(`results/02_finalists/finalists.txt`; mặc định **L_Ascon2**) trên ESP32, kèm giao diện PC trao đổi gói tin mã hoá qua Wi-Fi.
Hướng dẫn cho máy cắm board: **`HUONG_DAN_WINDOWS11.md`**. Đặc tả gói tin và kịch bản trình diễn: **`GIAO_THUC.md`**.

| Mức | Nội dung | Ở đâu |
|---|---|---|
| 1. Tự kiểm tra trên board | Board sinh lại 1089 vector KAT cho 7 tầng, băm SHA-256 rồi so với tệp PC (C = Python; Ascon = tệp KAT chính thức), kiểm tra giải mã và từ chối thẻ giả; đối chiếu hoán vị 64 bit ↔ bit-interleaved 32 bit | chạy khi khởi động; lệnh `/kat` |
| 2. Benchmark | Số chu kỳ của `p_L`, hoán vị 12 vòng (64 bit / bit-interleaved), AEAD 64 B và 1 KiB (cycles/byte, tỉ lệ so với Ascon) | lệnh `/bench` |
| 3. Demo PC ↔ ESP32 | ESP32 phát Wi-Fi AP, trao đổi UDP với `pc/pc_peer_gui.py` (Python, **độc lập** với bản C). Có kẻ tấn công mô phỏng (lật bit, phát lại), nút BOOT đổi tầng; đèn xanh dương GPIO2 báo nhận gói, xác thực đúng/sai rồi tự tắt (`HUONG_DAN_WINDOWS11.md`) | `GIAO_THUC.md` |

Lệnh qua Serial (115200 baud): `/kat` · `/bench` · `/list` · `/layer <tên>` · `/help` · hoặc văn bản bất kỳ (mã hoá rồi
giải mã ngay trên board).

## Kiểm thử trên PC trước khi nạp (máy phát triển)

```bash
bash /e/DATN/L_Ascon2_KetQua/esp32_demo/host/build_host.sh     # sinh mã + biên dịch + tự kiểm tra + test giao thức C<->Python
~/.platformio/penv/Scripts/python.exe host/test_gui.py   # GUI <-> ESP32 giả lập (mã C), 11 kịch bản demo
MSYS_NO_PATHCONV=1 ./host/host_test.exe "/layer ascon" "Xin chao"
```
Phải in `TU KIEM TRA: TAT CA PASS`, `TEST GIAO THUC: TAT CA PASS`, `TEST LED: TAT CA PASS` và `TEST GUI: TAT CA PASS`. Số chu kỳ đo trên PC là TSC,
**chỉ để tham khảo**; số liệu cho luận văn phải lấy từ board. `test_gui.py` cần Python có tkinter: bản MSYS không có, nên
dùng CPython trong `~/.platformio/penv`.

Build firmware trên máy này: `~/.platformio/penv/Scripts/pio.exe run -e esp32dev` (chạy trong `esp32_demo/`; đã build
thành công 2026-11-09: RAM 15,7 %, Flash 69,9 %, không cảnh báo; các hàm hoán vị nằm trong IRAM). Ảnh nạp gộp tại `0x0`:
```bash
B=.pio/build/esp32dev; T=~/.platformio/packages
$T/tool-esptool/esptool.exe --chip esp32 merge-bin -o prebuilt/esp32dev_L_Ascon2_merged.bin --flash-mode dio \
  --flash-freq 40m --flash-size 4MB 0x1000 $B/bootloader.bin 0x8000 $B/partitions.bin \
  0xe000 $T/framework-arduinoespressif32/tools/partitions/boot_app0.bin 0x10000 $B/firmware.bin
```
Đóng gói gửi sang máy khác: `bash make_package.sh` → `dist/esp32_demo_L_Ascon2_win11.zip`.

## Tệp

| Tệp | Nội dung |
|---|---|
| `gen_layers.py` | Đọc `finalists.txt`, sinh `ascon_esp32/layers_gen.h` (mã chuyên biệt 64 bit và bit-interleaved, hash KAT mong đợi) và `pc/layers_spec.py` (cùng chỉ số tầng); chép `ascon_aead_demo/python/ascon_aead.py` sang `pc/`. Kiểm tra KAT C = Python và Ascon = tệp chính thức. |
| `ascon_esp32/demo_core.[ch]` | Lõi không phụ thuộc nền tảng: AEAD (cấu trúc chép từ `ascon_aead_demo/c/ascon_aead.c`), SHA-256, KAT, benchmark, giao thức gói tin `net_seal`/`net_open` |
| `ascon_esp32/ascon_esp32.ino` | Lớp ESP32: Serial, Wi-Fi AP + UDP, nút BOOT, LED GPIO2 (đèn đỏ trên board là đèn nguồn, không điều khiển được) |
| `ascon_esp32/led_signal.h` | Chuỗi báo hiệu bằng 1 LED (GPIO2), không chặn, tự về trạng thái nghỉ; thuần C, kiểm thử trên PC bằng `host/test_led.c` |
| `ascon_esp32/platform_esp32.cpp` | Các hàm `plat_*` cho ESP32 (bộ đếm chu kỳ, in ra). Để trong `.cpp` riêng vì bộ tiền xử lý `.ino` làm hỏng khối `extern "C"`. |
| `prebuilt/` | Ảnh nạp gộp `esp32dev_L_Ascon2_merged.bin` (nạp tại `0x0`, ví dụ bằng esptool-js) |
| `pc/` | `pc_peer_gui.py` (tkinter), `smrm_net.py` (giao thức, bản Python), `run_gui.bat` |
| `host/` | `host_main.c` (lớp PC; `--seal`/`--open`), `build_host.sh`, `test_net.py`, `test_led.c`, `fake_esp32.py`, `test_gui.py` |
| `platformio.ini`, `flash.bat` | Build/nạp `esp32dev` (`espressif32@8.0.0`, Arduino); log Serial ghi vào `platformio-device-monitor-*.log` |

Ghi kết quả đo trên board vào `results/07_esp32/<chip>_<ngày>.log`, kèm board, tần số CPU, phiên bản core và cờ biên dịch.

## Giới hạn (cần ghi trong luận văn)

* **Đúng chức năng không có nghĩa là an toàn.** Demo chỉ cho thấy SMRM chạy đúng trong chế độ duplex. Độ an toàn dựa trên
  các cận vết ở `results/06_trail3r/`. IV và số vòng giữ nguyên như Ascon.
* Giao thức chỉ để minh hoạ: khoá chia sẻ trước cố định, NACK không xác thực, chống phát lại chỉ nhớ phiên gần nhất
  (`GIAO_THUC.md`).
* ESP32 dùng lõi Xtensa LX6, **không phải ARM Cortex-M** như CLAUDE.md §7.3 yêu cầu. Số đo ở đây là một nền 32 bit nhúng
  bổ sung.
* AEAD dùng hoán vị dạng từ 64 bit. Dạng bit-interleaved mới chỉ được đo ở mức hoán vị (`p12/32`).
* Số chu kỳ có cộng chi phí gọi hàm và vòng lặp đo. Chi phí này giống nhau cho mọi tầng, nên tỉ lệ giữa các tầng vẫn so
  sánh được.
