# ESP32 — Motor + Engel Koruması

DC motor kontrolü ve ultrasonik engel algılama firmware'i.

## Dosyalar

| Dosya | Görev |
|-------|-------|
| `src/main.cpp` | Tek dosya firmware — motor, engel, serial komut |
| `platformio.ini` | PlatformIO yapılandırması |

## Bağlantılar

```
GPIO 5 (PWM) → L298N ENA    GPIO 4 (PWM) → L298N ENB
GPIO 18      → L298N IN1    GPIO 16      → L298N IN3
GPIO 19      → L298N IN2    GPIO 17      → L298N IN4
GPIO 25      → HC-SR04 TRIG
GPIO 26      → HC-SR04 ECHO (voltaj bölücü ile!)
```

## Derleme

```bash
cd mcu && pio run --target upload
```
