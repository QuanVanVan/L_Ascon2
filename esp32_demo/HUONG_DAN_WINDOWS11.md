# Nạp Ascon-L_Ascon2 lên ESP32 (esp32dev) và chạy demo PC ↔ ESP32 qua Wi-Fi — máy Windows 11

Gói này tự chứa: chỉ cần thư mục `esp32_demo/` (đã giải nén). Không cần MSYS hay phần còn lại của repo DATN.
Tầng mặc định là **L_Ascon2**. Firmware chứa thêm Ascon gốc và 5 ứng viên SMRM khác để so sánh.

```
esp32_demo/
├── ascon_esp32/        firmware (nạp vào ESP32)
├── pc/                 giao diện PC: run_gui.bat, pc_peer_gui.py, smrm_net.py, ascon_aead.py, layers_spec.py
├── platformio.ini, flash.bat
└── HUONG_DAN_WINDOWS11.md (tệp này), GIAO_THUC.md (đặc tả gói tin + kịch bản), README.md
```

## 1. Cài đặt (một lần)

0. Giải nén vào một **đường dẫn ngắn, không dấu, không khoảng trắng**, ví dụ `C:\esp32_demo`. PlatformIO hay lỗi với
   đường dẫn có ký tự tiếng Việt, kể cả khi tên tài khoản Windows có dấu.

1. **Driver USB–UART.** Cắm board, mở *Device Manager → Ports (COM & LPT)*. Nếu không thấy cổng COM: board có chip CP2102
   thì cài driver *CP210x* (Silicon Labs), chip CH340 thì cài *CH341SER* (WCH). Nên dùng cáp USB có dây dữ liệu, không
   dùng cáp chỉ để sạc.
2. **VS Code** và extension **PlatformIO IDE**. Extension tự cài PlatformIO Core vào `%USERPROFILE%\.platformio\penv`, và
   `flash.bat` cũng dùng chỗ này.
3. **Python 3.10 trở lên** từ python.org (giữ mặc định *tcl/tk and IDLE* để có tkinter; tích *Add python.exe to PATH*).
   Giao diện PC chỉ dùng thư viện chuẩn, không phải `pip install` gì thêm.

Lần build đầu tiên cần Internet: PlatformIO tải `espressif32@8.0.0`, toolchain và core Arduino-ESP32, tổng cộng vài trăm MB.

## 2. Nạp firmware

* **VS Code:** *File → Open Folder* chọn `esp32_demo`, chọn môi trường `env:esp32dev`, bấm **→ (Upload)** rồi
  **🔌 (Serial Monitor)**.
* Hoặc nhấp đúp **`flash.bat`** (hay chạy `flash.bat COM5` trong cmd để chỉ định cổng).

Nếu upload dừng ở `Connecting....`, **giữ nút BOOT** cho tới khi thấy `Writing at 0x...` thì thả ra.

**Phương án dự phòng, không cần cài PlatformIO:** nạp tệp đã build sẵn `prebuilt\esp32dev_L_Ascon2_merged.bin`
(SHA-256 `d40fa80f…996b5`, build lại 2026-11-12, bản báo hiệu bằng đèn xanh dương GPIO2, từ đúng mã nguồn trong gói này) bằng công cụ chính thức của Espressif chạy trên trình duyệt:
mở **https://espressif.github.io/esptool-js/** bằng **Chrome hoặc Edge**, bấm *Connect*, chọn cổng COM của board, nhập
*Flash Address* = **`0x0`**, chọn tệp `.bin` trên, rồi bấm *Program*. Sau khi nạp xong, bấm nút EN/RESET và xem kết quả bằng
mục *Console* của trang (115200 baud) hoặc bất kỳ chương trình serial nào.

Serial Monitor (115200 baud) sẽ hiện:
```
TU KIEM TRA: TAT CA PASS
Wi-Fi AP DA BAT: SSID "DATN-SMRM", mat khau "datn2026", ESP32 = 192.168.4.1, UDP 4210
```
`TAT CA PASS` nghĩa là L_Ascon2 trên ESP32 trùng từng byte với cài đặt C và Python trên PC, và Ascon gốc trùng test vector
chính thức.

## 3. Demo PC ↔ ESP32 (mức 3)

1. Nối Wi-Fi của laptop vào **`DATN-SMRM`**, mật khẩu **`datn2026`**. Windows báo "No internet" là bình thường.
2. Nhấp đúp **`pc\run_gui.bat`**. Nếu Windows Defender Firewall hỏi quyền cho Python, tích **cả Private và Public** rồi bấm
   *Allow*. Mạng của ESP32 thường bị xếp là *Public*; nếu chặn, gói trả lời từ ESP32 sẽ không tới được PC.
