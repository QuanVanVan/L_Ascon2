"""
fake_esp32.py -- gia lap phia ESP32 tren PC de thu pc_peer_gui.py khi chua co board.
Hanh vi chep theo ascon_esp32.ino (ACK / NACK / goi so do moi 3 s / chong phat lai); MOI phep ma hoa va giai ma
di qua MA C (host_test.exe --seal / --open), nen GUI <-> fake_esp32 cung la kiem tra cheo C <-> Python.
  python fake_esp32.py [--layer L_Ascon2] [--port 4210]       (GUI: dat dia chi ESP32 = 127.0.0.1)
"""
import argparse
import os
import socket
import subprocess
import sys
import time

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "pc"))
import smrm_net as N  # noqa: E402

EXE = os.path.join(HERE, "host_test.exe")


def c_seal(ptype, layer, seq, session, msg):
    r = subprocess.run([EXE, "--seal", str(ptype), N.LAYERS[layer][0], str(seq), session.hex(),
                        str(N.DIR_ESP_TO_PC), msg.hex()], capture_output=True, text=True, check=True)
    return bytes.fromhex(r.stdout.strip())


def c_open(layer, pkt):
    r = subprocess.run([EXE, "--open", str(N.DIR_PC_TO_ESP), N.LAYERS[layer][0], pkt.hex()],
                       capture_output=True, text=True, check=True).stdout.split()
    if r[0] == "ERR":
        return int(r[1]), None
    return 0, (int(r[3]), bytes.fromhex(r[4]), bytes.fromhex(r[5]) if len(r) > 5 else b"")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--layer", default="L_Ascon2")
    ap.add_argument("--port", type=int, default=4210)
    ap.add_argument("--seconds", type=float, default=0, help="tu thoat sau N giay (0 = chay mai)")
    a = ap.parse_args()
    layer = N.layer_index(a.layer)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", a.port))
    s.settimeout(0.2)
    session, tx_seq, peer, last_tel = os.urandom(7), 0, None, time.time()
    pc_session, pc_last = None, 0
    n_ok = n_rej = 0
    t_end = time.time() + a.seconds if a.seconds else None
    print(f"ESP32 gia lap: 127.0.0.1:{a.port}, tang {a.layer}", flush=True)
    while t_end is None or time.time() < t_end:
        try:
            pkt, peer = s.recvfrom(2048)
        except socket.timeout:
            pkt = None
        if pkt is not None:
            rc, res = c_open(layer, pkt)
            if rc == 0:
                seq, ses, msg = res
                if ses == pc_session and seq <= pc_last:
                    rc = N.ERR_REPLAY
                else:
                    pc_session, pc_last = ses, seq
            try:
                _, _, hseq = N.parse_header(pkt)
            except N.NetError:
                hseq = 0
            if rc == 0:
                n_ok += 1
                text = msg.decode("utf-8", "replace")
                print(f"#{seq} HOP LE: {text}", flush=True)
                tx_seq += 1
                reply = f"ESP32 da nhan #{seq}: \"{text[:120]}\" (gia lap)".encode("utf-8")[:N.MAX_MSG]
                s.sendto(c_seal(N.ACK, layer, tx_seq, session, reply), peer)
            else:
                n_rej += 1
                why = f"ESP32 TU CHOI #{hseq}: {N.ERR_TEXT.get(rc, '?')} (ESP32 dung {a.layer})"
                print(why, flush=True)
                s.sendto(N.make_nack(layer, hseq, why), peer)
        if peer and time.time() - last_tel > 3:
            last_tel = time.time()
            tx_seq += 1
            tel = f"uptime gia lap | tang {a.layer} | da nhan: {n_ok} hop le, {n_rej} bi tu choi".encode()
            s.sendto(c_seal(N.TELEMETRY, layer, tx_seq, session, tel), peer)


if __name__ == "__main__":
    main()
