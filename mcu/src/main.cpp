/*
 * M2: ESP32 Komut Alıcı + Engel Koruyucu
 *
 * - RPi'den USB Serial üzerinden komut alır
 * - Komut formatı: <CMD:MOVE,DIR:FWD>\n
 * - 15cm engel varsa FWD komutunu reddeder
 * - Komut gelmezse DURUR (kumanda modeli — Seçenek B)
 * - 500ms komut gelmezse otomatik dur (güvenlik)
 */

#include <Arduino.h>

// ── PIN TANIMLARI ─────────────────────────────────────────────────
#define TRIG_PIN      25
#define ECHO_PIN      26

#define ENA_PIN        5
#define IN1_PIN       18
#define IN2_PIN       19

#define ENB_PIN        4
#define IN3_PIN       16
#define IN4_PIN       17

// ── AYARLAR ───────────────────────────────────────────────────────
#define ENGEL_CM      15      // Engel eşiği (cm)
#define MOTOR_HIZ    200      // Sabit hız 0-255
#define CMD_TIMEOUT  500      // ms — bu kadar komut gelmezse dur

// ── LEDC ──────────────────────────────────────────────────────────
#define LEDC_FREQ    1000
#define LEDC_BIT        8
#define LEDC_CH_A       0
#define LEDC_CH_B       1

// ── GLOBAL DURUM ─────────────────────────────────────────────────
String serialBuf = "";
unsigned long sonKomutZamani = 0;

// ── MOTOR FONKSİYONLARI ──────────────────────────────────────────
void motorDur() {
    ledcWrite(LEDC_CH_A, 0);
    ledcWrite(LEDC_CH_B, 0);
    digitalWrite(IN1_PIN, LOW); digitalWrite(IN2_PIN, LOW);
    digitalWrite(IN3_PIN, LOW); digitalWrite(IN4_PIN, LOW);
}

void motorIleri() {
    digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);
    digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);
    ledcWrite(LEDC_CH_A, MOTOR_HIZ);
    ledcWrite(LEDC_CH_B, MOTOR_HIZ);
}

void motorGeri() {
    digitalWrite(IN1_PIN, LOW); digitalWrite(IN2_PIN, HIGH);
    digitalWrite(IN3_PIN, LOW); digitalWrite(IN4_PIN, HIGH);
    ledcWrite(LEDC_CH_A, MOTOR_HIZ);
    ledcWrite(LEDC_CH_B, MOTOR_HIZ);
}

void motorSol() {
    // Sol motor yavaş, sağ motor hızlı
    digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);
    digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);
    ledcWrite(LEDC_CH_A, MOTOR_HIZ / 3);
    ledcWrite(LEDC_CH_B, MOTOR_HIZ);
}

void motorSag() {
    // Sol motor hızlı, sağ motor yavaş
    digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);
    digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);
    ledcWrite(LEDC_CH_A, MOTOR_HIZ);
    ledcWrite(LEDC_CH_B, MOTOR_HIZ / 3);
}

// ── SENSOR ────────────────────────────────────────────────────────
float mesafeOlc() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long sure = pulseIn(ECHO_PIN, HIGH, 30000);
    if (sure == 0) return 999.0;
    return sure * 0.0343 / 2.0;
}

// ── KOMUT İŞLEYİCİ ───────────────────────────────────────────────
void komutIsle(const String& komut) {
    sonKomutZamani = millis();

    float mesafe = mesafeOlc();

    if (komut.indexOf("DIR:FWD") >= 0) {
        if (mesafe <= ENGEL_CM) {
            Serial.print("[ENGEL] İleri reddedildi: ");
            Serial.print(mesafe); Serial.println("cm");
        } else {
            motorIleri();
            Serial.println("[MOTOR] İleri");
        }
    }
    else if (komut.indexOf("DIR:BWD") >= 0) {
        motorGeri();
        Serial.println("[MOTOR] Geri");
    }
    else if (komut.indexOf("DIR:LEFT") >= 0) {
        motorSol();
        Serial.println("[MOTOR] Sol");
    }
    else if (komut.indexOf("DIR:RIGHT") >= 0) {
        motorSag();
        Serial.println("[MOTOR] Sag");
    }
    else if (komut.indexOf("CMD:STOP") >= 0) {
        motorDur();
        Serial.println("[MOTOR] Dur");
    }
}

// ── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("=== M2: ESP32 Komut Alici ===");
    Serial.println("Format: <CMD:MOVE,DIR:FWD>");
    Serial.println("Yonler: FWD BWD LEFT RIGHT | STOP");

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(IN1_PIN, OUTPUT); pinMode(IN2_PIN, OUTPUT);
    pinMode(IN3_PIN, OUTPUT); pinMode(IN4_PIN, OUTPUT);

    ledcSetup(LEDC_CH_A, LEDC_FREQ, LEDC_BIT);
    ledcAttachPin(ENA_PIN, LEDC_CH_A);
    ledcSetup(LEDC_CH_B, LEDC_FREQ, LEDC_BIT);
    ledcAttachPin(ENB_PIN, LEDC_CH_B);

    motorDur();
    sonKomutZamani = millis();
    Serial.println("Hazir. Komut bekleniyor...");
}

// ── LOOP ──────────────────────────────────────────────────────────
void loop() {
    // Serial komut oku
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '>') {
            serialBuf += c;
            if (serialBuf.indexOf('<') >= 0 && serialBuf.indexOf('>') >= 0) {
                komutIsle(serialBuf);
                serialBuf = "";
            }
        } else {
            serialBuf += c;
            if (serialBuf.length() > 64) serialBuf = "";  // overflow koruması
        }
    }

    // Komut timeout — 500ms gelmediyse dur
    if (millis() - sonKomutZamani > CMD_TIMEOUT) {
        motorDur();
    }

    delay(10);
}
