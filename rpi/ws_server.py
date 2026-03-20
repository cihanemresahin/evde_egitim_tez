#!/usr/bin/env python3
"""
ws_server.py — M10: RPi → Cloud Relay + Motor + Servo

Cloud relay'e bağlanır, oda kodu alır.
Quest'ten gelen komutları ESP32'ye ve servo'ya iletir.
Kamera frame'lerini relay'e gönderir (binary JPEG).

Kullanım:
  python3 ws_server.py                         # varsayılan relay
  RELAY_URL=wss://xxx.onrender.com python3 ws_server.py  # özel relay
"""

import asyncio
import json
import time
import os
import cv2
import serial
import websockets

# ── AYARLAR ─────────────────────────────────────────────────────
RELAY_URL    = os.environ.get("RELAY_URL", "wss://vr-relay-ricg.onrender.com/ws?role=rpi")
SERIAL_PORT  = "/dev/ttyUSB0"
BAUD_RATE    = 115200

# Kamera
CAM_WIDTH, CAM_HEIGHT = 1280, 720
JPEG_QUALITY = 80
VIDEO_FPS    = 15  # Cloud üzerinden gideceği için 15 FPS yeterli

# Güvenlik
HEARTBEAT_TIMEOUT = 2.0

# Servo
PAN_STEPS  = [0, 30, 60, 90, 120, 150, 180]
TILT_STEPS = [0, 30, 60, 90, 120, 150, 180]
SERVO_CHECK_INTERVAL = 1.0

DIR_MAP = {
    "fwd":   "<CMD:MOVE,DIR:FWD>",
    "bwd":   "<CMD:MOVE,DIR:BWD>",
    "left":  "<CMD:MOVE,DIR:LEFT>",
    "right": "<CMD:MOVE,DIR:RIGHT>",
    "stop":  "<CMD:STOP>",
}

# ── SERIAL ──────────────────────────────────────────────────────
_ser = None

def serial_ac():
    global _ser
    try:
        _ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"[OK] ESP32: {SERIAL_PORT}")
    except serial.SerialException:
        _ser = None
        print("[UYARI] ESP32 bulunamadı")
    return _ser

def serial_yaz(cmd):
    global _ser
    if _ser is None:
        return
    try:
        _ser.write(cmd.encode())
    except (serial.SerialException, OSError):
        _ser = None

# ── SERVO ───────────────────────────────────────────────────────
pan_servo = None
tilt_servo = None
_current_pan = 90
_current_tilt = 90
_last_servo_check = 0.0
_latest_yaw = 0.0
_latest_pitch = 0.0

def snap_to_step(value, steps):
    return min(steps, key=lambda s: abs(s - value))

def servo_kur():
    global pan_servo, tilt_servo
    try:
        from gpiozero import AngularServo
        try:
            from gpiozero.pins.pigpio import PiGPIOFactory
            kwargs = {"pin_factory": PiGPIOFactory()}
        except Exception:
            kwargs = {}

        pan_servo = AngularServo(12, min_angle=0, max_angle=180,
            min_pulse_width=0.0005, max_pulse_width=0.0025, **kwargs)
        tilt_servo = AngularServo(13, min_angle=0, max_angle=180,
            min_pulse_width=0.0005, max_pulse_width=0.0025, **kwargs)
        pan_servo.angle = 90
        tilt_servo.angle = 90
        time.sleep(0.5)
        pan_servo.detach()
        tilt_servo.detach()
        print("[OK] Servolar başlatıldı")
    except Exception as e:
        print(f"[UYARI] Servo: {e}")

def servo_guncelle(yaw, pitch):
    global _latest_yaw, _latest_pitch, _last_servo_check
    global _current_pan, _current_tilt
    if pan_servo is None:
        return
    _latest_yaw = yaw
    _latest_pitch = pitch
    now = time.time()
    if now - _last_servo_check < SERVO_CHECK_INTERVAL:
        return
    _last_servo_check = now
    target_pan = snap_to_step(90 + _latest_yaw, PAN_STEPS)
    target_tilt = snap_to_step(90 + _latest_pitch, TILT_STEPS)
    if target_pan == _current_pan and target_tilt == _current_tilt:
        return
    try:
        if target_pan != _current_pan:
            pan_servo.angle = target_pan
            _current_pan = target_pan
        if target_tilt != _current_tilt:
            tilt_servo.angle = target_tilt
            _current_tilt = target_tilt
        time.sleep(0.4)
        pan_servo.detach()
        tilt_servo.detach()
    except Exception:
        pass

