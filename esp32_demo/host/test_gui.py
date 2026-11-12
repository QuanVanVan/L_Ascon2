"""
test_gui.py -- thu tu dong pc_peer_gui.py voi fake_esp32.py (127.0.0.1) theo dung kich ban demo:
  gui hop le -> ACK; lat bit ban ma / the / nonce / header -> ESP32 tu choi; phat lai -> tu choi;
  sua goi nhan -> PC tu choi; PC doi sang Ascon goc (ESP32 van L_Ascon2) -> ca hai chieu tu choi; doi lai -> hop le.
Phai in "TEST GUI: TAT CA PASS".
"""
import os
import subprocess
import sys
import time

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "pc"))
import tkinter as tk  # noqa: E402

import pc_peer_gui as G  # noqa: E402

PORT = 47210
fails = 0


def pump(root, sec):
    t = time.time() + sec
    while time.time() < t:
        root.update()
        time.sleep(0.02)


def main():
    global fails
    esp = subprocess.Popen([sys.executable, os.path.join(HERE, "fake_esp32.py"), "--port", str(PORT),
                            "--seconds", "60"], stdout=subprocess.PIPE, text=True)
    time.sleep(1.0)
    root = tk.Tk()
    app = G.App(root)
    app.ip.set("127.0.0.1")
    app.port.set(str(PORT))

    def step(name, action, want):
        global fails
        before = dict(app.cnt)
        action()
        pump(root, 1.2)
        diff = {k: app.cnt[k] - before[k] for k in app.cnt if app.cnt[k] != before[k]}
        diff.pop("gửi", None)
        unexpected = [k for k in ("ESP32 từ chối", "PC từ chối") if diff.get(k, 0) and k not in want]
        ok = all(diff.get(k, 0) >= v for k, v in want.items()) and not unexpected
        print(f"  {'OK  ' if ok else 'FAIL'} {name}: {diff}")
        fails += not ok

    step("ket noi + gui hop le", lambda: app.send("chao"), {"PC nhận hợp lệ": 1})
    for region in G.REGIONS:
        def tamper(r=region):
            app.tx_tamper.set(True); app.region.set(r); app.send("bi sua"); app.tx_tamper.set(False)
        step(f"lat bit {region}", tamper, {"ESP32 từ chối": 1})
    step("gui hop le lai", lambda: app.send("hop le"), {"PC nhận hợp lệ": 1})
    step("phat lai goi vua gui", app.replay, {"ESP32 từ chối": 1})

    def rx_tamper():
        app.rx_tamper.set(True); app.send("thu sua chieu ve")
    step("sua goi nhan (ACK)", rx_tamper, {"PC từ chối": 1})
    app.rx_tamper.set(False)

    def mismatch():
        app.layer.set("ascon"); app.send("khac tang")
    step("PC dung Ascon goc, ESP32 dung L_Ascon2", mismatch, {"ESP32 từ chối": 1})
    pump(root, 3.5)                                 # goi so do tu ESP32 (L_Ascon2) -> PC (ascon) phai tu choi
    print(f"  {'OK  ' if app.cnt['PC từ chối'] >= 2 else 'FAIL'} goi so do bi PC (ascon) tu choi")
    fails += app.cnt["PC từ chối"] < 2
    app.layer.set("L_Ascon2")
    step("doi lai L_Ascon2", lambda: app.send("lai hop le"), {"PC nhận hợp lệ": 1})

    log = app.text.get("1.0", "end")
    root.destroy()
    esp.kill()
    print("---- trich log GUI ----")
    print("\n".join(log.splitlines()[-12:]))
    print("TEST GUI:", "TAT CA PASS" if fails == 0 else f"{fails} LOI")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
