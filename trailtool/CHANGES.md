# AsconTrailTool — bản sửa lỗi, đa luồng và mở rộng cho tầng tuyến tính tổng quát (DATN)

Thư mục (`DATN/trailtool/`):

| Đường dẫn | Nội dung |
|---|---|
| `upstream/` | bản **gốc nguyên vẹn** do giáo viên hướng dẫn cung cấp (`README.md`, `reference/C++/`) — không sửa, dùng để đối chiếu |
| `src/` | mã đã sửa lỗi + song song hoá + mở rộng (`genericLayer.*`, `generic2R.*` là tệp mới) |
| `build.sh` | biên dịch `bin/t_dts.exe` (vết vi sai), `bin/t_lts.exe` (vết tuyến tính) và tự kiểm tra tầng tuyến tính |
| `scripts/` | `env.sh` (đường dẫn dùng chung), `build_variant.sh`, `replicate3r.sh` (tái lập README 3 vòng), `scan3r.sh` (quét 3 vòng một tầng/ngưỡng), `run_all_3r.sh` (quét tất cả, chạy tiếp được), `compare_cores.py`, `verify_cores.py` |
| `tests/` | `run_tests.sh` (bảng S-box, điểm cột, hàm tuyến tính gốc, tầng tổng quát C++ ↔ Python) |
| `analysis/` | `check_tables.py`, `check_score.py`, `span.py` (phân tích độ trải ảnh của 1 bit) |
| `RESUME.md` | cách chạy lại / chạy tiếp |
| (kết quả) | `../results/05_trailtool_validation/` (kiểm chứng công cụ), `../results/06_trail3r/` (quét 3 vòng) |

Danh mục 6 ứng viên SMRM dùng chung cho mọi chương trình: `../results/02_finalists/finalists.txt`
(trong các log cũ có thể còn ghi `trailtool/bin/layers.txt` — đó là bản sao giống hệt, đã bỏ).

## 1. Lỗi đã phát hiện và sửa

| # | Tệp | Lỗi | Hậu quả | Sửa |
|---|---|---|---|---|
| 1 | `tree.h` | thiếu `#include <cstdint>` | không biên dịch được với GCC ≥ 13 (`uint64_t does not name a type`) | thêm include |
| 2 | `treeCanonical.h` | `RowAsUnits(int p)` không khởi tạo `pivot` | hành vi không xác định nếu `period == 1`; `-Werror` dừng biên dịch | `pivot(0)` |
| 3 | `treeCanonical.cpp::printState` | ghi vào `FILE*` đã đóng / chưa mở | UB (crash khi bật debug) | mở tệp trước khi ghi |
| 4 | `trailExtension.cpp` | `offsetParityAtB` chưa khởi tạo | UB; `-Werror` dừng biên dịch | `= 0` |
| 5 | môi trường Windows | chương trình nạp `libstdc++-6.dll` của Git (GCC khác phiên bản) → **segfault khi `-O3`** ngay tại `ofstream` | không chạy được | liên kết tĩnh (`-static`), xem `scripts/build_variant.sh` |
| 6 | `treeCanonical.cpp::getTotalWeight` | ngưỡng dùng `2*limit = 2⌊T/2⌋` và **so sánh chặt** cho kiểu 21 | với cấu hình README (`T = 39`, kiểu 21) chỉ sinh lõi `2w0+w1 ≤ 37` thay vì `≤ 39`; tuyến tính `T = 26` chỉ sinh `≤ 24`. Các lõi 3 vòng có `2w0+w1 ∈ {38,39}` (vi sai) hoặc `= 26` (tuyến tính) bị bỏ sót ⇒ khẳng định "không có vết trọng số < 40/< 28" **chưa được chứng minh đầy đủ** | đúng ngữ nghĩa README `≤ T`; giữ lại hành vi cũ bằng `-DLEGACY_THRESHOLDS` |
| 7 | `histogram.*` | mảng cố định, bước nhảy hàng `2*limit` khi ghi nhưng `limitWeight` khi in; `memset(..., sizeof(long long*))` | tệp CSV histogram sai khi `T` lẻ (vd. 39), có thể dồn số liệu sang hàng khác | `vector`, kích thước theo giá trị lớn nhất, lưu bước nhảy |
| 8 | `main.cpp` | đọc `argv[]` vượt `argc` | crash khi thiếu đối số | kiểm tra đối số, in hướng dẫn |
| 9 | `main.cpp` | README ghi `display cores <file>` nhưng mã đọc `argv[2]` | lệnh trong README mở tệp tên "cores" | chấp nhận cả hai dạng |
| 10 | `trailExtension.cpp::backwardExtendTrail`, `treeExtension.cpp::extendTrailBackward`, `main.cpp` | hiệu không dấu `maxWeight - baseWeight` bị tràn | mất cắt tỉa sớm (chậm, kết quả vẫn được lọc đúng) | so sánh có dấu |
| 11 | `findTrailsInCores` | không truy cập được từ dòng lệnh | — | lệnh mới `trails <file> <maxWeight>` |

