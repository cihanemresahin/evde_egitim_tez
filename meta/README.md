# Unity (Meta Quest 3) — VR İstemci

HTTP MJPEG video alımı, joystick + head tracking kontrol gönderimi.

## Dosyalar

| Dosya | Görev |
|-------|-------|
| `InputReader.cs` | Quest joystick + head tracking okuma |
| `ControlSender.cs` | WebSocket ile RPi'ye komut gönderme |
| `HttpVideoReceiver.cs` | HTTP MJPEG stream'i Quad'a render |

## Gereksinimler

- Unity 6 (LTS)
- Meta XR SDK
- Meta Quest 3
