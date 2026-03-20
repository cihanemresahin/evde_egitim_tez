# Akış Şemaları – Mermaid Kaynak Kodları (v2.1)

> Bu dosya tez raporunda kullanılacak akış şemalarını içerir.
> [Mermaid Live Editor](https://mermaid.live/) ile PNG/SVG'ye dönüştürülebilir.
>
> **v2.1 Değişiklikler:** WebRTC → GStreamer webrtcbin, Servo → RPi PWM/PCA9685

## 1. Ana Sistem Akışı

```mermaid
flowchart TB
    subgraph QUEST["🥽 Meta Quest 3 - Öğrenci Evi"]
        Q_UI["Unity VR Arayüzü"]
        Q_CTRL["ControlSender<br/>Joystick + Head-tracking"]
        Q_WEBRTC["WebRTCManager"]
        Q_VIDEO["VideoRenderer"]
        Q_DEBUG["DebugOverlay<br/>FPS / Latency"]
    end

    subgraph CLOUD["☁️ Bulut Sunucu - 443 Only"]
        SIG["Signaling Server<br/>WSS 443"]
        AUTH["JWT Auth<br/>Short-lived Token"]
        TURN["coturn TURN<br/>TLS 443 Relay"]
    end

    subgraph RPI["🍓 Raspberry Pi 5 - Okulda Robot"]
        CAM["UVC Kamera<br/>GStreamer H.264"]
        PUB["GStreamer webrtcbin<br/>WebRTC Publish"]
        DC_HANDLER["DataChannel Handler"]
        SERIAL_BR["Serial Bridge<br/>USB Serial"]
        LED["LED Indicator<br/>Fail-safe OFF"]
        SERVO["Pan/Tilt Servo<br/>RPi PWM / PCA9685"]
    end

    subgraph MCU["⚡ ESP32 - Sadece Tekerlek"]
        PARSER["Serial Parser"]
        MOTOR["Motor Controller<br/>Antigravity Ramp"]
        WDG["Watchdog Timer<br/>500ms Timeout"]
        OBS["Obstacle Sensor<br/>HC-SR04 30cm"]
    end

    CAM -->|H.264| PUB
    PUB -->|SDP/ICE| SIG
    SIG -->|SDP/ICE| Q_WEBRTC
    PUB ---|"Medya TURN TLS 443"| TURN
    TURN ---|"Medya TURN TLS 443"| Q_WEBRTC
    Q_WEBRTC --> Q_VIDEO
    Q_VIDEO --> Q_DEBUG

    Q_CTRL -->|DataChannel| Q_WEBRTC
    Q_WEBRTC -->|"Kontrol JSON TURN"| TURN
    TURN -->|"Kontrol JSON"| PUB
    PUB --> DC_HANDLER
    DC_HANDLER --> SERIAL_BR
    DC_HANDLER --> SERVO
    SERIAL_BR -->|USB Serial| PARSER
    PARSER --> MOTOR

    Q_WEBRTC -->|"JWT Token"| AUTH
    AUTH -->|"Dogrulama"| SIG
    AUTH -->|"TURN Creds"| TURN

    WDG -->|"500ms Timeout STOP"| MOTOR
    OBS -->|"30cm Ileri Kilit"| MOTOR
    PUB -->|"Baglanti Durumu"| LED

    style QUEST fill:#4a90d9,color:#fff
    style CLOUD fill:#f5a623,color:#fff
    style RPI fill:#7ed321,color:#fff
    style MCU fill:#d0021b,color:#fff
    style WDG fill:#ff6600,color:#fff,stroke:#ff0000,stroke-width:3px
    style SERVO fill:#27ae60,color:#fff
```

## 2. Watchdog (Ölü Adam Anahtarı) Detay

```mermaid
flowchart LR
    A["RPi Serial Bridge"] -->|"Komut / Heartbeat<br/>her 200ms"| B["MCU Serial Parser"]
    B --> C{"Watchdog Timer<br/>Sifirla"}
    C -->|"Sinyal geldi"| D["Motor Normal"]
    C -->|"500ms Sinyal Yok"| E["MOTOR STOP<br/>Tum PWM = 0"]
    E --> F["LED: Hata Modu"]
    E --> G["Yeni sinyal gelene<br/>kadar kilitli"]
    G -->|"Yeni heartbeat"| C

    style E fill:#ff0000,color:#fff,stroke-width:3px
    style C fill:#ff9900,color:#fff
```

## 3. Güvenlik Katmanları

```mermaid
flowchart TB
    subgraph AUTH_FLOW["Kimlik Dogrulama"]
        A1["Quest"] -->|"1. Login"| A2["Auth Server"]
        A2 -->|"2. JWT 5dk"| A1
        A1 -->|"3. Token WSS"| A3["Signaling"]
        A3 -->|"4. Dogrula"| A2
        A3 -->|"5. TURN Creds"| A1
    end

    subgraph LAYERS["Guvenlik Katmanlari"]
        S1["TLS 443"]
        S2["JWT 5dk Expiry"]
        S3["Rate Limit"]
        S4["Watchdog 500ms"]
        S5["Obstacle Override"]
        S6["LED Fail-safe"]
        S7["Kayit Yok KVKK"]
    end

    style AUTH_FLOW fill:#e8f4ff
    style LAYERS fill:#fff3e0
```

## 4. Kontrol Komutu Yaşam Döngüsü

```mermaid
sequenceDiagram
    participant Q as Quest 3
    participant C as Cloud
    participant R as RPi
    participant M as MCU

    Q->>C: JWT ile WSS baglanti
    C-->>Q: TURN credentials
    Q->>C: SDP Offer
    C->>R: SDP Offer ilet
    R-->>C: SDP Answer
    C-->>Q: SDP Answer ilet

    loop Canli Yayin
        R->>Q: H.264 video (GStreamer webrtcbin)
    end

    loop Kontrol (Motor)
        Q->>R: DataChannel JSON komutu
        R->>M: USB Serial motor komutu
        M-->>R: ACK
    end

    loop Kontrol (Servo)
        Q->>R: DataChannel head-tracking
        Note over R: RPi servo dogrudan kontrol
    end

    loop Heartbeat
        R->>M: HB sinyali (her 200ms)
        Note over M: Watchdog timer sifirla
    end

    Note over M: 500ms sinyal yok!
    M->>M: MOTOR STOP!
```
