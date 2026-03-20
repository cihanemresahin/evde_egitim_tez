/*
 * ============================================================
 * Bu dosya ne yapar: HC-SR04 ultrasonik engel algılama modülü.
 * Neden var: Robot 30cm'den yakın engelle karşılaştığında
 *            ileri hareketi donanımsal olarak kilitler.
 * Güvenlik notu: 
 *   - Sensör arızası (timeout) = ileri hareket KİLİTLİ (fail-safe)
 *   - 3 ölçüm ortalaması alınır (gürültü filtresi)
 *   - Sadece ileri hareket engellenir (geri + dönüş serbest)
 * ============================================================
 */

#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <Arduino.h>

class ObstacleSensor {
public:
    // Sensör pinlerini ayarla ve başlat
    void begin(uint8_t trigPin, uint8_t echoPin, float thresholdCm);
    
    // Mesafe ölçümü yap (3 ölçüm ortalaması)
    // Bu fonksiyonu ana döngüde düzenli olarak çağırın
    void update();
    
    // İleri hareket kilitli mi? (mesafe < threshold)
    bool isForwardLocked();
    
    // Son ölçülen mesafe (cm)
    float getDistanceCm();
    
    // Sensör sağlıklı mı? (timeout olmadı)
    bool isSensorHealthy();

private:
    // Tek bir mesafe ölçümü yap (cm)
    float _measureOnce();
    
    uint8_t _trigPin;
    uint8_t _echoPin;
    float _thresholdCm = 30.0;   // Varsayılan engel mesafesi
    float _lastDistanceCm = 0.0; // Son ölçülen mesafe
    bool _forwardLocked = false;  // İleri hareket kilitli mi
    bool _sensorHealthy = true;   // Sensör çalışıyor mu
    unsigned long _lastMeasureTime = 0;
    
    // Ölçüm aralığı (ms) — çok sık ölçüm sensörü karıştırabilir
    static const unsigned long MEASURE_INTERVAL_MS = 100;
};

#endif // OBSTACLE_H
