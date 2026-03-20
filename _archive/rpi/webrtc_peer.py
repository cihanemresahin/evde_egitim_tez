#!/usr/bin/env python3
"""
webrtc_peer.py — M8: RPi WebRTC Peer (aiortc)
Kamera görüntüsünü H.264 olarak gönderir.
Motor komutlarını DataChannel üzerinden alır.

Kurulum: pip3 install aiortc aiohttp aiohttp-cors --break-system-packages
Çalıştır: python3 ~/webrtc_peer.py
"""
import asyncio
import json
import logging
import serial

from aiohttp import web, ClientSession, WSMsgType
from aiortc import RTCPeerConnection, RTCSessionDescription
from aiortc.contrib.media import MediaPlayer

# --- Ayarlar ---
SIGNAL_URL = "ws://localhost:8766/ws?role=rpi"
SERIAL_PORT = "/dev/ttyUSB0"
SERIAL_BAUD = 115200
CAMERA_DEVICE = "/dev/video0"  # gerçek capture device (video1 = metadata)

logging.basicConfig(level=logging.INFO)
log = logging.getLogger("webrtc_peer")


# --- Serial (motor) ---
try:
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
    log.info(f"[SERIAL] ESP32 bağlandı: {SERIAL_PORT}")
except Exception as e:
    ser = None
    log.warning(f"[SERIAL] ESP32 bağlanamadı: {e}")


def send_motor_command(data_str):
    """Unity'den gelen JSON komutunu ESP32'ye ilet."""
    try:
        data = json.loads(data_str)
        direction = data.get("dir", "stop").upper()
        cmd = f"<CMD:{direction}>\n"
        if ser and ser.is_open:
            ser.write(cmd.encode())
            log.info(f"[MOTOR] {cmd.strip()}")
    except Exception as e:
        log.warning(f"[MOTOR] Hata: {e}")


# --- WebRTC Peer ---
pc = None

async def run():
    global pc
    pc = RTCPeerConnection()

    # Kamera: ffmpeg V4L2 üzerinden doğrudan oku (OpenCV yok)
    player = MediaPlayer(CAMERA_DEVICE, format="v4l2", options={
        "video_size": "640x480",
        "framerate": "30",
    })
    if player.video:
        pc.addTrack(player.video)
        log.info(f"[CAM] MediaPlayer kamera açıldı: {CAMERA_DEVICE}")
    else:
        log.error("[CAM] Kamera track alınamadı!")

    # DataChannel (motor komutları — Unity'den gelir)
    channel = pc.createDataChannel("motor")

    @channel.on("message")
    def on_message(message):
        send_motor_command(message)

    # Signaling
    async with ClientSession() as session:
        async with session.ws_connect(SIGNAL_URL) as ws:
            log.info("[SIG] Signaling'e bağlandı")

            # SDP Offer oluştur
            offer = await pc.createOffer()
            await pc.setLocalDescription(offer)

            await ws.send_str(json.dumps({
                "type": "offer",
                "sdp": pc.localDescription.sdp
            }))
            log.info("[SIG] Offer gönderildi — Unity bağlanmasını bekle")

            # Unity'den answer bekle
            async for msg in ws:
                if msg.type == WSMsgType.TEXT:
                    data = json.loads(msg.data)
                    log.info(f"[SIG] Alındı: {data['type']}")

                    if data["type"] == "answer":
                        answer = RTCSessionDescription(
                            sdp=data["sdp"], type="answer"
                        )
                        await pc.setRemoteDescription(answer)
                        log.info("[WEBRTC] Bağlantı kuruldu!")

                    elif data["type"] == "candidate":
                        try:
                            candidate_str = data.get("candidate", "")
                            if candidate_str:
                                await pc.addIceCandidate({
                                    "candidate": candidate_str,
                                    "sdpMid": data.get("sdpMid", "0"),
                                    "sdpMLineIndex": data.get("sdpMLineIndex", 0)
                                })
                        except Exception as e:
                            log.warning(f"[ICE] Candidate hatası (görmezden gelindi): {e}")

            await asyncio.sleep(3600)

    await pc.close()
    player.video and player.__del__


if __name__ == "__main__":
    print(f"[WEBRTC] RPi peer başlatılıyor...")
    print(f"[WEBRTC] Signaling: {SIGNAL_URL}")
    asyncio.run(run())
