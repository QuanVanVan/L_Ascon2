# Demo mã hoá / giải mã xác thực Ascon-AEAD128 và Ascon-SMRM

Thư mục này cài đặt **Ascon-AEAD128** (NIST SP 800-232) với tầng tuyến tính `p_L` **thay thế được**, rồi:

1. kiểm tra cài đặt bằng **test vector chính thức** (1089 vector, `kat/`);
2. thay `p_L` bằng **L_Ascon2** (và các ứng viên SMRM khác), giữ nguyên mọi thứ còn lại, và kiểm tra rằng mã hoá – giải mã –
   xác thực vẫn đúng.

Có hai cài đặt **độc lập** (Python tham chiếu và C), đối chiếu chéo với nhau theo quy tắc ở `CLAUDE.md` §0 mục 5.

## Chạy

```bash
bash /e/DATN/L_Ascon2_KetQua/ascon_aead_demo/run_all.sh            # Ascon gốc + L_Ascon2   (≈ 15 s)
bash /e/DATN/L_Ascon2_KetQua/ascon_aead_demo/run_all.sh all        # Ascon gốc + cả 6 ứng viên SMRM
```
Log đầy đủ: `results/run_all.log`. Dòng cuối là `TONG KET: TAT CA PASS` nếu mọi bước đều đạt.

Từng lệnh riêng:
```bash
cd /e/DATN/L_Ascon2_KetQua/ascon_aead_demo
bash build.sh                                                   # biên dịch c/ascon_tool.exe
./c/ascon_tool.exe demo L_Ascon2                                 # minh hoạ (mặc định: ascon)
./c/ascon_tool.exe demo L_Ascon2 "ban ro tuy y" "AD tuy y"
./c/ascon_tool.exe kat kat/LWC_AEAD_KAT_128_128.txt ascon       # kiểm tra 1089 vector chính thức
./c/ascon_tool.exe gen results/c_L_Ascon2_KAT.txt L_Ascon2        # sinh KAT cho một tầng
python python/test_kat.py                                       # Python: vector chính thức
python python/test_smrm_aead.py L_Ascon2                         # Python: kiểm tra Ascon-SMRM
```
Tên tầng: `ascon`, `smrm:<ops>|<d>`, hoặc tên trong `results/02_finalists/finalists.txt` (`SMRM-5`, `SMRM-6`, `L_Ascon2`, …).
Python: `/c/msys64/ucrt64/bin/python.exe` (chỉ thư viện chuẩn).

## Cấu trúc

| Tệp | Nội dung |
|---|---|
| `python/ascon_aead.py` | cài đặt tham chiếu: hoán vị, `p_L` Ascon / SMRM, AEAD, đọc–ghi tệp KAT |
| `python/test_kat.py` | 1089 vector chính thức + sinh lại tệp KAT phải trùng từng byte |
| `python/test_smrm_aead.py` | kiểm tra Ascon-SMRM (mục A–D bên dưới), ghi `results/py_<TÊN>_KAT.txt` |
| `c/ascon_aead.[ch]` | cài đặt C (độc lập với Python) |
| `c/ascon_tool.c` | lệnh `kat`, `gen`, `demo` |
| `kat/` | tệp KAT chính thức + `SOURCE.md` (nguồn, SHA-256) |
| `results/` | KAT tự sinh của từng tầng (C và Python), `run_all.log` |

## Thay gì, giữ gì

Hoán vị `p = p_L ∘ p_S ∘ p_C`. **Chỉ `p_L` thay đổi**; giữ nguyên `p_C` (hằng số vòng `0xf0, 0xe1, …, 0x4b` cộng vào `x2`),
`p_S` (S-box 5 bit bitslice), số vòng (`p^12` khởi tạo/kết thúc, `p^8` xử lý dữ liệu), IV `0x00001000808C0001`, tốc độ
128 bit, đệm `0x01‖0*`, bit tách miền, thứ tự byte little-endian.

