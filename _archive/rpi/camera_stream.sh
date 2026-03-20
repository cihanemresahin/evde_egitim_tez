#!/bin/bash
# camera_stream.sh — M5: RPi → GStreamer → UDP Stream
#
# Kullanım: ./camera_stream.sh <UNITY_IP>
# Örnek:    ./camera_stream.sh 192.168.1.106

UNITY_IP=${1:-"192.168.1.100"}
PORT=5600
WIDTH=640
HEIGHT=480
FPS=30

echo "=== M5 Kamera Streami ==="
echo "Hedef: $UNITY_IP:$PORT"
echo "Cozunurluk: ${WIDTH}x${HEIGHT} @ ${FPS}fps"
echo ""

if [ -e /dev/usbcam ]; then
    echo "[INFO] /dev/usbcam bulundu — USB webcam"
    gst-launch-1.0 -v \
        v4l2src device=/dev/usbcam ! \
        video/x-raw,width=$WIDTH,height=$HEIGHT,framerate=$FPS/1 ! \
        videoconvert ! \
        jpegenc quality=70 ! \
        rtpjpegpay ! \
        udpsink host=$UNITY_IP port=$PORT

elif gst-inspect-1.0 libcamerasrc &>/dev/null; then
    echo "[INFO] libcamerasrc bulundu — RPi CSI kamera"
    gst-launch-1.0 -v \
        libcamerasrc ! \
        video/x-raw,width=$WIDTH,height=$HEIGHT,framerate=$FPS/1 ! \
        videoconvert ! \
        jpegenc quality=70 ! \
        rtpjpegpay ! \
        udpsink host=$UNITY_IP port=$PORT

else
    echo "[HATA] Kamera bulunamadi!"
    echo "  USB: ls /dev/usbcam veya ls /dev/video*"
    exit 1
fi