# ── KAMERA ──────────────────────────────────────────────────────
def kamera_ac():
    for idx in range(3):
        cam = cv2.VideoCapture(idx)
        if cam.isOpened():
            cam.set(cv2.CAP_PROP_FRAME_WIDTH, CAM_WIDTH)
            cam.set(cv2.CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT)
            cam.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
            print(f"[OK] Kamera: index={idx}")
            return cam
    print("[UYARI] Kamera bulunamadı")
    return None

def encode_frame(cam):
    if cam is None:
        return None
    ret, frame = cam.read()
    if not ret:
        return None
    bright = cv2.convertScaleAbs(frame, alpha=1.2, beta=10)
    ok, buf = cv2.imencode('.jpg', bright, [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])
    return buf.tobytes() if ok else None

# ── ANA BAĞLANTI ────────────────────────────────────────────────
async def run():
    serial_ac()
    servo_kur()
    cam = kamera_ac()

    while True:
        try:
            print(f"\n[RELAY] Bağlanıyor: {RELAY_URL}")
            async with websockets.connect(RELAY_URL, max_size=2**22) as ws:

                # Oda kodu al
                msg = await ws.recv()
                data = json.loads(msg)
                if data.get("type") == "room_code":
                    code = data["code"]
                    print(f"\n{'='*40}")
                    print(f"  ODA KODU: {code}")
                    print(f"  Quest'te bu kodu girin")
                    print(f"{'='*40}\n")

                # Quest bağlanmasını bekle
                print("[RELAY] Quest bekleniyor...")
                async for msg in ws:
                    if isinstance(msg, str):
                        data = json.loads(msg)

                        if data.get("type") == "connected":
                            print("[RELAY] Quest bağlandı ✅")
                            # Video + komut döngüsüne geç
                            await session_loop(ws, cam)
                            break

                        elif data.get("type") == "peer_disconnected":
                            print("[RELAY] Quest ayrıldı")
                            serial_yaz("<CMD:STOP>\n")

        except (websockets.exceptions.ConnectionClosed, ConnectionError, OSError) as e:
            print(f"[RELAY] Bağlantı koptu: {e}")
            serial_yaz("<CMD:STOP>\n")

        print("[RELAY] 3 saniye sonra yeniden bağlanılacak...")
        await asyncio.sleep(3)


async def session_loop(ws, cam):
    """Quest bağlıyken: video gönder + komut al"""
    last_cmd_time = time.time()
    frame_interval = 1.0 / VIDEO_FPS

    async def send_video():
        """Video frame'leri relay'e binary olarak gönder"""
        while True:
            jpeg = encode_frame(cam)
            if jpeg:
                try:
                    await ws.send(jpeg)
                except Exception:
                    return
            await asyncio.sleep(frame_interval)

    async def recv_commands():
        """Quest'ten gelen komutları işle"""
        nonlocal last_cmd_time
        async for msg in ws:
            if isinstance(msg, str):
                last_cmd_time = time.time()
                try:
                    data = json.loads(msg)

                    if data.get("type") == "peer_disconnected":
                        serial_yaz("<CMD:STOP>\n")
                        return

                    yön = data.get("dir", "stop").lower()
                    yaw = data.get("yaw", 0.0)
                    pitch = data.get("pitch", 0.0)

                    esp_cmd = DIR_MAP.get(yön, "<CMD:STOP>") + "\n"
                    serial_yaz(esp_cmd)
                    servo_guncelle(yaw, pitch)
                except json.JSONDecodeError:
                    pass

    async def watchdog():
        """Heartbeat watchdog"""
        nonlocal last_cmd_time
        while True:
            await asyncio.sleep(0.5)
            if time.time() - last_cmd_time > HEARTBEAT_TIMEOUT:
                serial_yaz("<CMD:STOP>\n")

    # Video + komut + watchdog paralel çalışsın
    tasks = [
        asyncio.create_task(send_video()),
        asyncio.create_task(recv_commands()),
        asyncio.create_task(watchdog()),
    ]
    done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
    for t in pending:
        t.cancel()


if __name__ == "__main__":
    try:
        asyncio.run(run())
    except KeyboardInterrupt:
        serial_yaz("<CMD:STOP>\n")
        print("\n[OK] Kapatıldı")
