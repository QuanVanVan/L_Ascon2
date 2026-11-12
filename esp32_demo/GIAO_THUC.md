# Giao thức gói tin ESP32 ↔ PC (demo mức 3)

Truyền qua UDP. ESP32 phát Wi-Fi AP `DATN-SMRM` (mật khẩu `datn2026`), địa chỉ `192.168.4.1`, cổng UDP **4210**. PC gửi
trước để ESP32 biết địa chỉ trả lời. Giao thức có hai cài đặt **độc lập**: C (`ascon_esp32/demo_core.c`, chạy trên ESP32)
và Python (`pc/smrm_net.py`, mã hoá bằng `ascon_aead.py`). `host/test_net.py` đối chiếu hai cài đặt này.

## Định dạng

```
gói    = header (9 B) ‖ nonce (16 B) ‖ C (0..200 B) ‖ T (16 B)          Ascon-AEAD128, AD = header
header = 'S' 'M' ‖ ver = 1 ‖ type ‖ layer ‖ seq (uint32, little-endian)
nonce  = dir ‖ session (7 B) ‖ seq (uint32 LE) ‖ 00 00 00 00
```

| Trường | Ý nghĩa |
|---|---|
| `type` | 1 TEXT (PC→ESP32), 2 TELEMETRY (ESP32→PC, 3 s/lần), 3 ACK (ESP32→PC), 4 NACK (ESP32→PC) |
| `layer` | Chỉ số tầng **bên gửi** đã dùng (0 = ascon, rồi theo `finalists.txt`). **Bên nhận không dùng trường này**: gói luôn được giải mã bằng tầng của chính bên nhận. Vì vậy khi hai bên khác tầng, thẻ xác thực sai và gói bị từ chối. |
| `dir` | 1 = PC→ESP32, 2 = ESP32→PC. Hai chiều không bao giờ dùng chung nonce. |
| `session` | 7 byte ngẫu nhiên, sinh mới mỗi lần PC chạy hoặc ESP32 khởi động. |
| `seq` | Tăng dần theo từng bên gửi; nằm trong cả header lẫn nonce và phải khớp nhau. |
| NACK | `header ‖ lý do (ASCII)`: **không mã hoá, không xác thực**, chỉ để hiển thị lý do ESP32 từ chối. |

Khoá: dùng chung trước, `00 01 … 0F`, **chỉ để demo**. Hệ thống thật cần phân phối khoá an toàn.

## Bên nhận kiểm tra theo thứ tự

1. Định dạng (magic, phiên bản, độ dài, seq trong nonce trùng seq trong header) → `sai dinh dang`
2. `dir` đúng chiều → `sai chieu`
3. Giải mã và xác thực bằng **tầng của bên nhận**, với AD = header → `the xac thuc SAI`. Mọi thay đổi ở header, nonce,
   bản mã hay thẻ, hoặc việc hai bên dùng khác tầng, đều rơi vào bước này.
4. Chống phát lại (chỉ xét **sau** khi xác thực thành công): cùng `session` mà `seq` ≤ seq lớn nhất đã nhận → `PHAT LAI`.

Giới hạn của demo: trạng thái chống phát lại chỉ nhớ phiên gần nhất. Gói của một phiên **cũ hơn** vẫn được chấp nhận
nếu chưa từng bị phát lại trong phiên hiện tại. Hệ thống thật cần thoả thuận phiên (handshake) và khoá phiên.

## Kịch bản trình diễn (đều đã thử tự động: `host/test_gui.py` với `host/fake_esp32.py`)

| Thao tác trên giao diện PC | Kết quả mong đợi |
|---|---|
| Gửi tin nhắn (cả hai bên dùng L_Ascon2) | Đèn xanh dương GPIO2 chớp 2 lần (nhận gói), ESP32 giải mã đúng, đèn sáng liền 1,5 s rồi tắt, gửi ACK; PC giải mã ACK hợp lệ |
| Bật "Sửa gói GỬI đi", chọn bản mã / thẻ / nonce / header | ESP32 từ chối (`the xac thuc SAI`), đèn chớp 2 lần rồi nháy nhanh 6 lần rồi tắt, gửi NACK |
| Bấm "Phát lại gói vừa gửi" | ESP32 từ chối (`PHAT LAI`), đèn nháy nhanh |
| Bật "Sửa gói NHẬN từ ESP32" | PC từ chối ACK và gói số đo |
| PC chọn `ascon` (ESP32 vẫn L_Ascon2), hoặc nhấn nút BOOT trên ESP32 | Mọi gói, cả hai chiều, đều bị từ chối |
| Chọn lại cùng một tầng ở hai bên | Trao đổi hợp lệ trở lại |
