/*
 * ============================================================
 * Bu dosya ne yapar: Motor kontrol modülü header dosyası.
 * Neden var: DC motorları Antigravity smooth ramp ile sürer,
 *            PWM %60 güvenlik sınırı uygular.
 * Güvenlik notu: MAX_PWM ve RAMP_DURATION_MS config.h'de tanımlı.
 *                emergencyStop() çağrıldığında tüm PWM sıfırlanır.
 * ============================================================
 */

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

// Motor yön tanımları
enum MotorDirection {
    DIR_FORWARD,   // İleri
    DIR_BACKWARD,  // Geri
    DIR_LEFT,      // Sola dönüş (sol motor yavaş, sağ motor hızlı)
    DIR_RIGHT,     // Sağa dönüş (sol motor hızlı, sağ motor yavaş)
    DIR_STOP       // Dur
};

class MotorController {
public:
    // Motor pinlerini ayarla ve başlat
    void begin();
    
    // Hedef hız ve yön belirle (smooth ramp ile ulaşılır)
    // speed: 0-100 arası yüzde değer (PWM cap uygulanır)
    void setTarget(MotorDirection direction, uint8_t speedPercent);
    
    // Ana döngüde çağrılarak smooth ramp güncellenir
    // Bu fonksiyon her loop iterasyonunda çağrılmalı
    void update();
    
    // ACİL DURDURMA: Tüm motorları anında durdur (ramp yok)
    // Watchdog timeout veya obstacle tetiklendiğinde kullanılır
    void emergencyStop();
    
    // [HARDENING #2] Acil stop'u temizle — heartbeat sonrası
    // Motor tekrar komut kabul etmeye başlar
    void clearEmergencyStop();
    
    // Motorlar şu an durmuş mu?
    bool isStopped();
    
    // Mevcut PWM değerlerini debug için al
    int getCurrentPwmLeft();
    int getCurrentPwmRight();

private:
    // Tek bir motoru PWM ile sür
    void _setMotor(uint8_t enPin, uint8_t in1Pin, uint8_t in2Pin, 
                   int pwmValue, bool forward);
    
    // Smooth ramp hesaplama
    int _calculateRamp(int current, int target);
    
    // Mevcut ve hedef PWM değerleri
    int _currentPwmLeft = 0;
    int _currentPwmRight = 0;
    int _targetPwmLeft = 0;
    int _targetPwmRight = 0;
    
    // Yön bilgileri
    bool _leftForward = true;
    bool _rightForward = true;
    
    // Ramp zamanlama
    unsigned long _lastUpdateTime = 0;
    
    // Acil stop aktif mi
    bool _emergencyStopped = false;
};

#endif // MOTOR_CONTROLLER_H
