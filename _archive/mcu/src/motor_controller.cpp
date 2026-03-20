/*
 * ============================================================
 * Bu dosya ne yapar: Motor kontrol modülü implementasyonu.
 * Neden var: DC motorları "Antigravity" smooth ramp ile sürer.
 *            Ani hareket yerine kademeli hızlanma/yavaşlama sağlar.
 * Güvenlik notu: 
 *   - PWM hiçbir zaman MAX_PWM (153/%60) değerini aşamaz
 *   - emergencyStop() anında tüm PWM'i sıfırlar (ramp yok)
 *   - update() her loop iterasyonunda çağrılmalıdır
 *
 * [HARDENING v2] Değişiklikler:
 *   #6: ESP32'de ledcSetup/ledcWrite kullanımı (analogWrite yerine)
 *   #7: PWM constrain güvencesi, emergencyStop kesin sıfırlama
 *   #2: clearEmergencyStop() — heartbeat sonrası motor kurtarma
 * ============================================================
 */

#include "motor_controller.h"
#include "config.h"

// Motor pinlerini çıkış olarak ayarla
void MotorController::begin() {
    // Sol motor pinleri
    pinMode(MOTOR_A_IN1, OUTPUT);
    pinMode(MOTOR_A_IN2, OUTPUT);
    
    // Sağ motor pinleri
    pinMode(MOTOR_B_IN1, OUTPUT);
    pinMode(MOTOR_B_IN2, OUTPUT);
    
    // [HARDENING #6] ESP32 LEDC PWM kanallarını yapılandır
    // ESP32'de analogWrite() yerine ledcSetup + ledcAttachPin + ledcWrite kullanılır
    // Bu sayede PWM frekansı ve çözünürlüğü kesin kontrol altında olur
    #ifdef ESP32
        ledcSetup(LEDC_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(MOTOR_A_EN, LEDC_CHANNEL_A);
        
        ledcSetup(LEDC_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(MOTOR_B_EN, LEDC_CHANNEL_B);
    #else
        // Arduino için klasik analogWrite — pinMode yeterli
        pinMode(MOTOR_A_EN, OUTPUT);
        pinMode(MOTOR_B_EN, OUTPUT);
    #endif
    
    // Başlangıçta motorları durdur
    emergencyStop();
    
    _lastUpdateTime = millis();
    _emergencyStopped = false;
    
    Serial.print("[MOTOR] Baslatildi. Max PWM: ");
    Serial.print(MAX_PWM);
    Serial.print(" Ramp: ");
    Serial.print(RAMP_DURATION_MS);
    Serial.print("ms PWM_FREQ: ");
    Serial.print(PWM_FREQ);
    Serial.println("Hz");
}

// Hedef hız ve yön belirle
void MotorController::setTarget(MotorDirection direction, uint8_t speedPercent) {
    // Acil stop modundaysa komutu reddet
    if (_emergencyStopped) {
        Serial.println("[MOTOR] Acil stop aktif - komut reddedildi");
        return;
    }
    
    // [HARDENING #7] Hız yüzdesini PWM değerine çevir + MAX_PWM güvencesi
    // constrain iki kez: önce yüzde, sonra PWM — MAX_PWM asla aşılamaz
    int rawPercent = constrain(speedPercent, 0, 100);
    int pwmValue = map(rawPercent, 0, 100, 0, MAX_PWM);
    pwmValue = constrain(pwmValue, 0, MAX_PWM);  // Çift güvence
    
    switch (direction) {
        case DIR_FORWARD:
            _targetPwmLeft = pwmValue;
            _targetPwmRight = pwmValue;
            _leftForward = true;
            _rightForward = true;
            break;
            
        case DIR_BACKWARD:
            _targetPwmLeft = pwmValue;
            _targetPwmRight = pwmValue;
            _leftForward = false;
            _rightForward = false;
            break;
            
        case DIR_LEFT:
            // Sol motor yavaş, sağ motor hızlı → sola dönüş
            _targetPwmLeft = pwmValue / 3;
            _targetPwmRight = pwmValue;
            _leftForward = true;
            _rightForward = true;
            break;
            
        case DIR_RIGHT:
            // Sol motor hızlı, sağ motor yavaş → sağa dönüş
            _targetPwmLeft = pwmValue;
            _targetPwmRight = pwmValue / 3;
            _leftForward = true;
            _rightForward = true;
            break;
            
        case DIR_STOP:
            _targetPwmLeft = 0;
            _targetPwmRight = 0;
            break;
    }
    
    Serial.print("[MOTOR] Hedef: dir=");
    Serial.print(direction);
    Serial.print(" spd=");
    Serial.print(speedPercent);
    Serial.print("% pwm_L=");
    Serial.print(_targetPwmLeft);
    Serial.print(" pwm_R=");
    Serial.println(_targetPwmRight);
}

// Smooth ramp hesaplama (Antigravity ivmelenme)
int MotorController::_calculateRamp(int current, int target) {
    if (current == target) return current;
    
    unsigned long now = millis();
    unsigned long dt = now - _lastUpdateTime;
    
    // Ramp hızı: MAX_PWM'e RAMP_DURATION_MS'de ulaş
    // Her ms'de ne kadar değişim olacak
    float rampRate = (float)MAX_PWM / (float)RAMP_DURATION_MS;
    int maxChange = max(1, (int)(rampRate * dt));
    
    if (current < target) {
        current = min(current + maxChange, target);
    } else {
        current = max(current - maxChange, target);
    }
    
    // [HARDENING #7] Son güvence — MAX_PWM asla aşılamaz
    return constrain(current, 0, MAX_PWM);
}

// Ana döngüde çağrılarak PWM değerlerini güncelle
void MotorController::update() {
    if (_emergencyStopped) return;
    
    // Smooth ramp ile mevcut PWM'i hedefe yaklaştır
    _currentPwmLeft = _calculateRamp(_currentPwmLeft, _targetPwmLeft);
    _currentPwmRight = _calculateRamp(_currentPwmRight, _targetPwmRight);
    
    // Motorları sür
    _setMotor(MOTOR_A_EN, MOTOR_A_IN1, MOTOR_A_IN2, 
              _currentPwmLeft, _leftForward);
    _setMotor(MOTOR_B_EN, MOTOR_B_IN1, MOTOR_B_IN2, 
              _currentPwmRight, _rightForward);
    
    _lastUpdateTime = millis();
}

// Tek bir motoru PWM ile sür
void MotorController::_setMotor(uint8_t enPin, uint8_t in1Pin, uint8_t in2Pin,
                                 int pwmValue, bool forward) {
    // [HARDENING #7] Son güvence — bu fonksiyona gelen PWM de sınırlanır
    pwmValue = constrain(pwmValue, 0, MAX_PWM);
    
    if (forward) {
        digitalWrite(in1Pin, HIGH);
        digitalWrite(in2Pin, LOW);
    } else {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, HIGH);
    }
    
    // [HARDENING #6] ESP32'de ledcWrite, Arduino'da analogWrite
    #ifdef ESP32
        // LEDC kanal numarasını pin'den belirle
        uint8_t channel = (enPin == MOTOR_A_EN) ? LEDC_CHANNEL_A : LEDC_CHANNEL_B;
        ledcWrite(channel, pwmValue);
    #else
        analogWrite(enPin, pwmValue);
    #endif
}

// ACİL DURDURMA: Ramp yok, anında PWM = 0
void MotorController::emergencyStop() {
    _currentPwmLeft = 0;
    _currentPwmRight = 0;
    _targetPwmLeft = 0;
    _targetPwmRight = 0;
    
    // [HARDENING #7] Tüm motor pinlerini kesin sıfırla — hiçbir PWM çıkışı kalmamalı
    #ifdef ESP32
        ledcWrite(LEDC_CHANNEL_A, 0);
        ledcWrite(LEDC_CHANNEL_B, 0);
    #else
        analogWrite(MOTOR_A_EN, 0);
        analogWrite(MOTOR_B_EN, 0);
    #endif
    
    // Yön pinlerini de sıfırla (motor freni)
    digitalWrite(MOTOR_A_IN1, LOW);
    digitalWrite(MOTOR_A_IN2, LOW);
    digitalWrite(MOTOR_B_IN1, LOW);
    digitalWrite(MOTOR_B_IN2, LOW);
    
    _emergencyStopped = true;
    Serial.println("[MOTOR] !!! ACIL DURDURMA !!!");
}

// [HARDENING #2] Acil stop'u temizle — heartbeat sonrası motor kurtarma
// main.cpp'de systemLocked = false yapıldıktan sonra çağrılır
void MotorController::clearEmergencyStop() {
    _emergencyStopped = false;
    _currentPwmLeft = 0;
    _currentPwmRight = 0;
    _targetPwmLeft = 0;
    _targetPwmRight = 0;
    Serial.println("[MOTOR] Acil stop temizlendi - komut kabul basliyor");
}

// Motorlar durmuş mu?
bool MotorController::isStopped() {
    return (_currentPwmLeft == 0 && _currentPwmRight == 0);
}

// Mevcut PWM değerleri (debug)
int MotorController::getCurrentPwmLeft() { return _currentPwmLeft; }
int MotorController::getCurrentPwmRight() { return _currentPwmRight; }
