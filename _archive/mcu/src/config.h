/*
 * ============================================================
 * Bu dosya ne yapar: MCU yapılandırma sabitleri.
 * Neden var: Tüm donanım pin atamaları ve güvenlik parametreleri
 *            tek bir yerde tanımlı olsun diye.
 * Güvenlik notu: WATCHDOG_TIMEOUT_MS ve MAX_PWM değerleri
 *                güvenlik açısından kritiktir.
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// WATCHDOG (ÖLÜ ADAM ANAHTARI) AYARLARI
// ==========================================
// RPi'den bu süre boyunca sinyal gelmezse motorlar durur
#undef  WATCHDOG_TIMEOUT_MS
#define WATCHDOG_TIMEOUT_MS 3000

// ==========================================
// MOTOR AYARLARI
// ==========================================
// Maksimum PWM değeri (%100 güç izni → 255)
#undef  MAX_PWM
#define MAX_PWM 255

// Smooth ramp süresi: 0'dan MAX_PWM'e ulaşma süresi (ms)
#undef  RAMP_DURATION_MS
#define RAMP_DURATION_MS 1000

// [HARDENING #6] ESP32 PWM yapılandırması
// ESP32'de analogWrite yerine ledcWrite kullanılır
#define PWM_FREQ       1000   // PWM frekansı (Hz)
#define PWM_RESOLUTION 8      // PWM çözünürlüğü (bit) → 0-255 aralığı
#define LEDC_CHANNEL_A 0      // Sol motor LEDC kanal numarası
#define LEDC_CHANNEL_B 1      // Sağ motor LEDC kanal numarası

// Motor A pinleri (sol motor)
#define MOTOR_A_EN  5    // PWM enable (ENA)
#define MOTOR_A_IN1 18   // Yön pin 1
#define MOTOR_A_IN2 19   // Yön pin 2

// Motor B pinleri (sağ motor)
#define MOTOR_B_EN  4    // PWM enable (ENB)
#define MOTOR_B_IN1 16   // Yön pin 1
#define MOTOR_B_IN2 17   // Yön pin 2

// ==========================================
// SERVO AYARLARI (PAN/TILT) — RPi TARAFINDAN KONTROL EDİLİR
// NOT: Servolar ESP32'ye değil, RPi'nin PWM pinlerine veya
//      PCA9685 sürücüsüne bağlıdır. Bu değerler sadece referanstır.
// ==========================================
// #define SERVO_PAN_PIN   — RPi tarafında tanımlı
// #define SERVO_TILT_PIN  — RPi tarafında tanımlı
#define SERVO_PAN_MIN   0    // Minimum açı (derece) — referans
#define SERVO_PAN_MAX   180  // Maksimum açı (derece) — referans
#define SERVO_TILT_MIN  30   // Minimum açı (aşağı sınır) — referans
#define SERVO_TILT_MAX  150  // Maksimum açı (yukarı sınır) — referans

// ==========================================
// ENGEL ALGILAMA (HC-SR04)
// ==========================================
// Bu mesafenin altında ileri hareket kilitlenir (cm)
#ifndef OBSTACLE_DISTANCE_CM
#define OBSTACLE_DISTANCE_CM 30
#endif

#define ULTRASONIC_TRIG_PIN  25  // Trigger pin
#define ULTRASONIC_ECHO_PIN  26  // Echo pin
#define OBSTACLE_SAMPLE_COUNT 3  // Ortalama için ölçüm sayısı

// ==========================================
// LED AYARLARI
// ==========================================
#ifndef HEARTBEAT_LED_PIN
#define HEARTBEAT_LED_PIN 2  // Yerleşik LED (ESP32)
#endif

// ==========================================
// SERİAL İLETİŞİM
// ==========================================
#define SERIAL_BAUD_RATE 115200

// Komut başlangıç ve bitiş karakterleri
#define CMD_START_CHAR '<'
#define CMD_END_CHAR   '>'
#define CMD_SEPARATOR  ','
#define CMD_KEY_VALUE  ':'

// Maksimum komut uzunluğu (byte)
#define CMD_MAX_LENGTH 128

// [HARDENING #5] Serial flood koruması
// Minimum komut aralığı (ms) — maks 50Hz komut işleme
#define CMD_MIN_INTERVAL_MS 20

// ==========================================
// DÖNGÜ ZAMANLAMA
// ==========================================
// [HARDENING #3] CPU yükü kontrolü — loop() sonunda bekleme (ms)
#define LOOP_DELAY_MS 5

#endif // CONFIG_H
