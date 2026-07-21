#!/usr/bin/env python3
"""
BLE Keyboard Tester
===================

toptal.com/developers/keycode  +  keyboardchecker.com  --> tek Python uygulamasi.

Ne yapar?
---------
- Ekranda gercekci bir klavye (Esc/F-tuslari, ana blok, ok tuslari, navigasyon
  kumesi ve numpad) cizer.
- pynput ile SISTEM GENELINDEKI tus olaylarini dinler (pencereye odaklanmasi
  gerekmez) -> ESP32/BLE klavyeniz bilgisayara HID klavye olarak bagliyken
  test etmek icin idealdir.
- Basili tutulan tus turuncu, en az bir kere basilip birakilan tus yesil
  ("test edildi") olur, hic dokunulmayanlar gri kalir. Boylece butun tuslarin
  calisip calismadigini gorsel olarak kontrol edebilirsiniz (keyboardchecker.com
  mantigi).
- Sag panelde toptal/keycode.info tarzinda son basilan tusun detaylari
  gosterilir: gorunen karakter, sembolik ad (pynput "Key.xxx"), varsa
  virtual-key (vk) kodu, ve o ana kadar kac kez basildigi.
- Alt kisimda son N tus olayinin log'u tutulur (zaman damgasi ile).

Kurulum
-------
    pip install pynput

Calistirma
----------
    python ble_keyboard_tester.py

Notlar
------
- macOS: Sistem Ayarlari > Gizlilik ve Guvenlik > Erisilebilirlik (Accessibility)
  izni vermeniz gerekir, yoksa global tus dinleme calismaz.
- Linux: bazi dagitimlarda X11/Wayland kisitlamalari veya `input` grubu izni
  gerekebilir.
- Klavye gorunumu ABD (US-QWERTY) fiziksel yerlesimine gore cizilmistir; amac
  metin girisi degil, HANGI TUSUN sinyal gonderdigini gormektir, bu yuzden
  Turkce/baska bir klavye surucusu kullansaniz bile pozisyon dogru kalir.
- Numpad tuslari ile ust siradaki rakamlar bazen ayni "id" ile eslenir (ozellikle
  Windows disinda); bu sadece gorsel testte ufak bir belirsizlik yaratir, HID
  tarafinda bir sorun degildir.
"""

import sys
import time
import queue
import threading
import tkinter as tk
from tkinter import ttk

try:
    from pynput import keyboard
except ImportError:
    print("Bu programin calismasi icin 'pynput' gerekli.\n  pip install pynput")
    sys.exit(1)


# --------------------------------------------------------------------------
# 1) KLAVYE YERLESIMI
# --------------------------------------------------------------------------
# Her tus: (match_ids, label, x, y, w, h)
#   match_ids : bu gorsel tusun eslesecegi pynput normalize-id listesi
#   x, y      : grid biriminde konum (1 birim = bir standart tus genisligi)
#   w, h      : grid biriminde genislik / yukseklik

def row(items, y, start_x=0.0, gap=0.0, h=1.0):
    """items: list of (ids, label, width) -> KEYS satirlarina cevirir."""
    out = []
    x = start_x
    for ids, label, w in items:
        out.append((ids, label, x, y, w, h))
        x += w + gap
    return out


KEYS = []

# --- Fonksiyon sirasi (y=0) ---
KEYS += row([(["esc"], "Esc", 1)], y=0, start_x=0)
KEYS += row([(["f1"], "F1", 1), (["f2"], "F2", 1), (["f3"], "F3", 1), (["f4"], "F4", 1)], y=0, start_x=2)
KEYS += row([(["f5"], "F5", 1), (["f6"], "F6", 1), (["f7"], "F7", 1), (["f8"], "F8", 1)], y=0, start_x=6.5)
KEYS += row([(["f9"], "F9", 1), (["f10"], "F10", 1), (["f11"], "F11", 1), (["f12"], "F12", 1)], y=0, start_x=11)

