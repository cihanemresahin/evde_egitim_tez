/*
 * ============================================================
 * Bu dosya ne yapar: Serial komut ayrıştırıcı header dosyası.
 * Neden var: RPi'den gelen serial komutları parse eder.
 *            M1'de test menüsü, M2'de RPi komutları için kullanılır.
 * Güvenlik notu: Geçersiz komutlar sessizce reddedilir ve loglanır.
 *                Komut formatı: <CMD:MOVE,DIR:FWD,SPD:50>
 * ============================================================
 */

#ifndef SERIAL_PARSER_H
#define SERIAL_PARSER_H

#include <Arduino.h>

// Komut tipleri
enum CommandType {
    CMD_NONE,       // Komut yok / geçersiz
    CMD_MOVE,       // Motor hareket komutu
    CMD_PAN,        // Pan servo komutu
    CMD_TILT,       // Tilt servo komutu
    CMD_HEARTBEAT,  // Heartbeat (watchdog besle)
    CMD_STOP,       // Durdur
    CMD_STATUS      // Durum sorgula
};

// Yön tipleri (motor komutu için)
enum DirectionType {
    DIRCMD_NONE,
    DIRCMD_FWD,    // İleri
    DIRCMD_BWD,    // Geri
    DIRCMD_LEFT,   // Sol
    DIRCMD_RIGHT,  // Sağ
    DIRCMD_STOP    // Dur
};

// Parse edilmiş komut yapısı
struct ParsedCommand {
    CommandType type;
    DirectionType direction;
    int value;          // Hız (0-100) veya servo açısı (0-180)
    bool valid;         // Komut geçerli mi
};

class SerialParser {
public:
    // Parser'ı başlat
    void begin();
    
    // Serial'den veri oku ve tamponla
    // Tam bir komut oluştuğunda true döner
    bool readSerial();
    
    // Son okunan komutu parse et ve döndür
    ParsedCommand getCommand();
    
    // Test komutu gönder (M1 test menüsü için)
    static void printTestMenu();

private:
    // Ham komutu parse et
    ParsedCommand _parseRawCommand(const String& raw);
    
    // Komut tamponu
    char _buffer[128];
    uint8_t _bufferIndex = 0;
    bool _commandReady = false;
    String _rawCommand = "";
};

#endif // SERIAL_PARSER_H
