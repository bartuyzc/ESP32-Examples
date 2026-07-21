#!/usr/bin/env python3
"""
Klavye Test Uygulamasi v2 (evdev + tkinter) - Gelismis UI
------------------------------------------------------------
Fiziksel klavye VEYA BLE klavye (orn. ESP32 MacroPad) uzerinden gelen
her tus basimini yakalar; hangi cihazdan geldigini, key adini,
scancode/keycode'unu, basma/birakma durumunu gosterir.

Ek ozellikler (v2):
- Sekmeli arayuz: Canli Izleme / Istatistikler
- Cihaz basina renkli etiketler + baglanti durumu noktasi
- Basili tusta buyuk ekranda "flash" animasyonu
- Tus bazinda basim sayaci + en cok kullanilan tuslar (bar grafik benzeri)
- Log icinde arama/filtreleme
- Cihaz bazinda basim istatistigi

Gereksinim:
    sudo apt install python3-evdev
    (veya) pip install evdev --break-system-packages

Calistirma:
    sudo python3 keytest_gui_v2.py
"""

import threading
import queue
import time
import sys
import tkinter as tk
from tkinter import ttk

try:
    from evdev import InputDevice, categorize, ecodes, list_devices
except ImportError:
    print("evdev bulunamadi. Kurmak icin:")
    print("    sudo apt install python3-evdev")
    print("    (veya) pip install evdev --break-system-packages")
    sys.exit(1)


event_queue = queue.Queue()
stop_flag = threading.Event()
reader_threads = []

STATE_NAMES = {0: "BIRAKILDI", 1: "BASILDI", 2: "TUTULUYOR"}

# Cihazlara sirayla atanacak renk paleti
PALETTE = [
    "#00ff88", "#00c8ff", "#ffcc00", "#ff5588",
    "#a78bfa", "#ff8844", "#4ade80", "#f472b6",
]

FONT_MONO = "Consolas"


def is_keyboard_like(device):
    caps = device.capabilities()
    return ecodes.EV_KEY in caps


def get_keyboard_devices():
    devices = []
    for path in list_devices():
        try:
            dev = InputDevice(path)
            if is_keyboard_like(dev):
                devices.append(dev)
            else:
                dev.close()
        except (OSError, PermissionError):
            continue
    return devices


def read_device(device):
    try:
        for event in device.read_loop():
            if stop_flag.is_set():
                break
            if event.type == ecodes.EV_KEY:
                key_event = categorize(event)
                event_queue.put({
                    "device": device.name,
                    "path": device.path,
                    "keycode": key_event.keycode,
                    "scancode": key_event.scancode,
                    "state": key_event.keystate,
                    "time": time.strftime("%H:%M:%S"),
                    "ts": time.time(),
                })
    except (OSError, PermissionError):
        pass


class KeyTestApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Klavye Test Uygulamasi v2")
        self.root.geometry("880x680")
        self.root.configure(bg="#121212")
        self.root.minsize(760, 560)

        self.devices = []
        self.monitoring = False
        self.device_colors = {}
        self.color_index = 0

        self.key_counts = {}         # keycode -> count
        self.device_counts = {}      # device name -> count
        self.total_presses = 0

        self._setup_style()
        self._build_ui()
        self._refresh_devices()
        self._poll_queue()

    # ---------------------------------------------------------- STYLE
    def _setup_style(self):
        style = ttk.Style()
        style.theme_use("clam")

        style.configure("TFrame", background="#121212")
        style.configure("Card.TFrame", background="#1a1a1a")
        style.configure("TLabel", background="#121212", foreground="#e6e6e6")
        style.configure("Card.TLabel", background="#1a1a1a", foreground="#e6e6e6")
        style.configure("Sub.TLabel", background="#1a1a1a", foreground="#888888")

        style.configure("TCheckbutton", background="#1a1a1a", foreground="#e6e6e6")
        style.map("TCheckbutton", background=[("active", "#1a1a1a")])

        style.configure("Accent.TButton", background="#00ff88", foreground="#111111",
                         font=(FONT_MONO, 10, "bold"), padding=8, borderwidth=0)
        style.map("Accent.TButton", background=[("active", "#00cc6f")])

        style.configure("Stop.TButton", background="#ff5588", foreground="#111111",
                         font=(FONT_MONO, 10, "bold"), padding=8, borderwidth=0)
        style.map("Stop.TButton", background=[("active", "#cc3f66")])

        style.configure("Ghost.TButton", background="#2a2a2a", foreground="#e6e6e6",
                         font=(FONT_MONO, 9), padding=6, borderwidth=0)
        style.map("Ghost.TButton", background=[("active", "#3a3a3a")])

        style.configure("TNotebook", background="#121212", borderwidth=0)
        style.configure("TNotebook.Tab", background="#1a1a1a", foreground="#999999",
                         padding=(16, 8), font=(FONT_MONO, 10))
        style.map("TNotebook.Tab",
                  background=[("selected", "#121212")],
                  foreground=[("selected", "#00ff88")])

        style.configure("Treeview", background="#1a1a1a", foreground="#e6e6e6",
                         fieldbackground="#1a1a1a", rowheight=26, borderwidth=0,
                         font=(FONT_MONO, 10))
        style.configure("Treeview.Heading", background="#242424", foreground="#00ff88",
                         font=(FONT_MONO, 10, "bold"), borderwidth=0)
        style.map("Treeview", background=[("selected", "#2f6f4f")])

        style.configure("TEntry", fieldbackground="#1a1a1a", foreground="#e6e6e6",
                         insertcolor="#e6e6e6", borderwidth=0)

    def _next_color(self, key):
        if key not in self.device_colors:
            self.device_colors[key] = PALETTE[self.color_index % len(PALETTE)]
            self.color_index += 1
        return self.device_colors[key]

    # ---------------------------------------------------------- UI BUILD
    def _build_ui(self):
        self._build_header()

        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=14, pady=(0, 14))

        self.live_tab = ttk.Frame(self.notebook, style="TFrame")
        self.stats_tab = ttk.Frame(self.notebook, style="TFrame")
        self.notebook.add(self.live_tab, text="  Canlı İzleme  ")
        self.notebook.add(self.stats_tab, text="  İstatistikler  ")

        self._build_live_tab()
        self._build_stats_tab()

    def _build_header(self):
        header = ttk.Frame(self.root)
        header.pack(fill="x", padx=14, pady=14)

        title_box = ttk.Frame(header)
        title_box.pack(side="left")
        tk.Label(title_box, text="⌨  Klavye Test Uygulaması", bg="#121212", fg="#00ff88",
                 font=(FONT_MONO, 18, "bold")).pack(anchor="w")
        tk.Label(title_box, text="evdev tabanlı — fiziksel & BLE klavyeleri izler",
                 bg="#121212", fg="#777777", font=(FONT_MONO, 10)).pack(anchor="w")

        status_box = ttk.Frame(header)
        status_box.pack(side="right")
        self.status_dot = tk.Canvas(status_box, width=14, height=14, bg="#121212",
                                     highlightthickness=0)
        self.status_dot.pack(side="left", padx=(0, 6))
        self.status_dot_id = self.status_dot.create_oval(2, 2, 12, 12, fill="#ff5555", outline="")
        self.status_label = tk.Label(status_box, text="Durduruldu", bg="#121212",
                                      fg="#ff5555", font=(FONT_MONO, 11, "bold"))
        self.status_label.pack(side="left")

    def _build_live_tab(self):
        tab = self.live_tab

        # --- Cihaz kartı ---
        dev_card = tk.Frame(tab, bg="#1a1a1a")
        dev_card.pack(fill="x", pady=(14, 10))

        tk.Label(dev_card, text="İZLENECEK CİHAZLAR", bg="#1a1a1a", fg="#00ff88",
                 font=(FONT_MONO, 9, "bold")).pack(anchor="w", padx=14, pady=(10, 4))

        self.device_list_frame = tk.Frame(dev_card, bg="#1a1a1a")
        self.device_list_frame.pack(fill="x", padx=14, pady=(0, 10))

        btn_row = tk.Frame(dev_card, bg="#1a1a1a")
        btn_row.pack(fill="x", padx=14, pady=(0, 12))
        ttk.Button(btn_row, text="↻ Cihazları Yenile", style="Ghost.TButton",
                   command=self._refresh_devices).pack(side="left", padx=(0, 8))
        self.toggle_btn = ttk.Button(btn_row, text="▶ İzlemeyi Başlat", style="Accent.TButton",
                                      command=self._toggle_monitoring)
        self.toggle_btn.pack(side="left")

        # --- Buyuk tus gosterimi ---
        self.big_card = tk.Frame(tab, bg="#0c0c0c", bd=0, highlightthickness=2,
                                  highlightbackground="#2a2a2a")
        self.big_card.pack(fill="x", pady=(0, 12))

        self.key_name_label = tk.Label(self.big_card, text="—", bg="#0c0c0c", fg="#333333",
                                        font=(FONT_MONO, 48, "bold"))
        self.key_name_label.pack(pady=(20, 4))

        self.key_detail_label = tk.Label(self.big_card, text="Bir tuşa basın...",
                                          bg="#0c0c0c", fg="#666666", font=(FONT_MONO, 11))
        self.key_detail_label.pack(pady=(0, 20))

        # --- Arama + log ---
        search_row = tk.Frame(tab, bg="#121212")
        search_row.pack(fill="x", pady=(0, 6))
        tk.Label(search_row, text="🔍", bg="#121212", fg="#666666").pack(side="left")
        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", lambda *a: self._apply_filter())
        search_entry = ttk.Entry(search_row, textvariable=self.search_var, style="TEntry")
        search_entry.pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(search_row, text="Temizle", style="Ghost.TButton",
                   command=self._clear_log).pack(side="right")

        log_frame = tk.Frame(tab, bg="#121212")
        log_frame.pack(fill="both", expand=True)

        columns = ("time", "device", "keycode", "scancode", "state")
        self.tree = ttk.Treeview(log_frame, columns=columns, show="headings", height=14)
        headings = {"time": "Saat", "device": "Cihaz", "keycode": "Tuş Adı",
                    "scancode": "Scancode", "state": "Durum"}
        widths = {"time": 80, "device": 230, "keycode": 150, "scancode": 90, "state": 120}
        for col in columns:
            self.tree.heading(col, text=headings[col])
            self.tree.column(col, width=widths[col], anchor="w")
        self.tree.pack(side="left", fill="both", expand=True)

        scrollbar = ttk.Scrollbar(log_frame, orient="vertical", command=self.tree.yview)
        scrollbar.pack(side="right", fill="y")
        self.tree.configure(yscrollcommand=scrollbar.set)

        self.tree.tag_configure("down", foreground="#00ff88")
        self.tree.tag_configure("up", foreground="#666666")
        self.tree.tag_configure("hold", foreground="#ffcc00")

        self.all_rows = []  # filtreleme icin ham veri sakla

    def _build_stats_tab(self):
        tab = self.stats_tab

        top_row = tk.Frame(tab, bg="#121212")
        top_row.pack(fill="x", pady=(14, 10))

        self.total_card = self._make_stat_card(top_row, "TOPLAM BASIM", "0")
        self.total_card.pack(side="left", padx=(0, 10), fill="both", expand=True)

        self.unique_card = self._make_stat_card(top_row, "FARKLI TUŞ", "0")
        self.unique_card.pack(side="left", padx=(0, 10), fill="both", expand=True)

        self.devcount_card = self._make_stat_card(top_row, "AKTİF CİHAZ", "0")
        self.devcount_card.pack(side="left", fill="both", expand=True)

        # En cok kullanilan tuslar
        tk.Label(tab, text="EN ÇOK KULLANILAN TUŞLAR", bg="#121212", fg="#00ff88",
                 font=(FONT_MONO, 10, "bold")).pack(anchor="w", pady=(10, 6))

        self.bars_frame = tk.Frame(tab, bg="#1a1a1a")
        self.bars_frame.pack(fill="x", pady=(0, 14))

        # Cihaz bazinda dagilim
        tk.Label(tab, text="CİHAZ BAŞINA BASIM", bg="#121212", fg="#00ff88",
                 font=(FONT_MONO, 10, "bold")).pack(anchor="w", pady=(0, 6))

        self.device_bars_frame = tk.Frame(tab, bg="#1a1a1a")
        self.device_bars_frame.pack(fill="both", expand=True)

    def _make_stat_card(self, parent, title, value):
        card = tk.Frame(parent, bg="#1a1a1a")
        tk.Label(card, text=title, bg="#1a1a1a", fg="#888888",
                 font=(FONT_MONO, 9, "bold")).pack(anchor="w", padx=14, pady=(12, 2))
        val_label = tk.Label(card, text=value, bg="#1a1a1a", fg="#00ff88",
                              font=(FONT_MONO, 26, "bold"))
        val_label.pack(anchor="w", padx=14, pady=(0, 12))
        card.value_label = val_label
        return card

    # ---------------------------------------------------------- CIHAZLAR
    def _refresh_devices(self):
        for widget in self.device_list_frame.winfo_children():
            widget.destroy()
        self.device_vars = {}

        try:
            self.devices = get_keyboard_devices()
        except PermissionError:
            tk.Label(self.device_list_frame, text="Yetki hatası! 'sudo' ile çalıştırın.",
                     bg="#1a1a1a", fg="#ff5555").pack(anchor="w")
            return

        if not self.devices:
            tk.Label(self.device_list_frame, text="Klavye benzeri cihaz bulunamadı.",
                     bg="#1a1a1a", fg="#ff5555").pack(anchor="w")
            return

        for dev in self.devices:
            color = self._next_color(dev.path)
            row = tk.Frame(self.device_list_frame, bg="#1a1a1a")
            row.pack(fill="x", pady=2)

            dot = tk.Canvas(row, width=10, height=10, bg="#1a1a1a", highlightthickness=0)
            dot.pack(side="left", padx=(0, 8))
            dot.create_oval(1, 1, 9, 9, fill=color, outline="")

            var = tk.BooleanVar(value=True)
            cb = ttk.Checkbutton(row, text=f"{dev.name}   ({dev.path})", variable=var)
            cb.pack(side="left")
            self.device_vars[dev.path] = var

    def _toggle_monitoring(self):
        if self.monitoring:
            self._stop_monitoring()
        else:
            self._start_monitoring()

    def _start_monitoring(self):
        selected_paths = [p for p, v in self.device_vars.items() if v.get()]
        if not selected_paths:
            self.key_detail_label.config(text="Hiçbir cihaz seçilmedi!", fg="#ff5555")
            return

        stop_flag.clear()
        reader_threads.clear()

        for dev in self.devices:
            if dev.path in selected_paths:
                t = threading.Thread(target=read_device, args=(dev,), daemon=True)
                t.start()
                reader_threads.append(t)

        self.monitoring = True
        self.toggle_btn.config(text="■ İzlemeyi Durdur", style="Stop.TButton")
        self.status_label.config(text="İzleniyor...", fg="#00ff88")
        self.status_dot.itemconfig(self.status_dot_id, fill="#00ff88")
        self.devcount_card.value_label.config(text=str(len(selected_paths)))

    def _stop_monitoring(self):
        stop_flag.set()
        self.monitoring = False
        self.toggle_btn.config(text="▶ İzlemeyi Başlat", style="Accent.TButton")
        self.status_label.config(text="Durduruldu", fg="#ff5555")
        self.status_dot.itemconfig(self.status_dot_id, fill="#ff5555")

    def _clear_log(self):
        self.all_rows.clear()
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.key_counts.clear()
        self.device_counts.clear()
        self.total_presses = 0
        self._refresh_stats()

    # ---------------------------------------------------------- FILTER
    def _apply_filter(self):
        query = self.search_var.get().lower().strip()
        for item in self.tree.get_children():
            self.tree.delete(item)
        for row in self.all_rows:
            haystack = " ".join(str(v) for v in row["values"]).lower()
            if query in haystack:
                self.tree.insert("", "end", values=row["values"], tags=row["tags"])

    # ---------------------------------------------------------- EVENTS
    def _poll_queue(self):
        try:
            while True:
                item = event_queue.get_nowait()
                self._handle_event(item)
        except queue.Empty:
            pass
        self.root.after(30, self._poll_queue)

    def _handle_event(self, item):
        keycode = item["keycode"]
        if isinstance(keycode, list):
            keycode = " / ".join(keycode)

        state = item["state"]
        state_text = STATE_NAMES.get(state, str(state))
        tag = {0: "up", 1: "down", 2: "hold"}.get(state, "")
        color = self._next_color(item["path"])

        if state == 1:
            self._flash_key(str(keycode).replace("KEY_", ""), item["device"], item["scancode"], color)
            self.key_counts[keycode] = self.key_counts.get(keycode, 0) + 1
            self.device_counts[item["device"]] = self.device_counts.get(item["device"], 0) + 1
            self.total_presses += 1
            self._refresh_stats()

        values = (item["time"], item["device"], keycode, item["scancode"], state_text)
        self.tree.insert("", 0, values=values, tags=(tag,))
        self.all_rows.insert(0, {"values": values, "tags": (tag,)})

        if len(self.all_rows) > 800:
            self.all_rows.pop()
        children = self.tree.get_children()
        if len(children) > 800:
            self.tree.delete(children[-1])

    def _flash_key(self, key_text, device, scancode, color):
        self.key_name_label.config(text=key_text, fg=color)
        self.key_detail_label.config(text=f"{device}   |   scancode={scancode}", fg="#aaaaaa")
        self.big_card.config(highlightbackground=color)

        def fade():
            self.big_card.config(highlightbackground="#2a2a2a")

        self.root.after(250, fade)

    # ---------------------------------------------------------- STATS
    def _refresh_stats(self):
        self.total_card.value_label.config(text=str(self.total_presses))
        self.unique_card.value_label.config(text=str(len(self.key_counts)))

        self._render_bars(self.bars_frame, self.key_counts, top_n=8)
        self._render_bars(self.device_bars_frame, self.device_counts, top_n=8)

    def _render_bars(self, container, counts, top_n=8):
        for widget in container.winfo_children():
            widget.destroy()

        if not counts:
            tk.Label(container, text="Henüz veri yok.", bg="#1a1a1a", fg="#666666",
                      font=(FONT_MONO, 10)).pack(anchor="w", padx=14, pady=10)
            return

        ordered = sorted(counts.items(), key=lambda kv: kv[1], reverse=True)[:top_n]
        max_val = max(v for _, v in ordered) if ordered else 1

        for name, count in ordered:
            row = tk.Frame(container, bg="#1a1a1a")
            row.pack(fill="x", padx=14, pady=4)

            label = str(name)
            if isinstance(name, list):
                label = " / ".join(name)
            label = label.replace("KEY_", "")

            tk.Label(row, text=label, bg="#1a1a1a", fg="#e6e6e6",
                     font=(FONT_MONO, 10), width=18, anchor="w").pack(side="left")

            bar_bg = tk.Canvas(row, height=16, bg="#1a1a1a", highlightthickness=0)
            bar_bg.pack(side="left", fill="x", expand=True, padx=8)

            def draw_bar(canvas=bar_bg, value=count, maximum=max_val):
                canvas.update_idletasks()
                w = canvas.winfo_width() or 300
                filled = int((value / maximum) * w)
                canvas.delete("all")
                canvas.create_rectangle(0, 0, w, 16, fill="#242424", outline="")
                canvas.create_rectangle(0, 0, filled, 16, fill="#00ff88", outline="")

            container.after(10, draw_bar)

            tk.Label(row, text=str(count), bg="#1a1a1a", fg="#888888",
                     font=(FONT_MONO, 10), width=5, anchor="e").pack(side="left")


def main():
    root = tk.Tk()
    app = KeyTestApp(root)

    def on_close():
        stop_flag.set()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