# --- Sayi sirasi (y=1.5) ---
KEYS += row([
    (["`"], "`", 1), (["1"], "1", 1), (["2"], "2", 1), (["3"], "3", 1), (["4"], "4", 1),
    (["5"], "5", 1), (["6"], "6", 1), (["7"], "7", 1), (["8"], "8", 1), (["9"], "9", 1),
    (["0"], "0", 1), (["-"], "-", 1), (["="], "=", 1), (["backspace"], "Backspace", 2),
], y=1.5)

# --- QWERTY sirasi (y=2.5) ---
KEYS += row([
    (["tab"], "Tab", 1.5), (["q"], "Q", 1), (["w"], "W", 1), (["e"], "E", 1), (["r"], "R", 1),
    (["t"], "T", 1), (["y"], "Y", 1), (["u"], "U", 1), (["i"], "I", 1), (["o"], "O", 1),
    (["p"], "P", 1), (["["], "[", 1), (["]"], "]", 1), (["\\"], "\\", 1.5),
], y=2.5)

# --- ASDF sirasi (y=3.5) ---
KEYS += row([
    (["caps_lock"], "Caps Lock", 1.75), (["a"], "A", 1), (["s"], "S", 1), (["d"], "D", 1),
    (["f"], "F", 1), (["g"], "G", 1), (["h"], "H", 1), (["j"], "J", 1), (["k"], "K", 1),
    (["l"], "L", 1), ([";"], ";", 1), (["'"], "'", 1), (["enter"], "Enter", 2.25),
], y=3.5)

# --- ZXCV sirasi (y=4.5) ---
KEYS += row([
    (["shift_l", "shift"], "Shift", 2.25), (["z"], "Z", 1), (["x"], "X", 1), (["c"], "C", 1),
    (["v"], "V", 1), (["b"], "B", 1), (["n"], "N", 1), (["m"], "M", 1), ([","], ",", 1),
    (["."], ".", 1), (["/"], "/", 1), (["shift_r", "shift"], "Shift", 2.75),
], y=4.5)

# --- Alt satir (y=5.5) ---
KEYS += row([
    (["ctrl_l", "ctrl"], "Ctrl", 1.25), (["cmd_l", "cmd"], "Win", 1.25), (["alt_l", "alt"], "Alt", 1.25),
    (["space"], "Space", 6.25), (["alt_r", "alt_gr", "alt"], "AltGr", 1.25), (["cmd_r", "cmd"], "Win", 1.25),
    (["menu"], "Menu", 1.25), (["ctrl_r", "ctrl"], "Ctrl", 1.25),
], y=5.5)

# --- Navigasyon kumesi ---
NAV_X = 15.5
KEYS += row([(["print_screen"], "PrtSc", 1), (["scroll_lock"], "ScrLk", 1), (["pause"], "Pause", 1)], y=0, start_x=NAV_X)
KEYS += row([(["insert"], "Insert", 1), (["home"], "Home", 1), (["page_up"], "PgUp", 1)], y=1.5, start_x=NAV_X)
KEYS += row([(["delete"], "Delete", 1), (["end"], "End", 1), (["page_down"], "PgDn", 1)], y=2.5, start_x=NAV_X)
KEYS += row([(["up"], "\u2191", 1)], y=4.5, start_x=NAV_X + 1)
KEYS += row([(["left"], "\u2190", 1), (["down"], "\u2193", 1), (["right"], "\u2192", 1)], y=5.5, start_x=NAV_X)

# --- Numpad ---
NP_X = 19.0
KEYS += row([(["num_lock"], "Num", 1), (["/"], "/", 1), (["*"], "*", 1)], y=1.5, start_x=NP_X)
KEYS += row([(["7"], "7", 1), (["8"], "8", 1), (["9"], "9", 1)], y=2.5, start_x=NP_X)
KEYS.append((["+"], "+", NP_X + 3, 2.5, 1, 2))
KEYS += row([(["4"], "4", 1), (["5"], "5", 1), (["6"], "6", 1)], y=3.5, start_x=NP_X)
KEYS += row([(["1"], "1", 1), (["2"], "2", 1), (["3"], "3", 1)], y=4.5, start_x=NP_X)
KEYS.append((["enter"], "Enter", NP_X + 3, 4.5, 1, 2))
KEYS.append((["0"], "0", NP_X, 5.5, 2, 1))
KEYS.append((["."], ".", NP_X + 2, 5.5, 1, 1))


