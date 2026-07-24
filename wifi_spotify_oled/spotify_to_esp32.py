import subprocess
import requests
import time

ESP32_IP = "192.168.1.92"  # ESP32'nin IP'si

def run_playerctl(*args):
    try:
        return subprocess.check_output(
            ["playerctl", "-p", "spotify", *args],
            stderr=subprocess.DEVNULL
        ).decode().strip()
    except subprocess.CalledProcessError:
        return None

def get_metadata():
    artist = run_playerctl("metadata", "artist")
    title = run_playerctl("metadata", "title")
    status = run_playerctl("status")  # "Playing" / "Paused"

    position_raw = run_playerctl("position")            # saniye (float, string)
    length_raw = run_playerctl("metadata", "mpris:length")  # mikrosaniye

    try:
        position = int(float(position_raw)) if position_raw else 0
    except ValueError:
        position = 0

    try:
        duration = int(int(length_raw) / 1_000_000) if length_raw else 0
    except ValueError:
        duration = 0

    return artist, title, status, position, duration

last_sent = None
while True:
    artist, title, status, position, duration = get_metadata()

    if artist and title:
        payload = {
            "artist": artist,
            "title": title,
            "position": position,
            "duration": duration,
            "playing": (status == "Playing"),
        }

        # Sadece anlamli bir degisiklik oldugunda gonder (sarki/durum degisimi)
        compare_key = (artist, title, payload["playing"])
        if compare_key != last_sent:
            print("Gonderiliyor:", payload)
        last_sent = compare_key

        try:
            requests.post(f"http://{ESP32_IP}/update", json=payload, timeout=2)
        except requests.exceptions.RequestException as e:
            print("ESP32'ye ulasilamadi:", e)

    time.sleep(1)  # progress bar'in akici gorunmesi icin 1sn'ye dusuruldu
