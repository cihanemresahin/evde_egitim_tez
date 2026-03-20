#!/usr/bin/env python3
"""
servo_test.py — SG90 Servo Test (RPi Hardware PWM)

GPIO 12 = Pan (Yaw)   — yatay
GPIO 13 = Tilt (Pitch) — dikey

Kullanım: python3 servo_test.py
"""
import time

try:
    from gpiozero import AngularServo
    from gpiozero.pins.pigpio import PiGPIOFactory

    # pigpiod donanımsal PWM sağlar — titreme olmaz
    factory = PiGPIOFactory()

    pan_servo = AngularServo(
        12, min_angle=-90, max_angle=90,
        min_pulse_width=0.0005, max_pulse_width=0.0025,
        pin_factory=factory
    )
    tilt_servo = AngularServo(
        13, min_angle=-90, max_angle=90,
        min_pulse_width=0.0005, max_pulse_width=0.0025,
        pin_factory=factory
    )
    print("[SERVO] pigpio factory ile başlatıldı ✅")

except Exception as e:
    print(f"[SERVO] pigpio kullanılamadı ({e}), software PWM deneniyor...")
    from gpiozero import AngularServo

    pan_servo = AngularServo(
        12, min_angle=-90, max_angle=90,
        min_pulse_width=0.0005, max_pulse_width=0.0025
    )
    tilt_servo = AngularServo(
        13, min_angle=-90, max_angle=90,
        min_pulse_width=0.0005, max_pulse_width=0.0025
    )
    print("[SERVO] Software PWM ile başlatıldı")

def test():
    print("\n[TEST] Ortaya getir...")
    pan_servo.angle = 0
    tilt_servo.angle = 0
    time.sleep(1)

    print("[TEST] Pan: Sol → Orta → Sağ")
    for a in [-45, 0, 45, 0]:
        pan_servo.angle = a
        print(f"  Pan = {a}°")
        time.sleep(0.8)

    print("[TEST] Tilt: Yukarı → Orta → Aşağı")
    for a in [-30, 0, 30, 0]:
        tilt_servo.angle = a
        print(f"  Tilt = {a}°")
        time.sleep(0.8)

    print("[TEST] Servo testi tamamlandı ✅")

if __name__ == "__main__":
    test()
