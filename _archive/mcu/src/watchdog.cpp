/*
 * ============================================================
 * Bu dosya ne yapar: Watchdog (Ölü Adam Anahtarı) implementasyonu.
 * Neden var: 500ms boyunca RPi'den sinyal gelmezse motorları
 *            güvenli şekilde durdurur.
 * Güvenlik notu: feed() her geçerli komut/heartbeat'de çağrılmalı.
 *                check() her loop iterasyonunda çağrılmalı.
 * ============================================================
 */

#include "watchdog.h"

// Watchdog'u verilen timeout süresiyle başlat
void Watchdog::begin(unsigned long timeoutMs) {
    _timeoutMs = timeoutMs;
    _lastFeedTime = millis();  // Başlangıçta zamanlayıcıyı sıfırla
    _triggered = false;
    Serial.print("[WATCHDOG] Baslatildi. Timeout: ");
    Serial.print(_timeoutMs);
    Serial.println("ms");
}

// Watchdog zamanlayıcısını sıfırla (sinyal geldi)
void Watchdog::feed() {
    _lastFeedTime = millis();
    
    // Eğer daha önce tetiklenmişse, yeni sinyal ile normale dön
    if (_triggered) {
        _triggered = false;
        Serial.println("[WATCHDOG] Sinyal geldi - normal moda donuldu");
    }
}

// Timeout kontrolü yap
WatchdogState Watchdog::check() {
    unsigned long elapsed = millis() - _lastFeedTime;
    
    if (elapsed >= _timeoutMs) {
        if (!_triggered) {
            // İlk kez tetikleniyor — log bas
            _triggered = true;
            Serial.print("[WATCHDOG] !!! TIMEOUT !!! ");
            Serial.print(elapsed);
            Serial.println("ms sinyal yok - MOTOR STOP");
        }
        return WDG_TIMEOUT;
    }
    
    return WDG_OK;
}

// Watchdog tetiklendi mi?
bool Watchdog::isTriggered() {
    return _triggered;
}

// Son sinyal zamanından bu yana geçen süre
unsigned long Watchdog::timeSinceLastFeed() {
    return millis() - _lastFeedTime;
}