# --------------------------------------------------------------------------
# 2) TUS OLAYLARINI NORMALLESTIRME
# --------------------------------------------------------------------------
# Shift ile basilan sembolleri temel tusa geri esler (US klavye varsayimiyla),
# boylece "!" basildiginda gorsel olarak "1" tusu vurgulanir.
SHIFTED_TO_BASE = {
    '!': '1', '@': '2', '#': '3', '$': '4', '%': '5', '^': '6', '&': '7',
    '*': '8', '(': '9', ')': '0', '_': '-', '+': '=', '{': '[', '}': ']',
    '|': '\\', ':': ';', '"': "'", '<': ',', '>': '.', '?': '/', '~': '`',
}


def normalize_key(key):
    """pynput key objesinden (match_id, gosterilecek_char, vk, ham_ad) uretir."""
    vk = getattr(key, "vk", None)
    char = getattr(key, "char", None)

    if char is not None:
        low = char.lower()
        match_id = SHIFTED_TO_BASE.get(low, low)
        raw = repr(key)
        return match_id, char, vk, raw

    # Ozel tus (Key enum)
    raw = str(key)
    name = raw.replace("Key.", "")
    return name, None, vk, raw


# --------------------------------------------------------------------------
# 3) ARAYUZ
# --------------------------------------------------------------------------
UNIT = 46          # bir grid biriminin piksel karsiligi
PAD = 14
GAP = 4            # tuslar arasi bosluk (px)

COLOR_UNTESTED = "#3a3f4b"
COLOR_HELD = "#e8a33d"
COLOR_TESTED = "#3fae5a"
COLOR_TEXT = "#f1f1f1"
BG = "#20232b"
PANEL_BG = "#262a33"


class KeyTesterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("BLE Keyboard Tester")
        self.root.configure(bg=BG)

        max_x = max(k[2] + k[4] for k in KEYS)
        max_y = max(k[3] + k[5] for k in KEYS)
        canvas_w = int(max_x * UNIT + PAD * 2)
        canvas_h = int(max_y * UNIT + PAD * 2)

        main = tk.Frame(root, bg=BG)
        main.pack(fill="both", expand=True)

        # --- Sol: klavye canvasi ---
        left = tk.Frame(main, bg=BG)
        left.pack(side="left", fill="both", expand=True, padx=10, pady=10)

        title = tk.Label(left, text="BLE Keyboard Tester", bg=BG, fg=COLOR_TEXT,
                          font=("Segoe UI", 16, "bold"))
        title.pack(anchor="w")

        subtitle = tk.Label(
            left,
            text="Klavyenizdeki her tusa basip birakin: gri = test edilmedi, "
                 "turuncu = basili, yesil = test edildi.",
            bg=BG, fg="#9aa0ac", font=("Segoe UI", 10), wraplength=canvas_w,
            justify="left",
        )
        subtitle.pack(anchor="w", pady=(0, 8))

        self.canvas = tk.Canvas(left, width=canvas_w, height=canvas_h,
                                 bg=BG, highlightthickness=0)
        self.canvas.pack()

        # --- Alt: durum cubugu + reset ---
        status_bar = tk.Frame(left, bg=BG)
        status_bar.pack(fill="x", pady=(8, 0))

        self.progress_var = tk.StringVar(value="Test edilen: 0 / {}".format(len(KEYS)))
        tk.Label(status_bar, textvariable=self.progress_var, bg=BG, fg=COLOR_TEXT,
                 font=("Segoe UI", 10, "bold")).pack(side="left")

        reset_btn = tk.Button(status_bar, text="Sifirla", command=self.reset_tested,
                               bg="#3a3f4b", fg=COLOR_TEXT, relief="flat", padx=12, pady=4)
        reset_btn.pack(side="right")

        # --- Alt: log ---
        log_frame = tk.Frame(left, bg=BG)
        log_frame.pack(fill="both", expand=True, pady=(10, 0))
        tk.Label(log_frame, text="Olay Gunlugu", bg=BG, fg=COLOR_TEXT,
                 font=("Segoe UI", 11, "bold")).pack(anchor="w")

        log_container = tk.Frame(log_frame, bg=BG)
        log_container.pack(fill="both", expand=True)
        self.log_text = tk.Text(log_container, height=8, bg="#14161b", fg="#c9cdd6",
                                 font=("Consolas", 9), relief="flat", wrap="none")
        self.log_text.pack(side="left", fill="both", expand=True)
        log_scroll = tk.Scrollbar(log_container, command=self.log_text.yview)
        log_scroll.pack(side="right", fill="y")
        self.log_text.configure(yscrollcommand=log_scroll.set)
        self.log_text.configure(state="disabled")

        # --- Sag panel: keycode detaylari (toptal/keycode.info stili) ---
        right = tk.Frame(main, bg=PANEL_BG, width=280)
        right.pack(side="right", fill="y")
        right.pack_propagate(False)

        tk.Label(right, text="Son Basilan Tus", bg=PANEL_BG, fg=COLOR_TEXT,
                 font=("Segoe UI", 14, "bold")).pack(anchor="w", padx=16, pady=(16, 6))

        self.detail_vars = {
            "key": tk.StringVar(value="-"),
            "char": tk.StringVar(value="-"),
            "vk": tk.StringVar(value="-"),
            "raw": tk.StringVar(value="-"),
            "state": tk.StringVar(value="-"),
            "count": tk.StringVar(value="0"),
        }
        labels = [
            ("key", "Eslesen ID"),
            ("char", "Karakter"),
            ("vk", "Virtual-Key (vk)"),
            ("raw", "pynput ham deger"),
            ("state", "Durum"),
            ("count", "Bu tus icin basma sayisi"),
        ]
        for key, label in labels:
            row_f = tk.Frame(right, bg=PANEL_BG)
            row_f.pack(fill="x", padx=16, pady=4)
            tk.Label(row_f, text=label, bg=PANEL_BG, fg="#9aa0ac",
                     font=("Segoe UI", 9)).pack(anchor="w")
            tk.Label(row_f, textvariable=self.detail_vars[key], bg=PANEL_BG, fg=COLOR_TEXT,
                     font=("Consolas", 12, "bold"), wraplength=250, justify="left").pack(anchor="w")

        ttk.Separator(right, orient="horizontal").pack(fill="x", padx=16, pady=12)

        tk.Label(right, text="Toplam basilan tus olayi:", bg=PANEL_BG, fg="#9aa0ac",
                 font=("Segoe UI", 9)).pack(anchor="w", padx=16)
        self.total_var = tk.StringVar(value="0")
        tk.Label(right, textvariable=self.total_var, bg=PANEL_BG, fg=COLOR_TEXT,
                 font=("Consolas", 16, "bold")).pack(anchor="w", padx=16, pady=(0, 12))

        # --- Veri yapilari ---
        self.rects = {}          # canvas_item_id -> key_index
        self.match_index = {}    # match_id -> [key_index, ...]
        self.state = ["untested"] * len(KEYS)
        self.press_count = [0] * len(KEYS)
        self.total_events = 0
        self.tested_count = 0

        self._draw_keys()

        # --- pynput dinleyici ile tkinter arasindaki kuyruk ---
        self.event_queue = queue.Queue()
        self.listener = keyboard.Listener(on_press=self._on_press, on_release=self._on_release)
        self.listener.daemon = True
        self.listener.start()

        self.root.after(15, self._poll_queue)
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ---------------------------------------------------------------- draw
    def _draw_keys(self):
        for idx, (ids, label, x, y, w, h) in enumerate(KEYS):
            x0 = PAD + x * UNIT + GAP / 2
            y0 = PAD + y * UNIT + GAP / 2
            x1 = PAD + (x + w) * UNIT - GAP / 2
            y1 = PAD + (y + h) * UNIT - GAP / 2

            rect = self.canvas.create_rectangle(
                x0, y0, x1, y1, fill=COLOR_UNTESTED, outline="#12141a", width=1.5,
            )
            font_size = 9 if len(label) > 2 else 11
            self.canvas.create_text(
                (x0 + x1) / 2, (y0 + y1) / 2, text=label, fill=COLOR_TEXT,
                font=("Segoe UI", font_size, "bold"),
            )
            self.rects[idx] = rect
            for mid in ids:
                self.match_index.setdefault(mid, []).append(idx)

    def _set_key_state(self, idx, new_state):
        if self.state[idx] == "tested" and new_state == "held":
            # zaten test edilmis bir tus tekrar basiliyor: gecici olarak
            # turuncuya donsun, birakinca tekrar yesile donecek.
            pass
        self.state[idx] = new_state
        color = {"untested": COLOR_UNTESTED, "held": COLOR_HELD, "tested": COLOR_TESTED}[new_state]
        self.canvas.itemconfig(self.rects[idx], fill=color)

    # ------------------------------------------------------- pynput thread
    def _on_press(self, key):
        self.event_queue.put(("press", key, time.time()))

    def _on_release(self, key):
        self.event_queue.put(("release", key, time.time()))

    # --------------------------------------------------------- main thread
    def _poll_queue(self):
        try:
            while True:
                kind, key, ts = self.event_queue.get_nowait()
                self._handle_event(kind, key, ts)
        except queue.Empty:
            pass
        self.root.after(15, self._poll_queue)

    def _handle_event(self, kind, key, ts):
        match_id, char, vk, raw = normalize_key(key)
        indices = self.match_index.get(match_id, [])

        if kind == "press":
            self.total_events += 1
            self.total_var.set(str(self.total_events))
            for idx in indices:
                self.press_count[idx] += 1
                if self.state[idx] != "tested":
                    self._set_key_state(idx, "held")
                else:
                    self._set_key_state(idx, "held")

            self._update_detail_panel(match_id, char, vk, raw, "BASILI", indices)
            self._append_log(ts, "DOWN", match_id, char, vk, raw)

        else:  # release
            for idx in indices:
                was_untested = self.state[idx] == "untested" or self.state[idx] == "held"
                if self.state[idx] != "tested":
                    self.tested_count += 1
                self._set_key_state(idx, "tested")
            self._update_detail_panel(match_id, char, vk, raw, "BIRAKILDI", indices)
            self._append_log(ts, "UP  ", match_id, char, vk, raw)
            self.progress_var.set("Test edilen: {} / {}".format(self.tested_count, len(KEYS)))

    def _update_detail_panel(self, match_id, char, vk, raw, state_label, indices):
        self.detail_vars["key"].set(match_id)
        self.detail_vars["char"].set(char if char is not None else "-")
        self.detail_vars["vk"].set(str(vk) if vk is not None else "-")
        self.detail_vars["raw"].set(raw)
        self.detail_vars["state"].set(state_label)
        count = self.press_count[indices[0]] if indices else 0
        self.detail_vars["count"].set(str(count))

    def _append_log(self, ts, direction, match_id, char, vk, raw):
        t = time.strftime("%H:%M:%S", time.localtime(ts))
        ms = int((ts - int(ts)) * 1000)
        line = "{}.{:03d}  {}  id={:<12} char={!r:<8} vk={!s:<6} raw={}\n".format(
            t, ms, direction, match_id, char, vk, raw
        )
        self.log_text.configure(state="normal")
        self.log_text.insert("end", line)
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def reset_tested(self):
        for idx in range(len(KEYS)):
            self._set_key_state(idx, "untested")
            self.press_count[idx] = 0
        self.tested_count = 0
        self.total_events = 0
        self.total_var.set("0")
        self.progress_var.set("Test edilen: 0 / {}".format(len(KEYS)))
        self.log_text.configure(state="normal")
        self.log_text.delete("1.0", "end")
        self.log_text.configure(state="disabled")

    def _on_close(self):
        try:
            self.listener.stop()
        except Exception:
            pass
        self.root.destroy()


def main():
    root = tk.Tk()
    app = KeyTesterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
