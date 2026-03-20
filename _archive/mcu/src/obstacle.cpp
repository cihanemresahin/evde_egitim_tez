/*
 * ============================================================
 * Bu dosya ne yapar: HC-SR04 ultrasonik engel algılama implementasyonu.
 * Neden var: 30cm altında ileri hareketi donanımsal olarak kilitler.
 * Güvenlik notu:
 *   - Sensör timeout (30ms echo yok) = ileri hareket KİLİTLİ
 *   - 3 ölçüm ortalaması gürültüyü filtreler  
 *   - MEASURE_INTERVAL_MS: ölçümler arası minimum bekleme
 *
 * [HARDENING v2] Değişiklikler:
 *   #4: pulseIn timeout 38ms→30ms (daha hızlı fail-safe)
 *   #4: Sensör arızası = ileri hareket KALİCİ KİLİTLİ
 * ============================================================
 */

#include "obstacle.h"
#include "config.h"

// Sensör pinlerini ayarla
void ObstacleSensor::begin(uint8_t trigPin, uint8_t echoPin, float thresholdCm) {
    _trigPin = trigPin;
    _echoPin = echoPin;
    _thresholdCm = thresholdCm;
    
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    
    // Trigger'ı düşük seviyede başlat
    digitalWrite(_trigPin, LOW);
    
    Serial.print("[OBSTACLE] Baslatildi. Esik: ");
    Serial.print(_thresholdCm);
    Serial.println("cm");
}

// Tek bir mesafe ölçümü (cm)
float ObstacleSensor::_measureOnce() {
    // Trigger: 10µs HIGH pulse gönder
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);
    
    // [HARDENING #4] Echo süresini ölç (timeout: 30ms ≈ ~515cm)
    // 38ms yerine 30ms — daha hızlı fail-safe tepkisi
    // pulseIn bloklanma süresi kısaltıldı (loop dongusu daha sık döner)
    unsigned long duration = pulseIn(_echoPin, HIGH, 30000);
    
    if (duration == 0) {
        // [HARDENING #4] Timeout — sensör yanıt vermedi
        // Fail-safe: -1 döndür, update() bunu ileri kilit olarak değerlendirir
        return -1.0;
    }
    
    // Mesafe hesapla: ses hızı ≈ 343 m/s → 0.0343 cm/µs
    // Gidiş-dönüş olduğu için / 2
    float distanceCm = (duration * 0.0343) / 2.0;
    
    return distanceCm;
}

// Mesafe ölçümü güncelle (3 ölçüm ortalaması)
void ObstacleSensor::update() {
    unsigned long now = millis();
    
    // Çok sık ölçme — sensör karışabilir
    if (now - _lastMeasureTime < MEASURE_INTERVAL_MS) {
        return;
    }
    _lastMeasureTime = now;
    
    float total = 0;
    int validCount = 0;
    
    // OBSTACLE_SAMPLE_COUNT (3) kez ölç
    for (int i = 0; i < OBSTACLE_SAMPLE_COUNT; i++) {
        float d = _measureOnce();
        if (d > 0) {
            total += d;
            validCount++;
        }
        // Ölçümler arası kısa bekleme
        delayMicroseconds(500);
    }
    
    if (validCount == 0) {
        // Hiçbir ölçüm geçerli değil — FAIL-SAFE: ileri hareketi kilitle
        _sensorHealthy = false;
        _forwardLocked = true;
        _lastDistanceCm = 0;
        Serial.println("[OBSTACLE] !!! Sensor hatasi - ileri KILITLI (fail-safe)");
        return;
    }
    
    _sensorHealthy = true;
    _lastDistanceCm = total / validCount;
    
    // Engel mesafe kontrolü
    bool wasLocked = _forwardLocked;
    _forwardLocked = (_lastDistanceCm < _thresholdCm);
    
    // Durum değişikliğinde log bas
    if (_forwardLocked && !wasLocked) {
        Serial.print("[OBSTACLE] !!! ENGEL ALGILANDI: ");
        Serial.print(_lastDistanceCm);
        Serial.println("cm - ileri KILITLI");
    } else if (!_forwardLocked && wasLocked) {
        Serial.print("[OBSTACLE] Engel kaldirildi: ");
        Serial.print(_lastDistanceCm);
        Serial.println("cm - ileri SERBEST");
    }
}

// İleri hareket kilitli mi?
bool ObstacleSensor::isForwardLocked() {
    return _forwardLocked;
}

// Son mesafe değeri
float ObstacleSensor::getDistanceCm() {
    return _lastDistanceCm;
}

// Sensör sağlıklı mı?
bool ObstacleSensor::isSensorHealthy() {
    return _sensorHealthy;
}
