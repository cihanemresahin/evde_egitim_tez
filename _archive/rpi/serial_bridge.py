#!/usr/bin/env python3
"""
rpi/serial_bridge.py — M2: RPi → ESP32 Serial Köprüsü

Kullanım:
  python3 serial_bridge.py

Komutlar (terminal girişi):
  fwd    → <CMD:MOVE,DIR:FWD>
  bwd    → <CMD:MOVE,DIR:BWD>
  left   → <CMD:MOVE,DIR:LEFT>
  right  → <CMD:MOVE,DIR:RIGHT>
  stop   → <CMD:STOP>
  q      → çıkış

GPIO LED (pin 17): ESP32 bağlantısı açıkken ON, kapanınca OFF.
"""

import serial
import time
import sys
import threading

# ── AYARLAR ───────────────────────────────────────────────────────
SERIAL_PORT  = "/dev/ttyUSB0"   # veya /dev/ttyACM0
BAUD_RATE    = 115200
LED_PIN      = 17               # GPIO17 — yayın durumu LED

# Komut kısayolları → ESP32 formatı
KOMUTLAR = {
    "fwd":   "<CMD:MOVE,DIR:FWD>",
    "bwd":   "<CMD:MOVE,DIR:BWD>",
    "left":  "<CMD:MOVE,DIR:LEFT>",
    "right": "<CMD:MOVE,DIR:RIGHT>",
    "stop":  "<CMD:STOP>",
    "s":     "<CMD:STOP>",
}

# ── GPIO (RPi'de çalışıyorsa) ────────────────────────────────────
try:
    import RPi.GPIO as GPIO
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(LED_PIN, GPIO.OUT)
    GPIO.output(LED_PIN, GPIO.LOW)
    GPIO_AKTIF = True
except ImportError:
    print("[UYARI] RPi.GPIO bulunamadi — LED devre disi (gelistirme modu)")
    GPIO_AKTIF = False

def led_ac():
    if GPIO_AKTIF:
        GPIO.output(LED_PIN, GPIO.HIGH)

def led_kapat():
    if GPIO_AKTIF:
        GPIO.output(LED_PIN, GPIO.LOW)

# ── ESP32'DEN GELEN MESAJLARI OKU (arka plan thread) ─────────────
def serial_oku(ser):
    while True:
        try:
            satir = ser.readline().decode("utf-8", errors="ignore").strip()
            if satir:
                print(f"  ← ESP32: {satir}")
        except Exception:
            break

# ── ANA PROGRAM ──────────────────────────────────────────────────
def main():
    print("=" * 45)
    print("  M2 — RPi → ESP32 Serial Köprüsü")
    print("=" * 45)
    print(f"Port: {SERIAL_PORT} | Baud: {BAUD_RATE}")
    print("Komutlar: fwd / bwd / left / right / stop / q")
    print("-" * 45)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # ESP32 reset için bekle
        led_ac()
        print(f"[OK] ESP32 bağlandı — LED ON (GPIO {LED_PIN})\n")
    except serial.SerialException as e:
        print(f"[HATA] Serial port açılamadı: {e}")
        print("  → 'ls /dev/ttyUSB*' veya 'ls /dev/ttyACM*' ile portu kontrol edin")
        sys.exit(1)

    # Okuma thread'i başlat
    t = threading.Thread(target=serial_oku, args=(ser,), daemon=True)
    t.start()

    try:
        while True:
            girdi = input("→ Komut: ").strip().lower()
            if girdi == "q":
                break
            if girdi in KOMUTLAR:
                mesaj = KOMUTLAR[girdi] + "\n"
                ser.write(mesaj.encode())
                print(f"  → ESP32: {mesaj.strip()}")
            else:
                print(f"  [?] Bilinmeyen komut: '{girdi}'")
                print(f"  Geçerli: {', '.join(KOMUTLAR.keys())} | q (çıkış)")
    except KeyboardInterrupt:
        pass
    finally:
        # Temizlik
        ser.write(b"<CMD:STOP>\n")
        time.sleep(0.1)
        ser.close()
        led_kapat()
        if GPIO_AKTIF:
            GPIO.cleanup()
        print("\n[OK] Bağlantı kapatıldı — LED OFF")

if __name__ == "__main__":
    main()
