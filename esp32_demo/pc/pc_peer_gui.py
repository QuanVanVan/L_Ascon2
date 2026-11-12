"""
pc_peer_gui.py -- giao dien PC cho demo Ascon-SMRM: trao doi goi tin ma hoa voi ESP32 qua Wi-Fi (UDP).

Phia PC ma hoa / giai ma bang ascon_aead.py (Python), DOC LAP voi cai dat C tren ESP32: goi do ESP32 tao ra ma PC giai
ma dung (va nguoc lai) la kiem tra cheo giua hai cai dat tren kenh truyen that.
Chi dung thu vien chuan (tkinter, socket, threading). Chay: python pc_peer_gui.py   (hoac nhap dup run_gui.bat)
"""
import os
import queue
import socket
import threading
import time
import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText

import smrm_net as N

ESP_IP, ESP_PORT = "192.168.4.1", 4210
REGIONS = {"bản mã (C)": "ct", "thẻ xác thực (T)": "tag", "nonce": "nonce", "header (AD)": "hdr"}
TYPE_VI = {N.TEXT: "tin nhắn", N.TELEMETRY: "số đo định kỳ", N.ACK: "xác nhận", N.NACK: "NACK"}


def flip_bit(pkt, region):
    """Lat 1 bit (byte dau tien) trong vung chon; tra (goi moi, vi tri byte)."""
    if region == "hdr":
        pos = 3                                   # truong type: chi duoc bao ve boi AD -> loi o buoc xac thuc
                                                  # (sua seq se bi bat som hon o buoc dinh dang vi seq co ca trong nonce)
    elif region == "nonce":
        pos = N.HDR + 1                           # byte dau cua session
    elif region == "tag" or len(pkt) == N.HDR + 32:
        pos = len(pkt) - 16                       # ban ro rong -> khong co ban ma, sua the
    else:
        pos = N.HDR + 16
    b = bytearray(pkt)
    b[pos] ^= 0x01
    return bytes(b), pos


def short_hex(pkt):
    h, n, c = pkt[:N.HDR].hex(), pkt[N.HDR:N.HDR + 16].hex(), pkt[N.HDR + 16:-16].hex()
    if len(c) > 48:
        c = c[:48] + "…"
    return f"hdr {h} | nonce {n} | C {c or '(rỗng)'} | T {pkt[-16:].hex()}"


