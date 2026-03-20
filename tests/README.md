# 🧪 Güvenlik Test Kapısı (M9)

> Tüm modülleri birleştirmeden önce çalıştırılması zorunlu güvenlik testleri.

## 📋 Test Listesi

| Test | Dosya | Açıklama |
|------|-------|----------|
| JWT doğrulama | `test_auth.py` | Geçersiz/süresi dolmuş token reddedilmeli |
| Rate limit | `test_rate_limit.py` | Flood saldırısı engellemeli |
| Watchdog | `test_watchdog.py` | 500ms timeout sonrası motor durmalı |
| Fail-safe | `test_failsafe.py` | Bağlantı kopunca LED OFF + motor STOP |

## 🚀 Çalıştırma (M9 modülünde detaylandırılacak)

```bash
cd tests
python -m pytest -v
```

> [!CAUTION]
> M10 entegrasyon modülüne geçmeden önce bu testlerin **tamamı** başarılı olmalıdır.
