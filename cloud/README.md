# ☁️ Cloud Sunucu – Signaling, Auth ve TURN

> WebSocket signaling (WSS 443), JWT kimlik doğrulama,
> coturn TURN relay (TLS 443). FATİH ağı uyumlu.

## 📁 Alt Dizinler

| Dizin | Açıklama |
|-------|----------|
| `signaling/` | WebSocket signaling sunucusu (SDP/ICE değişimi) |
| `auth/` | JWT token oluşturma ve doğrulama |
| `turn/` | coturn TURN sunucu yapılandırması |

## 🌐 Ağ Mimarisi

```
Quest 3 ──WSS 443──→ Signaling ──WSS 443──→ RPi
Quest 3 ──TLS 443──→ TURN relay ──TLS 443──→ RPi
```

- **Tüm trafik TCP 443** (FATİH ağ kısıtı)
- UDP kullanılmaz, P2P kullanılmaz
- TURN relay zorunlu (coturn)

## 🛠️ Gereksinimler

- VPS (1 vCPU, 1GB RAM yeterli) veya okul sunucusu
- Node.js 20 LTS
- coturn
- Let's Encrypt TLS sertifikası (veya self-signed test için)
- Docker + Docker Compose (opsiyonel ama önerilen)

## 🚀 Kurulum (M1 modülünde detaylandırılacak)

```bash
# Docker ile tek komut
cd cloud
cp .env.example .env
nano .env  # Değerleri doldurun
docker-compose up -d
```

## 🔒 Güvenlik Notları

- JWT token 5 dakika ömürlü (short-lived)
- Rate limiting aktif (flood koruması)
- Medya kaydı yapılmaz (KVKK)
- Loglar minimum: sadece hata kodu + timestamp
- TURN credentials her oturum için dinamik üretilir
