# Nguồn test vector

`LWC_AEAD_KAT_128_128.txt` — test vector chính thức của **Ascon-AEAD128** (NIST SP 800-232), tải nguyên văn ngày 2026-10-30 từ
kho cài đặt tham chiếu của nhóm thiết kế Ascon:

https://raw.githubusercontent.com/ascon/ascon-c/main/crypto_aead/asconaead128/LWC_AEAD_KAT_128_128.txt

* 1089 vector: khoá `00..0F`, nonce `10..1F`, PT = `20 21 …` dài 0..32 byte, AD = `30 31 …` dài 0..32 byte; `CT` = bản mã ‖ thẻ 16 byte.
* SHA-256: `bbbc34692fe05e5fda0a3b025585622ab3e3747495e5e3655b29aae8c2a4bd33`
* Không sửa tệp này. Các tệp KAT cho Ascon-SMRM nằm ở `../results/` và **do dự án tự sinh** (không phải vector chính thức).
