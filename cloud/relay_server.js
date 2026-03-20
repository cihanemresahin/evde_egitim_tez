/*
 * relay_server.js — M10: WebSocket Relay (Cloud)
 *
 * RPi ve Quest farklı ağlarda. Bu sunucu ikisini eşleştirir.
 *
 * Akış:
 *   RPi  → ws://relay/ws?role=rpi    → oda kodu alır
 *   Quest → ws://relay/ws?room=XXXX&role=quest → odaya katılır
 *   Komutlar: Quest → relay → RPi
 *   Video:    RPi → relay → Quest (binary JPEG)
 *
 * Deploy: Render.com (ücretsiz Node.js)
 */

const { WebSocketServer } = require("ws");
const http = require("http");

const PORT = process.env.PORT || 3000;

// ── ODA YÖNETİMİ ─────────────────────────────────────────────
const rooms = new Map(); // roomCode → { rpi: ws, quest: ws }

function generateRoomCode() {
  let code;
  do {
    code = Math.floor(100000 + Math.random() * 900000).toString();
  } while (rooms.has(code));
  return code;
}

// ── HTTP SUNUCU (health check) ────────────────────────────────
const server = http.createServer((req, res) => {
  if (req.url === "/health") {
    res.writeHead(200, { "Content-Type": "application/json" });
    res.end(JSON.stringify({
      status: "ok",
      rooms: rooms.size,
      uptime: process.uptime()
    }));
  } else {
    res.writeHead(200, { "Content-Type": "text/plain" });
    res.end("VR Telepresence Relay Server");
  }
});

// ── WEBSOCKET SUNUCU ──────────────────────────────────────────
const wss = new WebSocketServer({ server });

wss.on("connection", (ws, req) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const role = url.searchParams.get("role");  // "rpi" veya "quest"
  const room = url.searchParams.get("room");  // Quest için oda kodu

  if (role === "rpi") {
    // RPi bağlanıyor → yeni oda oluştur
    const code = generateRoomCode();
    rooms.set(code, { rpi: ws, quest: null });
    ws._roomCode = code;
    ws._role = "rpi";

    // Oda kodunu RPi'ye gönder
    ws.send(JSON.stringify({ type: "room_code", code }));
    console.log(`[ROOM] ${code} oluşturuldu (RPi bağlandı)`);

  } else if (role === "quest" && room) {
    // Quest bağlanıyor → odaya katıl
    const roomData = rooms.get(room);
    if (!roomData) {
      ws.send(JSON.stringify({ type: "error", msg: "Oda bulunamadı" }));
      ws.close();
      return;
    }
    if (roomData.quest) {
      ws.send(JSON.stringify({ type: "error", msg: "Oda dolu" }));
      ws.close();
      return;
    }

    roomData.quest = ws;
    ws._roomCode = room;
    ws._role = "quest";

    // İki tarafa da bildir
    ws.send(JSON.stringify({ type: "connected", msg: "RPi ile eşleşildi" }));
    if (roomData.rpi && roomData.rpi.readyState === 1) {
      roomData.rpi.send(JSON.stringify({ type: "connected", msg: "Quest bağlandı" }));
    }
    console.log(`[ROOM] ${room} eşleşme tamamlandı`);

  } else {
    ws.send(JSON.stringify({ type: "error", msg: "role=rpi veya role=quest&room=XXXX gerekli" }));
    ws.close();
    return;
  }

  // ── MESAJ İLETME ──────────────────────────────────────────
  ws.on("message", (data, isBinary) => {
    const roomData = rooms.get(ws._roomCode);
    if (!roomData) return;

    // Karşı tarafa ilet
    const target = ws._role === "rpi" ? roomData.quest : roomData.rpi;
    if (target && target.readyState === 1) {
      target.send(data, { binary: isBinary });
    }
  });

  // ── BAĞLANTI KOPMA ───────────────────────────────────────
  ws.on("close", () => {
    const code = ws._roomCode;
    const roomData = rooms.get(code);
    if (!roomData) return;

    console.log(`[ROOM] ${code} — ${ws._role} ayrıldı`);

    // Karşı tarafa STOP bildir
    const other = ws._role === "rpi" ? roomData.quest : roomData.rpi;
    if (other && other.readyState === 1) {
      other.send(JSON.stringify({ type: "peer_disconnected" }));
    }

    // RPi ayrılırsa odayı kaldır
    if (ws._role === "rpi") {
      if (roomData.quest && roomData.quest.readyState === 1) {
        roomData.quest.close();
      }
      rooms.delete(code);
      console.log(`[ROOM] ${code} silindi`);
    } else {
      roomData.quest = null;
    }
  });
});

// ── BAŞLAT ──────────────────────────────────────────────────
server.listen(PORT, () => {
  console.log(`[RELAY] WebSocket relay: ws://0.0.0.0:${PORT}/ws`);
  console.log(`[RELAY] Health check:    http://0.0.0.0:${PORT}/health`);
});