3. Bấm **Kết nối**. Sau khoảng 3 s phải thấy các dòng màu xanh `← ESP32 … số đo định kỳ, PC giải mã HỢP LỆ`.
4. Trình diễn theo thứ tự (chi tiết trong `GIAO_THUC.md`):

| Thao tác | Kết quả |
|---|---|
| Gõ tin nhắn rồi **Gửi** | Đèn xanh dương chớp 2 lần (đã nhận gói), ESP32 giải mã đúng (in ra Serial), đèn **sáng liền 1,5 s** rồi tắt; ESP32 trả ACK; PC giải mã ACK hợp lệ |
| Tích **Sửa gói GỬI đi**, chọn bản mã / thẻ / nonce / header, rồi Gửi | Đèn chớp 2 lần (đã nhận gói), ESP32 **từ chối**, đèn **nháy nhanh 6 lần** rồi tắt |
| **Phát lại gói vừa gửi** | ESP32 từ chối: `PHAT LAI`; đèn nháy nhanh 6 lần rồi tắt |
| Tích **Sửa gói NHẬN từ ESP32** | PC từ chối ACK và gói số đo |
| Nhấn **nút BOOT** trên ESP32 (đổi sang Ascon gốc; đèn xanh dương sáng dài 1 lần) | Mọi gói, cả hai chiều, đều bị từ chối, vì hai hoán vị khác nhau |
| Chọn `ascon` trong ô "Tầng tuyến tính của PC" | Trao đổi hợp lệ trở lại; nhấn BOOT lần nữa và chọn `L_Ascon2` để quay về |

**Đèn báo** (chỉ dùng đèn có sẵn trên board, không cần nối thêm gì):
* **Đèn đỏ** là đèn báo **nguồn**, nối thẳng vào 3,3 V, nên luôn sáng khi cắm board. Phần mềm không điều khiển được đèn này.
* **Đèn xanh dương** (GPIO2, cạnh cổng USB) dùng để báo hiệu. Một số board không có đèn ở GPIO2; khi đó chỉ cần xem Serial
  Monitor.

| Đèn xanh dương | Ý nghĩa |
|---|---|
| tắt | nghỉ, chờ gói (trạng thái ban đầu) |
| 2 chớp ngắn | vừa nhận một gói từ PC |
| tiếp theo **sáng liền 1,5 s**, rồi tắt | giải mã và xác thực **đúng** |
| tiếp theo **nháy nhanh 6 lần**, rồi tắt | **từ chối**: sai thẻ xác thực, khác tầng, phát lại, hoặc sai định dạng |
| sáng dài 2 lần (0,4 s) | vừa đổi sang L_Ascon2 (nút BOOT hoặc lệnh `/layer`) |
| sáng dài 1 lần (0,4 s) | vừa đổi sang Ascon gốc |
| lúc khởi động: sáng 1 s | tự kiểm tra PASS |
| lúc khởi động: nháy nhanh khoảng 3 s | tự kiểm tra có lỗi (xem Serial Monitor) |

Sau mỗi gói, đèn **tự tắt** về trạng thái ban đầu. Nếu gói mới đến khi đèn chưa xong, chuỗi cũ bị thay bằng chuỗi của gói
mới. Tầng đang dùng hiện trên Serial Monitor và trong các gói số đo mà GUI nhận được.

Ý nghĩa: ESP32 mã hoá bằng **C**, PC giải mã bằng **Python**, hai cài đặt viết độc lập. Gói được chấp nhận ở cả hai chiều
chứng tỏ cài đặt Ascon-L_Ascon2 trên vi điều khiển đúng.

## 4. Benchmark và gửi kết quả về

Trong Serial Monitor gõ `/bench` (chạy ít nhất 3 lần) và `/kat`. PlatformIO tự lưu toàn bộ đầu ra vào
`platformio-device-monitor-*.log` trong thư mục `esp32_demo`. Gửi các tệp đó về, ghi kèm loại module in trên board
(ví dụ `ESP32-WROOM-32`).

## Gỡ lỗi nhanh

| Hiện tượng | Cách xử lý |
|---|---|
| Không thấy Wi-Fi `DATN-SMRM` | Xem Serial có dòng `Wi-Fi AP DA BAT` chưa; bấm EN/RESET |
| GUI báo đã gửi nhưng không nhận được gì | Kiểm tra laptop đang nối `DATN-SMRM`; *Settings → Network & internet → Wi-Fi → DATN-SMRM → Network profile type* chọn **Private**; hoặc *Windows Security → Firewall & network protection → Allow an app through firewall* tích Python ở cả Private và Public; tắt VPN |
| Upload báo cổng COM đang bận | Đóng Serial Monitor, trang esptool-js hoặc mọi chương trình khác đang mở cổng đó |
| `run_gui.bat` báo lỗi `tkinter` | Cài lại Python từ python.org, giữ mục *tcl/tk* |
| Serial Monitor ra ký tự rác | Đặt tốc độ 115200 |
