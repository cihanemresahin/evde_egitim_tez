/*
 * ============================================================
 * Bu dosya ne yapar: Serial komut ayrıştırıcı implementasyonu.
 * Neden var: RPi'den (veya Serial Monitor'den) gelen komutları
 *            güvenli şekilde parse eder.
 * Güvenlik notu:
 *   - Buffer taşması koruması (CMD_MAX_LENGTH)
 *   - Geçersiz komutlar loglanır ve reddedilir
 *   - Komut formatı: <CMD:MOVE,DIR:FWD,SPD:50>
 * ============================================================
 */

#include "serial_parser.h"
#include "config.h"

// Parser'ı başlat
void SerialParser::begin() {
    _bufferIndex = 0;
    _commandReady = false;
    _rawCommand = "";
    Serial.println("[PARSER] Baslatildi. Format: <CMD:MOVE,DIR:FWD,SPD:50>");
}

// Serial'den veri oku ve tamponla
bool SerialParser::readSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == CMD_START_CHAR) {
            // Yeni komut başlangıcı — tamponu sıfırla
            _bufferIndex = 0;
            _commandReady = false;
        } else if (c == CMD_END_CHAR) {
            // Komut tamamlandı
            _buffer[_bufferIndex] = '\0';
            _rawCommand = String(_buffer);
            _commandReady = true;
            _bufferIndex = 0;
            return true;
        } else if (_bufferIndex < CMD_MAX_LENGTH - 1) {
            // Karakteri tampona ekle
            _buffer[_bufferIndex++] = c;
        } else {
            // Buffer taşması — sıfırla
            Serial.println("[PARSER] HATA: Buffer tasma - komut reddedildi");
            _bufferIndex = 0;
            _commandReady = false;
        }
    }
    return false;
}

// Son okunan komutu parse et
ParsedCommand SerialParser::getCommand() {
    ParsedCommand cmd;
    cmd.type = CMD_NONE;
    cmd.direction = DIRCMD_NONE;
    cmd.value = 0;
    cmd.valid = false;
    
    if (!_commandReady || _rawCommand.length() == 0) {
        return cmd;
    }
    
    _commandReady = false;
    cmd = _parseRawCommand(_rawCommand);
    
    if (cmd.valid) {
        Serial.print("[PARSER] Gecerli komut: ");
        Serial.println(_rawCommand);
    } else {
        Serial.print("[PARSER] Gecersiz komut: ");
        Serial.println(_rawCommand);
    }
    
    return cmd;
}

