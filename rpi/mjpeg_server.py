#!/usr/bin/env python3
"""
mjpeg_server.py — RPi USB Kamera HTTP Sunucu (Optimize Edilmiş)
Çalıştır: python3 ~/proje/mjpeg_server.py
Stream:   http://<IP>:8080/stream
Tek kare: http://<IP>:8080/frame.jpg
"""
from flask import Flask, Response
import cv2
import threading

app = Flask(__name__)

# ─── Kamera Ayarları ──────────────────────────────────────────────
FRAME_WIDTH  = 1280   # HD çözünürlük
FRAME_HEIGHT = 720
CAMERA_FPS   = 30
JPEG_QUALITY = 85     # 0-100, yüksek = daha iyi kalite
# ─────────────────────────────────────────────────────────────────

camera = None
frame_lock = threading.Lock()
latest_frame = None

def open_camera():
    for idx in [0, 1, 2]:
        cam = cv2.VideoCapture(idx)
        if cam.isOpened():
            # Kameraya native MJPEG isliyorsa doğrudan al (çok daha hızlı)
            cam.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
            cam.set(cv2.CAP_PROP_FRAME_WIDTH,  FRAME_WIDTH)
            cam.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
            cam.set(cv2.CAP_PROP_FPS, CAMERA_FPS)
            w = int(cam.get(cv2.CAP_PROP_FRAME_WIDTH))
            h = int(cam.get(cv2.CAP_PROP_FRAME_HEIGHT))
            fps = cam.get(cv2.CAP_PROP_FPS)
            print(f"[CAM] Kamera açıldı: index={idx}  {w}x{h}  {fps}fps")
            return cam
        cam.release()
    print("[CAM] HATA: Hiçbir kamera bulunamadı!")
    return None

def capture_loop():
    """Arka planda sürekli kare yakala — UI thread'ini bloklamaz"""
    global camera, latest_frame
    camera = open_camera()
    while True:
        if camera is None or not camera.isOpened():
            camera = open_camera()
            if camera is None:
                import time; time.sleep(2); continue
        ok, frame = camera.read()
        if ok:
            with frame_lock:
                latest_frame = frame
        else:
            camera.release()
            camera = None

# Kare yakalama iş parçacığı
threading.Thread(target=capture_loop, daemon=True).start()

def encode_jpeg(frame):
    # Sadece çok hafif kontrast ver, ekstra parlaklık ekleme
    bright = cv2.convertScaleAbs(frame, alpha=1.1, beta=0)
    ok, buf = cv2.imencode('.jpg', bright, [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])
    return buf.tobytes() if ok else None

def gen_frames():
    """MJPEG stream — tarayıcı için"""
    while True:
        with frame_lock:
            frame = latest_frame
        if frame is None:
            import time; time.sleep(0.033); continue
        data = encode_jpeg(frame)
        if data:
            yield b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + data + b'\r\n'

@app.route('/stream')
def stream():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/frame.jpg')
def frame_jpg():
    """Unity bu endpoint'i poll eder"""
    with frame_lock:
        frame = latest_frame
    if frame is None:
        return Response(status=503)
    data = encode_jpeg(frame)
    if data:
        return Response(data, mimetype='image/jpeg')
    return Response(status=503)

if __name__ == '__main__':
    import time
    time.sleep(0.5)  # capture_loop başlamasını bekle
    print(f"[CAM] Çözünürlük: {FRAME_WIDTH}x{FRAME_HEIGHT}  Kalite: {JPEG_QUALITY}")
    print(f"[CAM] Stream:    http://192.168.1.126:8080/stream")
    print(f"[CAM] Tek kare:  http://192.168.1.126:8080/frame.jpg")
    app.run(host='0.0.0.0', port=8080, threaded=True)
