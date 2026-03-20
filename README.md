# Evde Eğitim — VR Telepresence Robot

Evde eğitim alan öğrencilerin VR başlık ile sınıf ortamına uzaktan katılmasını sağlayan telepresence robot sistemi.

## Mimari

```
Quest 3 (Unity)  ←→  Raspberry Pi  ←→  ESP32
   ↓ Joystick        ↓ WebSocket       ↓ Motor
   ↓ Head Track       ↓ HTTP MJPEG      ↓ Engel Sensörü
   ↓ VR Ekran         ↓ Servo PWM
```

## Klasör Yapısı

```
mcu/          ESP32 firmware (motor + engel koruması)
rpi/          RPi servisleri (video stream + WebSocket + servo)
meta/Scripts/ Unity C# scriptleri (kontrol + video)
_archive/     Eski/kullanılmayan dosyalar
```

## Çalıştırma

**RPi:**
```bash
python3 ~/proje/mjpeg_server.py &
python3 ~/proje/ws_server.py
```

**Unity:** Play tuşuna bas (Quest bağlı)

**ESP32:** PlatformIO ile `mcu/` klasöründen yükle

## Güvenlik

| Katman | Mekanizma |
|--------|-----------|
| Unity | Otomatik yeniden bağlantı, hız sınırı, kapanma STOP |
| RPi | Heartbeat watchdog (2s), serial auto-reconnect |
| ESP32 | 500ms komut timeout, 15cm engel koruması |
