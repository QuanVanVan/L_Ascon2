# Chạy lại / chạy tiếp các thí nghiệm của trailtool

Mọi lệnh dưới đây chạy trong Git Bash hoặc MSYS2, **từ bất kỳ thư mục nào** (các script tự tìm đường dẫn).
Đặt `D` là thư mục gốc của dự án, ví dụ `D=/e/DATN/L_Ascon2_KetQua`.

## 1. Biên dịch và kiểm thử (luôn làm trước)
```bash
bash $D/trailtool/build.sh            # -> trailtool/bin/t_dts.exe, t_lts.exe + checklayer cho 8 tầng
bash $D/trailtool/tests/run_tests.sh  # phải in "ALL TESTS PASSED"
```

## 2. Tái lập kết quả 3 vòng của bài báo cho Ascon (cây duyệt gốc, đã song song hoá)
```bash
bash $D/trailtool/scripts/replicate3r.sh /duong/dan/thu_muc_moi                       # ngưỡng đã sửa
bash $D/trailtool/scripts/replicate3r.sh /duong/dan/thu_muc_moi -DLEGACY_THRESHOLDS   # hành vi gốc
```
So sánh với `$D/results/05_trailtool_validation/d_replicate_fixed_thresholds/` (hoặc `b_...legacy`) bằng `cmp`.

## 3. Quét 3 vòng cho một tầng và một ngưỡng
```bash
source $D/trailtool/scripts/env.sh
bash $D/trailtool/scripts/scan3r.sh /thu_muc_ra "file:$FINALISTS_W#L_Ascon2" DTS 40   # vi sai, T3 <= 40
bash $D/trailtool/scripts/scan3r.sh /thu_muc_ra ascon LTS 28 tree                     # Ascon bằng cây gốc
"$PYTHON" $D/trailtool/scripts/verify_cores.py LTS SMRM-5 "$FINALISTS" <tep_loi_3_vong>   # kiểm tra độc lập
```

## 4. Quét toàn bộ (tự chạy tiếp)
```bash
bash $D/trailtool/scripts/run_all_3r.sh                       # mặc định: $D/results/06_trail3r/run_3r, 2400 s/ngưỡng
bash $D/trailtool/scripts/run_all_3r.sh /thu_muc_khac 7200    # ngân sách lớn hơn (tiếp tục nâng cận dưới)
```
Kết quả: `<thư mục>/SUMMARY.txt`. Bộ điều khiển bỏ qua những gì đã xong (ngưỡng đã quét rỗng, đã tìm được tối
thiểu, hoặc đã hết ngân sách). Hiện **mọi** cặp (tầng, chế độ) trong `results/06_trail3r/run_3r` đã kết thúc;
muốn nâng cận dưới cần ngân sách lớn hơn: xoá dòng `rc=124` tương ứng trong `SUMMARY.txt` rồi chạy với
budget lớn hơn.

Dừng giữa chừng: phải dừng **cả** bộ điều khiển lẫn tiến trình con (nếu chỉ giết `t_dts.exe`, bộ điều khiển
chuyển sang tầng kế tiếp; nếu giết `timeout.exe`, lần quét chạy không giới hạn):
```bash
wmic process where "name='bash.exe' and CommandLine like '%run_all_3r%'" call terminate
taskkill //F //IM timeout.exe ; taskkill //F //IM t_dts.exe ; taskkill //F //IM t_lts.exe
```
Sau đó xoá dòng dở dang cuối của `SUMMARY.txt` (có `rc=` khác 124 mà không có `RESULT`) và thư mục
`<tầng>/<chế độ>/T<ngưỡng>` tương ứng.