Đã kiểm tra và **không** có lỗi: toàn bộ bảng DDT/LAT viết cứng (`analysis/check_tables.py`, đối chiếu S-box tính lại từ đặc tả bitslice); 4 hàm tuyến tính `L, L^{-1}, L^T, L^{-T}` (`tests/test_linear.cpp`, 10^5 vector); các công thức điểm cột `biggerThanTwo/Three` là cận dưới hợp lệ khi các hàng sau còn thay đổi (`analysis/check_score.py`).

## 2. Đa luồng

Bản gốc **đơn luồng** (có `#include <omp.h>` nhưng không có `#pragma omp` nào). Đã song song hoá bằng OpenMP:

* **Sinh lõi 2 vòng (cây duyệt của bài báo):** cây các trạng thái chính tắc được chia thành 165 cây con độc lập theo (hàng hoạt động đầu tiên `y0`, phần tử thứ hai `j` của hàng đó, `j = 0..32`). Mỗi cây con ghi tệp riêng, ghép theo đúng thứ tự duyệt tuần tự ⇒ **tệp kết quả trùng từng byte** với bản đơn luồng (đã kiểm tra bằng `cmp` cho cả 8 tệp của README).
* **Mở rộng vết (`extendWW`, `extend`, `trails`):** song song trên các lõi đầu vào, đầu ra gom theo thứ tự đầu vào ⇒ tất định.
* Lớp `State` dùng mảng cố định thay cho `vector<Row>` (bỏ cấp phát heap ở mỗi trạng thái; với MinGW `malloc` là nút cổ chai khi đa luồng).

Thời gian (máy 8 luồng logic, tái lập README 3 vòng; đầu ra: `../results/05_trailtool_validation/`):

| Bước | Gốc (1 luồng) | Bản sửa (8 luồng) |
|---|---|---|
| sinh lõi vi sai `2w0+w1`, T = 39 | 75,5 s | 36 s (cây con lớn nhất 33,5 s) |
| mở rộng xuôi 284 561 lõi | 15,1 s | 1,1 s |
| sinh lõi vi sai `w0+2w1`, T = 40 | 15,1 s | 6,4 s |
| mở rộng ngược 15 252 lõi | 4,4 s | 0,8 s |

## 3. Mở rộng: tầng tuyến tính chọn lúc chạy và bộ sinh lõi 2 vòng tổng quát

* `--layer=<spec>` (`genericLayer.*`): `ascon` (hàm gốc), `ascon-generic`, `smrm:<ops>|<d>`, `file:<path>#NAME`. Nghịch đảo và chuyển vị được suy ra **chính xác** từ chương trình xoay–XOR; lệnh `checklayer` tự kiểm tra `L·L⁻¹ = I`, `⟨Lx,y⟩ = ⟨x,Lᵀy⟩`, bất biến tịnh tiến.
* Lệnh `generic2r <11|12|21> <T>` (`generic2R.*`): sinh **vét cạn** mọi lõi 2 vòng chính tắc cho tầng bất kỳ bất biến theo phép xoay đồng thời 5 hàng (phương pháp và chứng minh tính đúng: đầu tệp `generic2R.cpp`). Lệnh `generate` của bài báo chỉ đúng cho Ascon và bị chặn với tầng khác.
* Kiểm chứng: trên Ascon, `generic2r` cho **đúng cùng tập lõi** (modulo tịnh tiến, không trùng lặp) với cây duyệt của bài báo ở 4 cấu hình: tuyến tính 2w0+w1 ≤ 26 và w0+2w1 ≤ 28, vi sai w0+2w1 ≤ 40 và 2w0+w1 ≤ 31 (`scripts/compare_cores.py`). Vi sai 2w0+w1 ≤ 39 không chạy hết bằng `generic2r` (tầng Ascon thưa ⇒ chậm, > 1 giờ); với Ascon nên dùng `generate`.
* Kết quả quét 3 vòng: `../results/06_trail3r/RESULTS_3R.md`.