// Ham komutu parse et
// Format: CMD:MOVE,DIR:FWD,SPD:50
// veya:   CMD:HB (heartbeat)
// veya:   CMD:PAN,VAL:90
// veya:   CMD:TILT,VAL:45
// veya:   CMD:STOP
// veya:   CMD:STATUS
ParsedCommand SerialParser::_parseRawCommand(const String& raw) {
    ParsedCommand cmd;
    cmd.type = CMD_NONE;
    cmd.direction = DIRCMD_NONE;
    cmd.value = 0;
    cmd.valid = false;
    
    // CMD alanını bul
    int cmdIdx = raw.indexOf("CMD:");
    if (cmdIdx < 0) return cmd;
    
    // CMD değerini al (sonraki virgül veya string sonuna kadar)
    int cmdEnd = raw.indexOf(',', cmdIdx);
    String cmdValue;
    if (cmdEnd < 0) {
        cmdValue = raw.substring(cmdIdx + 4);
    } else {
        cmdValue = raw.substring(cmdIdx + 4, cmdEnd);
    }
    cmdValue.trim();
    
    // Komut tipini belirle
    if (cmdValue == "MOVE") {
        cmd.type = CMD_MOVE;
        
        // DIR alanını bul
        int dirIdx = raw.indexOf("DIR:");
        if (dirIdx >= 0) {
            int dirEnd = raw.indexOf(',', dirIdx);
            String dirValue;
            if (dirEnd < 0) {
                dirValue = raw.substring(dirIdx + 4);
            } else {
                dirValue = raw.substring(dirIdx + 4, dirEnd);
            }
            dirValue.trim();
            
            if (dirValue == "FWD") cmd.direction = DIRCMD_FWD;
            else if (dirValue == "BWD") cmd.direction = DIRCMD_BWD;
            else if (dirValue == "LEFT") cmd.direction = DIRCMD_LEFT;
            else if (dirValue == "RIGHT") cmd.direction = DIRCMD_RIGHT;
            else if (dirValue == "STOP") cmd.direction = DIRCMD_STOP;
            else return cmd; // Geçersiz yön
        }
        
        // SPD alanını bul
        int spdIdx = raw.indexOf("SPD:");
        if (spdIdx >= 0) {
            int spdEnd = raw.indexOf(',', spdIdx);
            String spdValue;
            if (spdEnd < 0) {
                spdValue = raw.substring(spdIdx + 4);
            } else {
                spdValue = raw.substring(spdIdx + 4, spdEnd);
            }
            spdValue.trim();
            cmd.value = constrain(spdValue.toInt(), 0, 100);
        }
        
        cmd.valid = (cmd.direction != DIRCMD_NONE);
        
    } else if (cmdValue == "HB") {
        cmd.type = CMD_HEARTBEAT;
        cmd.valid = true;
        
    } else if (cmdValue == "PAN") {
        cmd.type = CMD_PAN;
        int valIdx = raw.indexOf("VAL:");
        if (valIdx >= 0) {
            String valStr = raw.substring(valIdx + 4);
            valStr.trim();
            cmd.value = constrain(valStr.toInt(), SERVO_PAN_MIN, SERVO_PAN_MAX);
            cmd.valid = true;
        }
        
    } else if (cmdValue == "TILT") {
        cmd.type = CMD_TILT;
        int valIdx = raw.indexOf("VAL:");
        if (valIdx >= 0) {
            String valStr = raw.substring(valIdx + 4);
            valStr.trim();
            cmd.value = constrain(valStr.toInt(), SERVO_TILT_MIN, SERVO_TILT_MAX);
            cmd.valid = true;
        }
        
    } else if (cmdValue == "STOP") {
        cmd.type = CMD_STOP;
        cmd.valid = true;
        
    } else if (cmdValue == "STATUS") {
        cmd.type = CMD_STATUS;
        cmd.valid = true;
    }
    
    return cmd;
}

// Test menüsünü Serial Monitor'e yazdır
void SerialParser::printTestMenu() {
    Serial.println();
    Serial.println("========================================");
    Serial.println(" M1 - ESP32 DONANIM TEST MENUSU");
    Serial.println(" [HARDENED v2 - Production Ready]");
    Serial.println("========================================");
    Serial.println(" Motor komutlari:");
    Serial.println("   <CMD:MOVE,DIR:FWD,SPD:50>   Ileri %50");
    Serial.println("   <CMD:MOVE,DIR:BWD,SPD:30>   Geri %30");
    Serial.println("   <CMD:MOVE,DIR:LEFT,SPD:40>  Sola don");
    Serial.println("   <CMD:MOVE,DIR:RIGHT,SPD:40> Saga don");
    Serial.println("   <CMD:STOP>                  Durdur");
    Serial.println();
    Serial.println(" Sistem:");
    Serial.println("   <CMD:HB>           Heartbeat (watchdog)");
    Serial.println("   <CMD:STATUS>       Durum sorgula");
    Serial.println();
    Serial.println(" NOT: Servo (PAN/TILT) RPi tarafindan");
    Serial.println("      kontrol edilir, ESP32'de yok.");
    Serial.println("========================================");
    Serial.println(" WATCHDOG: 500ms HB gelmezse STOP+KILIT");
    Serial.println(" ENGEL: 30cm altinda ileri KILITLI");
    Serial.println(" PWM CAP: Max %60 (153/255)");
    Serial.println(" FLOOD: Maks 50Hz komut isleme");
    Serial.println(" KILIT: Timeout sonrasi sadece HB acar");
    Serial.println("========================================");
    Serial.println();
}
