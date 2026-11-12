# hieu_nang_asconc/ — L_Ascon2 trong ascon-c (đo hiệu năng x86)

Thư mục này chỉ chứa **phần sửa thêm** vào bộ cài đặt tham chiếu **ascon-c** (github.com/ascon/ascon-c, commit
`446347f`). Mã của ascon-c không nằm trong kho này.

| Tệp | Vai trò |
|---|---|
| `l_ascon2_layer.h` | macro `L_ASCON2_LAYER(x)`: tầng L_Ascon2 trên 5 từ 64 bit (12 XOR) |
| `round_ref_l_ascon2.h` | thay `round.h` của `crypto_aead/asconaead128/ref` → thư mục mới `ref_l_ascon2` |
| `round_opt64_l_ascon2.h` | thay `round.h` của `crypto_aead/asconaead128/opt64` → thư mục mới `opt64_l_ascon2` |
| `bench_perm.c` | đo số chu kỳ (TSC) của một lần gọi Ascon-p[12] và Ascon-p[8] |
| `run_bench.sh` | chạy xen kẽ 7 lượt cho 4 bản (`opt64`, `opt64_l_ascon2`, `ref`, `ref_l_ascon2`), ghim vào CPU 2 |

Cách dùng (Linux):
1. Chép `crypto_aead/asconaead128/ref` thành `ref_l_ascon2`, và `opt64` thành `opt64_l_ascon2`.
2. Trong mỗi bản sao, thay `round.h` bằng tệp `round_*_l_ascon2.h` tương ứng (đổi tên thành `round.h`) và chép thêm
   `l_ascon2_layer.h`. Không sửa tệp nào khác.
3. Build `genkat_aead` và `getcycles` của ascon-c cho cả 4 bản, cùng `bench_perm.c` (thêm `-DOPT64` cho bản opt64).
   Đặt tên các tệp chạy là `getcycles_<bản>` và `perm_<bản>`, đúng như `run_bench.sh` gọi.
4. Chạy `run_bench.sh`.

Kiểm tra trước khi đo: `genkat_aead` của bản gốc phải trùng tệp KAT chuẩn. Bản L_Ascon2 phải trùng KAT mà
`ascon_aead_demo` sinh ra (`ascon_aead_demo/results/c_L_Ascon2_KAT.txt`, sau khi chạy `build_all.sh`).

**Lưu ý ở bản opt64:** phép NOT trên `x2` phải đặt **trước** tầng L_Ascon2. Với `p_L` gốc, phép NOT có thể đi xuyên qua
Σ2; với tầng trộn nhiều hàng thì không.
