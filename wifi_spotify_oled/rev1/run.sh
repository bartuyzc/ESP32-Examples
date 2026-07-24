#!/bin/bash
# Betiğin bulunduğu dizine git
cd "$(dirname "$0")"

# Sanal ortam yoksa oluştur ve kütüphaneleri yükle
if [ ! -d ".venvs/venv" ]; then
    echo "Sanal ortam oluşturuluyor..."
    python3 -m venv .venvs/venv
    source venv/bin/activate
    pip install -r requirements.txt
else
    source .venvs/venv/bin/activate
fi

# Uygulamayı çalıştır
python3 spotify_to_esp32.py
