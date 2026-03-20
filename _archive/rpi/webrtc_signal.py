#!/usr/bin/env python3
"""
webrtc_signal.py — M8: WebRTC Signaling Sunucusu
WebSocket üzerinden SDP offer/answer ve ICE candidate değişimi.
RPi ve Unity arasında relay yapar.

Çalıştır: python3 ~/webrtc_signal.py
Port: 8766
"""
import asyncio
import json
import logging
from aiohttp import web
import aiohttp_cors

logging.basicConfig(level=logging.INFO)
log = logging.getLogger("signal")

# Bağlı peer'lar: {"rpi": ws, "unity": ws}
peers = {}
stored_offer = None  # Unity geç bağlanırsa offer'ı tekrar gönder

async def ws_handler(request):
    global stored_offer
    ws = web.WebSocketResponse()
    await ws.prepare(request)

    role = request.query.get("role", "unknown")
    peers[role] = ws
    log.info(f"[SIG] Bağlandı: {role}")

    # Unity geç bağlandıysa saklanan offer'ı gönder
    if role == "unity" and stored_offer:
        log.info("[SIG] Saklanan offer Unity'ye gönderiliyor")
        await ws.send_str(stored_offer)

    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                data = json.loads(msg.data)
                log.info(f"[SIG] {role} → {data.get('type','?')}")

                # Offer'ı sakla (Unity sonradan bağlanabilir)
                if role == "rpi" and data.get("type") == "offer":
                    stored_offer = msg.data
                    log.info("[SIG] Offer saklandı")

                # Diğer tarafa ilet
                other = "unity" if role == "rpi" else "rpi"
                if other in peers and not peers[other].closed:
                    await peers[other].send_str(msg.data)
    finally:
        peers.pop(role, None)
        log.info(f"[SIG] Ayrıldı: {role}")

    return ws

async def health(request):
    return web.Response(text="WebRTC Signaling OK")

app = web.Application()
app.router.add_get("/ws", ws_handler)
app.router.add_get("/", health)

cors = aiohttp_cors.setup(app, defaults={
    "*": aiohttp_cors.ResourceOptions(allow_credentials=True,
                                      expose_headers="*",
                                      allow_headers="*")
})

if __name__ == "__main__":
    print("[SIG] Signaling sunucu: ws://0.0.0.0:8766/ws?role=rpi")
    print("[SIG] Unity bağlantısı: ws://<RPi_IP>:8766/ws?role=unity")
    web.run_app(app, host="0.0.0.0", port=8766)