class App:
    def __init__(self, root):
        self.root = root
        root.title("Ascon-SMRM — PC ↔ ESP32 (Wi-Fi)")
        root.geometry("980x680")
        self.q = queue.Queue()
        self.sock = None
        self.session = os.urandom(7)
        self.seq = 0
        self.esp_state = N.Peer()
        self.last_sent = None
        self.cnt = {"gửi": 0, "PC nhận hợp lệ": 0, "PC từ chối": 0, "ESP32 từ chối": 0}

        top = ttk.Frame(root, padding=6)
        top.pack(fill="x")
        ttk.Label(top, text="ESP32:").pack(side="left")
        self.ip = tk.StringVar(value=ESP_IP)
        ttk.Entry(top, textvariable=self.ip, width=14).pack(side="left")
        self.port = tk.StringVar(value=str(ESP_PORT))
        ttk.Entry(top, textvariable=self.port, width=6).pack(side="left", padx=(2, 12))
        ttk.Label(top, text="Tầng tuyến tính của PC:").pack(side="left")
        self.layer = tk.StringVar(value="L_Ascon2")
        cb = ttk.Combobox(top, textvariable=self.layer, values=[n for n, _ in N.LAYERS], width=10, state="readonly")
        cb.pack(side="left")
        cb.bind("<<ComboboxSelected>>", lambda e: self.log(f"PC đổi tầng → {self.layer.get()}", "info"))
        self.btn_conn = ttk.Button(top, text="Kết nối", command=self.connect)
        self.btn_conn.pack(side="left", padx=12)
        ttk.Label(top, text="(trước đó: nối laptop vào Wi-Fi “DATN-SMRM”, mật khẩu datn2026)",
                  foreground="#666").pack(side="left")

        row = ttk.Frame(root, padding=(6, 0))
        row.pack(fill="x")
        ttk.Label(row, text="Tin nhắn:").pack(side="left")
        self.msg = tk.StringVar(value="Xin chào ESP32, đây là Ascon-L_Ascon2!")
        ent = ttk.Entry(row, textvariable=self.msg)
        ent.pack(side="left", fill="x", expand=True, padx=4)
        ent.bind("<Return>", lambda e: self.send())
        ttk.Button(row, text="Gửi (mã hoá)", command=self.send).pack(side="left")

        atk = ttk.LabelFrame(root, text="Kẻ tấn công ở giữa (mô phỏng trên PC)", padding=6)
        atk.pack(fill="x", padx=6, pady=6)
        self.tx_tamper = tk.BooleanVar()
        ttk.Checkbutton(atk, text="Sửa gói GỬI đi: lật 1 bit ở", variable=self.tx_tamper).pack(side="left")
        self.region = tk.StringVar(value="bản mã (C)")
        ttk.Combobox(atk, textvariable=self.region, values=list(REGIONS), width=16, state="readonly").pack(side="left")
        self.rx_tamper = tk.BooleanVar()
        ttk.Checkbutton(atk, text="Sửa gói NHẬN từ ESP32 (lật 1 bit bản mã)", variable=self.rx_tamper).pack(
            side="left", padx=16)
        ttk.Button(atk, text="Phát lại gói vừa gửi (replay)", command=self.replay).pack(side="left")

        self.text = ScrolledText(root, font=("Consolas", 10), wrap="word")
        self.text.pack(fill="both", expand=True, padx=6)
        for tag, color in [("ok", "#0a7d22"), ("err", "#c4161c"), ("info", "#555555"), ("tel", "#1f5fbf"),
                           ("hex", "#8a8a8a"), ("tx", "#000000")]:
            self.text.tag_configure(tag, foreground=color)
        self.status = tk.StringVar()
        ttk.Label(root, textvariable=self.status, padding=4).pack(fill="x")
        self.update_status()
        self.log("Tầng: " + ", ".join(n for n, _ in N.LAYERS) + ". Khoá chia sẻ trước 000102…0F (CHỈ ĐỂ DEMO).",
                 "info")
        self.log(f"Phiên PC: session = {self.session.hex()} (ngẫu nhiên mỗi lần chạy, nằm trong nonce).", "info")
        root.after(50, self.drain)

    # ---------------------------------------------------------------- tien ich
    def log(self, s, tag="info"):
        self.text.insert("end", time.strftime("%H:%M:%S ") + s + "\n", tag)
        self.text.see("end")

    def update_status(self):
        self.status.set("   ".join(f"{k}: {v}" for k, v in self.cnt.items()))

    def layer_idx(self):
        return N.layer_index(self.layer.get())

    def addr(self):
        return self.ip.get().strip(), int(self.port.get())

    # ---------------------------------------------------------------- mang
    def connect(self):
        if self.sock is None:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.bind(("", 0))
            self.sock.settimeout(0.3)
            threading.Thread(target=self.rx_loop, daemon=True).start()
            self.btn_conn.configure(text="Đã kết nối")
            self.log(f"Mở UDP cổng cục bộ {self.sock.getsockname()[1]}, gửi tới {self.addr()}", "info")
        self.send("PC kết nối")

    def rx_loop(self):
        while True:
            try:
                data, src = self.sock.recvfrom(2048)
            except socket.timeout:
                continue
            except OSError as e:
                self.q.put(("oserr", str(e)))
                time.sleep(1)
                continue
            self.q.put(("pkt", data))

    def send(self, text=None):
        if self.sock is None:
            self.connect()                          # mo socket + gui loi chao de ESP32 biet dia chi PC
        if text is None:
            text = self.msg.get()
        data = text.encode("utf-8")
        if len(data) > N.MAX_MSG:
            self.log(f"Tin nhắn dài {len(data)} byte > {N.MAX_MSG}: đã cắt bớt.", "err")
            data = data[:N.MAX_MSG]
        self.seq += 1
        L = self.layer_idx()
        t0 = time.perf_counter()
        pkt = N.seal(N.TEXT, L, self.seq, self.session, N.DIR_PC_TO_ESP, data)
        dt = (time.perf_counter() - t0) * 1e3
        self.last_sent = pkt
        note = ""
        if self.tx_tamper.get():
            pkt, pos = flip_bit(pkt, REGIONS[self.region.get()])
            note = f"  ⚠ KẺ TẤN CÔNG lật 1 bit ở {self.region.get()} (byte {pos})"
        self.transmit(pkt)
        self.log(f"→ PC gửi #{self.seq} [{self.layer.get()}] \"{data.decode('utf-8', 'replace')}\" "
                 f"(Python mã hoá {dt:.1f} ms){note}", "err" if note else "tx")
        self.log("   " + short_hex(pkt), "hex")

    def replay(self):
        if self.last_sent is None:
            self.log("Chưa có gói nào để phát lại.", "err")
            return
        self.transmit(self.last_sent)
        _, _, seq = N.parse_header(self.last_sent)
        self.log(f"→ ⚠ KẺ TẤN CÔNG phát lại nguyên văn gói #{seq} (gói hợp lệ, đã gửi trước đó)", "err")

    def transmit(self, pkt):
        try:
            self.sock.sendto(pkt, self.addr())
            self.cnt["gửi"] += 1
        except OSError as e:
            self.log(f"Lỗi gửi: {e} — laptop đã nối Wi-Fi DATN-SMRM chưa?", "err")
        self.update_status()

    # ---------------------------------------------------------------- nhan
    def drain(self):
        try:
            while True:
                kind, data = self.q.get_nowait()
                if kind == "pkt":
                    self.on_packet(data)
                else:
                    self.log("Lỗi mạng: " + data, "err")
        except queue.Empty:
            pass
        self.root.after(50, self.drain)

    def on_packet(self, pkt):
        try:
            ptype, slayer, seq = N.parse_header(pkt)
        except N.NetError:
            self.log(f"← gói lạ {len(pkt)} byte (bỏ qua)", "err")
            return
        sname = N.LAYERS[slayer][0] if slayer < len(N.LAYERS) else "?"
        if ptype == N.NACK:
            self.cnt["ESP32 từ chối"] += 1
            self.update_status()
            self.log("← " + pkt[N.HDR:].decode("ascii", "replace") + "  [NACK: văn bản rõ, không xác thực]", "err")
            return
        note = ""
        if self.rx_tamper.get():
            pkt, pos = flip_bit(pkt, "ct")
            note = f" (⚠ kẻ tấn công đã lật bit byte {pos})"
        try:
            ptype, _, seq, _, msg = N.open_packet(pkt, N.DIR_ESP_TO_PC, self.layer_idx(), self.esp_state)
        except N.NetError as e:
            self.cnt["PC từ chối"] += 1
            self.update_status()
            self.log(f"← PC TỪ CHỐI gói #{seq} ({TYPE_VI.get(ptype, ptype)}) từ ESP32{note}: {e} "
                     f"[gói ghi tầng {sname}, PC dùng {self.layer.get()}]", "err")
            return
        self.cnt["PC nhận hợp lệ"] += 1
        self.update_status()
        text = msg.decode("utf-8", "replace")
        self.log(f"← ESP32 #{seq} [{sname}] {TYPE_VI.get(ptype, ptype)}, PC giải mã HỢP LỆ: {text}",
                 "tel" if ptype == N.TELEMETRY else "ok")


def main():
    root = tk.Tk()
    try:
        ttk.Style().theme_use("vista")
    except tk.TclError:
        pass
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
