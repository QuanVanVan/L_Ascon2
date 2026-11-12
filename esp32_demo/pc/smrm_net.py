"""
smrm_net.py -- giao thuc goi tin ESP32 <-> PC (ban Python, doc lap voi ban C trong ascon_esp32/demo_core.c).
Dac ta: esp32_demo/GIAO_THUC.md. Ma hoa bang ascon_aead.py (cai dat tham chieu da khop 1089/1089 KAT chinh thuc).

  goi    = header (9 byte) | nonce (16) | C | T (16)
  header = 'S' 'M' | ver=1 | type | layer | seq (uint32 LE)       -- duoc xac thuc (dung lam AD)
  nonce  = dir | session (7 byte ngau nhien moi phien) | seq (uint32 LE) | 00 00 00 00
  NACK   = header (type 4) | ly do (ASCII) -- KHONG ma hoa, KHONG xac thuc (chi de hien thi trang thai)
"""
import struct

from ascon_aead import AsconAEAD128
from layers_spec import LAYERS

VERSION = 1
HDR = 9
MAX_MSG = 200
TEXT, TELEMETRY, ACK, NACK = 1, 2, 3, 4
TYPE_NAMES = {TEXT: "TEXT", TELEMETRY: "TELEMETRY", ACK: "ACK", NACK: "NACK"}
DIR_PC_TO_ESP, DIR_ESP_TO_PC = 1, 2
KEY = bytes(range(16))                                   # khoa chia se truoc -- CHI DE DEMO

OK, ERR_FORMAT, ERR_DIR, ERR_AUTH, ERR_REPLAY = 0, -1, -2, -3, -4
ERR_TEXT = {ERR_FORMAT: "sai dinh dang", ERR_DIR: "sai chieu (nonce)",
            ERR_AUTH: "the xac thuc SAI (du lieu bi sua hoac khac tang)", ERR_REPLAY: "PHAT LAI (seq da dung)"}

_aead = {}


def layer_index(name):
    for i, (n, _) in enumerate(LAYERS):
        if n == name:
            return i
    raise KeyError(name)


def aead(idx):
    if idx not in _aead:
        _aead[idx] = AsconAEAD128(LAYERS[idx][1])
    return _aead[idx]


class NetError(Exception):
    def __init__(self, code):
        super().__init__(ERR_TEXT.get(code, "?"))
        self.code = code


class Peer:
    """Trang thai chong phat lai cua mot ben gui."""
    def __init__(self):
        self.session = None
        self.last_seq = 0


def header(ptype, layer, seq):
    return b"SM" + bytes([VERSION, ptype, layer]) + struct.pack("<I", seq)


def nonce(direction, session, seq):
    assert len(session) == 7
    return bytes([direction]) + session + struct.pack("<I", seq) + bytes(4)


def seal(ptype, layer, seq, session, direction, msg):
    assert len(msg) <= MAX_MSG and 0 <= layer < len(LAYERS)
    h, n = header(ptype, layer, seq), nonce(direction, session, seq)
    return h + n + aead(layer).encrypt(KEY, n, h, msg)


def parse_header(pkt):
    if len(pkt) < HDR or pkt[:2] != b"SM" or pkt[2] != VERSION:
        raise NetError(ERR_FORMAT)
    return pkt[3], pkt[4], struct.unpack("<I", pkt[5:9])[0]


def open_packet(pkt, direction, layer, peer=None):
    """Mo goi bang tang CUA BEN NHAN. Tra (type, layer_nguoi_gui, seq, session, msg); loi -> NetError."""
    if len(pkt) < HDR + 32 or len(pkt) > HDR + 32 + MAX_MSG:
        raise NetError(ERR_FORMAT)
    ptype, slayer, seq = parse_header(pkt)
    if ptype == NACK:
        raise NetError(ERR_FORMAT)
    n = pkt[HDR:HDR + 16]
    if n[0] != direction:
        raise NetError(ERR_DIR)
    if struct.unpack("<I", n[8:12])[0] != seq or n[12:] != bytes(4):
        raise NetError(ERR_FORMAT)
    msg = aead(layer).decrypt(KEY, n, pkt[:HDR], pkt[HDR + 16:])
    if msg is None:
        raise NetError(ERR_AUTH)
    session = n[1:8]
    if peer is not None:                                 # chi xet sau khi xac thuc
        if peer.session == session and seq <= peer.last_seq:
            raise NetError(ERR_REPLAY)
        peer.session, peer.last_seq = session, seq
    return ptype, slayer, seq, session, msg


def make_nack(layer, seq, reason):
    return header(NACK, layer, seq) + reason.encode("ascii", "replace")[:120]
