/*
 * ============================================================
 * Bu dosya ne yapar: Watchdog (Ölü Adam Anahtarı) modülü.
 * Neden var: RPi'den 500ms boyunca sinyal gelmezse motorları
 *            otomatik durdurarak fiziksel kazaları önler.
 * Güvenlik notu: Bu modül güvenlik açısından KRİTİKTİR.
 *                Timeout değeri config.h'de tanımlıdır.
 * ============================================================
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

// Watchdog durumları
enum WatchdogState {
    WDG_OK,       // Sinyal geliyor, normal çalışma
    WDG_TIMEOUT   // Sinyal kesildi, motorlar durdurulmalı
};

class Watchdog {
public:
    // Watchdog'u başlat (timeout: ms cinsinden, varsayılan config.h'den)
    void begin(unsigned long timeoutMs);

    // Her geçerli komut veya heartbeat geldiğinde çağrılır
    // Watchdog zamanlayıcısını sıfırlar
    void feed();

    // Ana döngüde sürekli çağrılarak timeout kontrolü yapılır
    // Dönen değer: WDG_OK veya WDG_TIMEOUT
    WatchdogState check();

    // Watchdog tetiklendi mi? (500ms sinyal yok)
    bool isTriggered();

    // Son besleme zamanından bu yana geçen süre (ms)
    unsigned long timeSinceLastFeed();

private:
    unsigned long _timeoutMs = 500;    // Varsayılan timeout süresi
    unsigned long _lastFeedTime = 0;   // Son sinyal zamanı (millis)
    bool _triggered = false;           // Timeout tetiklendi mi
};

#endif // WATCHDOG_H
