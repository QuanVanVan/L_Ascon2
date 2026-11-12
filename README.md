# L_Ascon2 — mã nguồn

Mã nguồn của đề tài *"Tầng tuyến tính mới dựa trên cấu trúc LS-Designs cho mật mã dựa trên hoán vị"*. Đề tài đề xuất
**L_Ascon2**, một tầng tuyến tính thay cho `p_L` của hoán vị Ascon (họ SMRM, Symmetric Multiple-Row Mixer).

Kho này **chỉ chứa mã nguồn** và hai tệp đầu vào bắt buộc:
* `results/02_finalists/finalists.txt`: tham số của L_Ascon2 và 5 ứng viên đối chứng, đầu ra đã chọn lọc của bước tìm kiếm;
* `ascon_aead_demo/kat/LWC_AEAD_KAT_128_128.txt`: test vector chính thức của Ascon-AEAD128, xem `kat/SOURCE.md`.

Không có kết quả chạy, tệp `.exe`, ảnh nạp ESP32 hay tài liệu tham khảo. Mọi tệp đó **sinh lại được** bằng `build_all.sh`
và đã được liệt kê trong `.gitignore`.

```
L_Ascon2|1,4,45;4,3,31;3,2,23;2,0,15;0,1,22;1,4,21|0,39,2,16,7
```
Thứ tự thực thi trên trạng thái 5 × 64 bit: `for l = k..1: x[j] ^= rotl(x[i], r)`, rồi `x[w] = rotl(x[w], d_w)`, rồi
`for l = 1..k: x[i] ^= rotl(x[j], r)`. Tổng cộng 12 XOR 64 bit (2,4 XOR/bit).

## Cấu trúc

| Thư mục | Nội dung |
|---|---|
| `smrm/` | **Tìm kiếm và đánh giá** tầng tuyến tính (C + Python), bảng tệp trong `smrm/README.md`: `opt_trail.c` (leo đồi theo trọng số vết 3 vòng, ràng buộc độ sâu), `opt_smrm.c`, `search_smrm.c`, `rank_candidates.py`; `bn.c` (số nhánh chính xác), `trail3.c` (vết 3 vòng tốt nhất, cận trên), `eval_candidate.py`, `verify_trail.py`, `cost.py`, `diffusion.py`, `trail_sat.py`; `layers.py` (cài đặt tham chiếu); `export_*.py` (ghi ma trận) |
| `trailtool/` | **AsconTrailTool** đã sửa lỗi, đa luồng và mở rộng cho tầng tuyến tính bất kỳ: chứng minh cận dưới vết vi sai/tuyến tính 3 vòng. `upstream/` là bản gốc của tác giả; `src/` là bản đã sửa (`CHANGES.md`); `scripts/` để quét (`scan3r.sh`, `scan_fine.sh`, `run_all_3r.sh`, `verify_cores.py` …); `tests/`; `analysis/`. Cách chạy: `RESUME.md` |
| `ascon_aead_demo/` | **Ascon-AEAD128** (NIST SP 800-232) với `p_L` thay được, viết bằng Python và C độc lập. Kiểm tra mã hoá, giải mã và xác thực khi dùng L_Ascon2 |
| `esp32_demo/` | **Demo ESP32**: firmware (`ascon_esp32/`), giao diện PC (`pc/`), kiểm thử trên PC (`host/`), `gen_layers.py`. Hướng dẫn trong `README.md`, `HUONG_DAN_WINDOWS11.md`, `GIAO_THUC.md` |
| `hieu_nang_asconc/` | Phần sửa thêm vào ascon-c để đo hiệu năng x86 (`README.md`) |
| `results/02_finalists/finalists.txt` | **Nguồn duy nhất** của tham số các tầng. Mọi chương trình đều đọc tệp này |

## Môi trường

* Windows 10/11, **Git Bash** hoặc MSYS2; gcc 13 và Python 3 của **MSYS2 UCRT64** (`C:\msys64\ucrt64\bin`), chỉ dùng thư viện
  chuẩn. Mọi `build.sh` liên kết tĩnh. Đây là yêu cầu bắt buộc: không liên kết tĩnh thì chương trình C++ nạp nhầm
  `libstdc++` của Git và crash.
* Firmware ESP32: PlatformIO (`espressif32@8.0.0`, Arduino). GUI PC cần Python có tkinter (bản từ python.org).
* `smrm/trail_sat.py` dùng Kissat (`tools/kissat.exe`, không kèm theo) nếu có, ngược lại dùng Z3.

## Chạy

```bash
bash build_all.sh
```
Lệnh này lần lượt:
1. biên dịch `smrm/` (tự sinh `smrm_gen.h`);
2. ghi các ma trận `results/mat/*.bin`;
3. biên dịch trailtool và chạy `checklayer`;
4. chạy bộ kiểm thử trailtool;
5. chạy AEAD: Ascon gốc với 1089 KAT chính thức, sau đó 6 tầng thay thế (gồm L_Ascon2), sinh KAT;
6. sinh mã ESP32 và tự kiểm tra firmware trên PC (KAT, giao thức C ↔ Python, chuỗi LED).

Mỗi bước phải in PASS. Thời gian chạy vài phút.

Sau đó có thể chạy từng phần:
```bash
cd smrm
./trail3.exe ../results/mat/ascon.bin diff 3        # kiểm chứng công cụ: Ascon 15 S-box / trọng số 40
./trail3.exe ../results/mat/L_Ascon2.bin diff 3     # L_Ascon2: cận trên vết vi sai 3 vòng
./trail3.exe ../results/mat/L_Ascon2.bin lin 3      # cận trên vết tuyến tính
./bn.exe ../results/mat/L_Ascon2.bin 4              # số nhánh chính xác B_d, B_l (~4 phút)

# cận dưới đã chứng minh: quét vết 3 vòng (vi sai, T3 <= 40; ngưỡng lớn hơn cần nhiều giờ)
source ../trailtool/scripts/env.sh
bash ../trailtool/scripts/scan3r.sh /thu_muc_ra "file:$FINALISTS_W#L_Ascon2" DTS 40

cd ../ascon_aead_demo && ./c/ascon_tool.exe demo L_Ascon2 "ban ro" "AD"   # mã hoá -> giải mã -> thử sửa gói

cd ../esp32_demo && pio run -e esp32dev            # firmware; nạp: flash.bat (xem README.md của thư mục)
```
Đầu ra của các lần quét trailtool mặc định ghi vào `results/06_trail3r/run_3r/` (bị bỏ qua bởi `.gitignore`).

**Lưu ý khi sửa:** các tệp KAT được so từng byte và bằng SHA-256, nên không để git hay trình soạn thảo đổi LF thành CRLF.
`.gitattributes` đã đặt `* -text` cho việc này.
