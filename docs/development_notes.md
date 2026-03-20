# Yerel Geliştirme Notları

> Bu belge, projeyi kendi makinenizde çalıştırmak için gereken
> temel bilgileri içerir.

## 🖥️ Geliştirme Ortamı

| Bileşen | Araç | Versiyon |
|---------|------|----------|
| Unity (Quest 3) | Unity Hub + Unity Editor | 2022.3 LTS |
| RPi kodu | VS Code + SSH Remote | Python 3.11+ |
| MCU kodu | VS Code + PlatformIO | Arduino framework |
| Cloud sunucu | VS Code + Node.js | Node 20 LTS |
| TURN sunucu | Docker veya doğrudan kurulum | coturn 4.6+ |

## 📋 Ön Koşullar

1. **Git** kurulu olmalı
2. **Node.js 20 LTS** (cloud geliştirme için)
3. **Python 3.11+** (RPi simülasyonu için)
4. **PlatformIO CLI** (MCU geliştirme için)
5. **Unity 2022.3 LTS** + Android Build Support

## 🔧 İlk Kurulum Adımları

```bash
# 1. Repoyu klonla
git clone <repo-url>
cd evde_egitim_tez

# 2. Ortam değişkenlerini hazırla
cp .env.example .env
# Her alt dizinde de:
cp cloud/.env.example cloud/.env
cp rpi/.env.example rpi/.env

# 3. .env dosyalarını düzenle (gerçek değerleri gir)
```

## 🧪 Modül Bazında Test

Her modül bağımsız test edilebilir. Sıra:

1. **M1 (Cloud)**: `cd cloud && npm start` → WSS bağlantı testi
2. **M2 (RPi)**: `cd rpi && python webrtc/publisher.py` → Video yayın
3. **M5 (MCU)**: `cd mcu && pio run` → Serial komut simülasyonu
4. **M3 (Unity)**: Unity Editor'de Play → Video alım testi

## ⚠️ Windows'ta Bilinen Kısıtlamalar

- RPi GPIO ve serial kodu Windows'ta doğrudan çalışmaz
  - RPi kodunu test etmek için SSH ile gerçek RPi veya WSL kullanın
- MCU kodu PlatformIO ile derlenip ESP32/Arduino'ya yüklenir
- Unity projesi Windows'ta normal çalışır (Quest Link ile test)

## 🔑 .env Dosyaları Hakkında

> [!CAUTION]
> `.env` dosyaları Git'e **asla** eklenmez. Her ortam için ayrı `.env`
> oluşturulmalıdır. Gerçek anahtarları `.env.example` dosyasına yazmayın!
