import subprocess
import requests
import time
import io
from PIL import Image

ESP32_IP = "192.168.1.92"

def run_playerctl(*args):
    try:
        return subprocess.check_output(
            ["playerctl", "-p", "spotify", *args],
            stderr=subprocess.DEVNULL
        ).decode().strip()
    except subprocess.CalledProcessError:
        return None

def clean_text(text):
    if not text:
        return ""
    
    # Türkçe karakterlerin ASCII karşılıkları
    tr_map = {
        'ç': 'c', 'Ç': 'C',
        'ğ': 'g', 'Ğ': 'G',
        'ı': 'i', 'I': 'I',
        'İ': 'I',
        'ö': 'o', 'Ö': 'O',
        'ş': 's', 'Ş': 'S',
        'ü': 'u', 'Ü': 'U',
        '’': "'", '‘': "'", '`': "'",  # Tipografik tırnak/kesme işaretleri
        '“': '"', '”': '"',
        '–': '-', '—': '-'             # Özel tire işaretleri
    }
    
    for tr, ascii_char in tr_map.items():
        text = text.replace(tr, ascii_char)
        
    return text

def get_metadata():
    artist = clean_text(run_playerctl("metadata", "artist"))
    title = clean_text(run_playerctl("metadata", "title"))
    
    status = run_playerctl("status")
    art_url = run_playerctl("metadata", "mpris:artUrl")

    position_raw = run_playerctl("position")
    length_raw = run_playerctl("metadata", "mpris:length")

    try:
        position = int(float(position_raw)) if position_raw else 0
    except ValueError:
        position = 0

    try:
        duration = int(int(length_raw) / 1_000_000) if length_raw else 0
    except ValueError:
        duration = 0

    return artist, title, status, position, duration, art_url

def process_and_send_art(art_url):
    """Resmi indirir, 48x48'e boyutlandırıp Dithering ile 1-bit monochroma çevirir ve ESP32'ye gönderir."""
    try:
        if art_url.startswith("file://"):
            file_path = art_url.replace("file://", "")
            img = Image.open(file_path)
        elif art_url.startswith("http"):
            resp = requests.get(art_url, timeout=3)
            img = Image.open(io.BytesIO(resp.content))
        else:
            return False

        # Pillow sürüm uyumluluk kontrolleri
        try:
            resample_mode = Image.Resampling.LANCZOS
        except AttributeError:
            resample_mode = Image.LANCZOS

        try:
            dither_mode = Image.Dither.FLOYDSTEINBERG
        except AttributeError:
            dither_mode = Image.FLOYDSTEINBERG

        # 48x48 piksel boyutlandırma ve Dithering
        img = img.convert("L").resize((48, 48), resample_mode)
        img = img.convert("1", dither=dither_mode)

# 1-bit GFX Bitmap Formatına Dönüştürme (288 byte)
        raw_bytes = bytearray()
        pixels = img.load()
        for y in range(48):
            for x_byte in range(6):
                b = 0
                for bit in range(8):
                    x = x_byte * 8 + bit
                    if pixels[x, y] > 0:
                        b |= (1 << (7 - bit))
                raw_bytes.append(b)

        # 288 byte binary veriyi 576 karakterlik Hex string'e çevir
        hex_data = raw_bytes.hex()

        # /art endpoint'ine metin olarak gönder
        res = requests.post(
            f"http://{ESP32_IP}/art",
            data=hex_data,
            headers={"Content-Type": "text/plain"},
            timeout=3
        )
        if res.status_code == 200:
            print("Album kapağı başarıyla gönderildi.")
            return True
        else:
            print(f"ESP32 kapak resmi hatası: HTTP {res.status_code}")
            return False

    except Exception as e:
        print("Kapak resmi işlenirken veya gönderilirken hata oluştu:", e)
        return False

last_sent_art = None
last_sent_track = None

while True:
    artist, title, status, position, duration, art_url = get_metadata()

    if artist and title:
        # Kapak resmi değişmişse veya önceki gönderim başarısız olduysa tekrar dene
        if art_url and art_url != last_sent_art:
            success = process_and_send_art(art_url)
            if success:
                last_sent_art = art_url  # Sadece başarılı olursa güncelliyoruz

        payload = {
            "artist": artist,
            "title": title,
            "position": position,
            "duration": duration,
            "playing": (status == "Playing"),
        }

        compare_key = (artist, title, payload["playing"])
        if compare_key != last_sent_track:
            print("Track Bilgisi Gönderiliyor:", payload)
            last_sent_track = compare_key

        try:
            requests.post(f"http://{ESP32_IP}/update", json=payload, timeout=2)
        except requests.exceptions.RequestException as e:
            print("ESP32'ye ulaşılamadı:", e)

    time.sleep(1)
