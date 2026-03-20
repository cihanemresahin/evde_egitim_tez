# RPi — Robot Kontrol Birimi

HTTP video stream, WebSocket motor kontrolü ve servo pan/tilt.

## Dosyalar

| Dosya | Görev |
|-------|-------|
| `mjpeg_server.py` | 1280×720 MJPEG stream (port 8080) |
| `ws_server.py` | WebSocket sunucu (port 8765) — motor + servo |
| `servo_test.py` | Servo bağımsız test aracı |

## Çalıştırma

```bash
pip3 install -r requirements.txt
python3 mjpeg_server.py &
python3 ws_server.py
```

## Bağlantılar

- USB Kamera → RPi USB
- ESP32 → RPi USB Serial (`/dev/ttyUSB0`)
- Pan Servo → GPIO 12, Tilt Servo → GPIO 13
