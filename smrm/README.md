# smrm/ — thiết kế, tìm kiếm và đánh giá tầng tuyến tính SMRM

Biên dịch: `bash build.sh` (gcc MSYS2 UCRT64, `-O3 -march=native -fopenmp -static`; `.exe` đặt cạnh mã nguồn).
Python: `/c/msys64/ucrt64/bin/python.exe` (chỉ thư viện chuẩn). Chạy các lệnh **từ thư mục `smrm/`**
(các tham số đường dẫn trong ví dụ là tương đối với nó). Phương pháp + chứng minh: `../docs/methods/README_methods.md`.

| Tệp | Vai trò | Bước trong quy trình | Đầu ra |
|---|---|---|---|
| `layers.py` | định nghĩa tầng (Ascon p_L, MRM/Hsilu, SMRM) + đại số ma trận 320×320 — **cài đặt tham chiếu** | nền | — |
| `sbox.py` | S-box Ascon (từ bitslice), DDT/LAT; S-box χ | nền | — |
| `lscommon.h` | hàm C dùng chung (SMRM, ma trận, S-box, đếm cột) | nền | — |
| `search_smrm.c` | lấy mẫu ngẫu nhiên + lọc theo tầng | 1. khảo sát | (in ra màn hình; tóm tắt trong `RESULTS.md` §4) |
| `opt_smrm.c` | leo đồi tối đa số nhánh | 2 | `../results/01_search/opt_k*.txt` |
| `opt_trail.c` | leo đồi tối đa `min(W_d, W_l)` vết 3 vòng (a1 ≤ 2), tuỳ chọn `Dmax` | 3 (chính) | `../results/01_search/opt_trail_*.txt` |
| `rank_candidates.py` | xếp hạng lại với `trail3` (a1 ≤ 3) | 4 | `../results/01_search/rank_h3_ascon.txt`, `../results/mat/*.bin` |
| `bn.c` | số nhánh CHÍNH XÁC + mẫu hoạt động (Gray code, bổ đề cắt) | 5 | `../results/02_finalists/finalists_eval*.txt` |
| `trail3.c` | vết 3 vòng tốt nhất trên lớp a1 ≤ h (cận trên) | 4–5 | như trên |
| `eval_candidate.py` | đánh giá một ứng viên: ma trận, kiểm `L^bT = L`, mật độ, số nhánh (gọi `bn.exe`); vết 3 vòng chạy riêng bằng `trail3.exe` | 5 | như trên |
| `verify_trail.py` | dựng lại và kiểm tra độc lập vết đạt cận trên | 5 | — |
| `trail_sat.py`, `run_sat_*.sh` | cận dưới số S-box bằng SAT (Kissat, `../tools/`) | 5 (cũ) | `../results/04_sat/` |
| `milp_trail.py` | mô hình MILP (quá yếu, chỉ tham khảo) | — | — |
| `cost.py`, `diffusion.py` | chi phí XOR/độ sâu/số lệnh; số vòng khuếch tán | 5 | `../results/03_cost_diffusion_bench/` |
| `gen_smrm_c.py` → `smrm_gen.h`; `perm_bench*.c` | sinh mã hằng cứng và đo tốc độ | 5 | `../results/03_cost_diffusion_bench/bench_*.txt` |
| `export_baselines.py` | ghi ma trận Ascon/Hsilu/MRM | nền | `../results/mat/` |

Danh sách 6 ứng viên chung kết dùng chung: `../results/02_finalists/finalists.txt`.
Bước 6 (chứng minh cận dưới bằng AsconTrailTool) nằm ở `../trailtool/`.