`p_L` SMRM (`L = B·D·B^bT`), cùng quy ước với `smrm/layers.py` và trailtool:
```
for l = k..1:  x[j_l] ^= rotl(x[i_l], r_l)
x[w] = rotl(x[w], d_w)                      (w = 0..4)
for l = 1..k:  x[i_l] ^= rotl(x[j_l], r_l)
```
L_Ascon2 = `1,4,45;4,3,31;3,2,23;2,0,15;0,1,22;1,4,21 | 0,39,2,16,7` (12 XOR 64 bit = 2,4 XOR/bit; Ascon: 10 XOR = 2,0).

## Kết quả (`results/run_all.log`, 2026-10-30)

**Ascon gốc**
* Python: 1089/1089 vector chính thức đúng. Với mỗi vector, mã hoá cho đúng `CT‖T`, giải mã cho đúng `PT`, và thẻ bị lật 1 bit thì bị từ chối.
  Tệp KAT sinh lại **trùng từng byte** với tệp chính thức.
* C: 1089/1089 vector đúng, tệp KAT sinh lại cũng trùng từng byte với tệp chính thức.

**L_Ascon2** (cả 6 ứng viên `SMRM-5/6/7/8/8L, L_Ascon2` đều cho kết quả giống hệt dưới đây khi chạy `run_all.sh all`):

| Mục | Kiểm tra | Kết quả |
|---|---|---|
| A | `p_L` ở đây = `smrm/layers.py` = ma trận `results/mat/L_Ascon2.bin` | 10⁴ / 10⁴ / 10³ trạng thái ngẫu nhiên khớp |
| A | khả nghịch `L⁻¹(L(x)) = x` | 10⁴ / 10⁴ |
| B | giải mã(mã hoá(m)) = m | 1089/1089 (lưới KAT) + 300/300 thông điệp ngẫu nhiên 0–1000 byte |
| C | lật từng bit của bản mã‖thẻ | 496/496 bị từ chối |
| C | lật từng bit của AD / nonce / khoá | 384/384 bị từ chối |
| C | cắt/thêm byte, bỏ AD, thêm `00` vào AD, giải mã bằng Ascon gốc | đều bị từ chối |
| D | đầu ra khác Ascon gốc | 1089/1089 vector |
| D | lật 1 bit nonce → số bit thẻ thay đổi | trung bình 64,6/128 (Ascon: 64,0) |
| — | KAT do C sinh = KAT do Python sinh | trùng từng byte (1089 vector) |

Kiểm tra ngược để chắc bộ test không "luôn đúng": dùng tầng L_Ascon2 với tệp chính thức của Ascon thì 0/1089 vector đúng;
dùng SMRM-6 với tệp KAT của L_Ascon2 cũng cho 0/1089.

## Giới hạn: những gì kết quả này *không* chứng minh

* **Đúng chức năng không có nghĩa là an toàn.** Chế độ duplex của Ascon chỉ dùng chiều thuận của hoán vị, nên giải mã
  đúng với *mọi* hàm vòng. Thử nghiệm ở đây xác nhận **cài đặt** đúng, cùng một hiệu ứng tuyết lở thô. Độ an toàn của
  Ascon-SMRM dựa trên phân tích vết (`results/06_trail3r/RESULTS_3R.md`) và các phân tích còn thiếu (H6, ≥ 4 vòng).
* **Không có test vector chính thức cho Ascon-SMRM.** Các tệp `results/*_KAT.txt` do dự án tự sinh. Chúng đáng tin vì hai
  cài đặt độc lập cho kết quả trùng nhau, và cùng hai cài đặt đó khớp vector chính thức khi dùng `p_L` gốc.
* **IV và số vòng giữ như Ascon** để chỉ đổi đúng một thành phần. Một biến thể thật nên dùng **IV riêng** để không trùng
  miền với chuẩn. Việc `p^8` có đủ cho SMRM hay không chưa được đánh giá.
* Đây là cài đặt tham chiếu để kiểm thử. Chỉ phép so sánh thẻ là thời gian hằng; mã chưa được gia cố chống kênh kề.
  Vòng lặp SMRM trong C là bản tổng quát, chưa tối ưu, nên **không** dùng để đo tốc độ (số đo tốc độ: `results/03_cost_diffusion_bench/`).
