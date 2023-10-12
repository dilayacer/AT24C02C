# AT24C02C

Bu header dosyası, EEPROM (Elektrikle Silinebilir Programlanabilir Salt Okunur Bellek) işlemlerini gerçekleştirmek için kullanılan bir kütüphaneyi tanımlar. STM32G4 serisi mikrodenetleyicilerle uyumludur ve çeşitli EEPROM işlevlerini sağlar. Ayrıca, bit dizisi (bit array) üzerinde çalışan yardımcı işlevler içerir.

Fonksiyonlar
EEPROM İşlemleri

void eeprom_Write (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size): EEPROM'a veri yazma işlemini gerçekleştirir.

void eeprom_Read (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size): EEPROM'dan veri okuma işlemini gerçekleştirir.

void eeprom_Erase (uint16_t page): Belirtilen sayfayı silme işlemini gerçekleştirir.
void clear_all_eeprom(): EEPROM'u tamamen temizler.
Bit Dizisi (Bit Array) İşlemleri
void bitArray_uint32_to_uint8(uint32_t value, uint8_t *array, uint8_t index): 32 bitlik bir tamsayıyı 8 bitlik bir bit dizisine dönüştürür.
void bitArray_uint16_to_uint8(uint16_t value, uint8_t *array, uint8_t index): 16 bitlik bir tamsayıyı 8 bitlik bir bit dizisine dönüştürür.
void bitArray_float_to_uint8(float value, uint8_t *array, uint8_t index): float türündeki bir sayıyı 8 bitlik bir bit dizisine dönüştürür.
void writeCRC_MSB(uint16_t value, uint8_t *array, uint8_t index): 16 bitlik bir tamsayıyı 8 bitlik bir bit dizisine dönüştürür.
Bit Dizisi Okuma İşlemleri
uint32_t bitArray_uint8_to_uint32(uint8_t *array, uint8_t index): 8 bitlik bir bit dizisini 32 bitlik bir tamsayıya dönüştürür.
uint16_t bitArray_uint8_to_uint16 (uint8_t *array, uint8_t index): 8 bitlik bir bit dizisini 16 bitlik bir tamsayıya dönüştürür.
float bitArray_uint8_to_float(uint8_t *array, uint8_t index): 8 bitlik bir bit dizisini float türündeki bir sayıya dönüştürür.
İlk Ayar İşlemleri
uint32_t init_Read_Fill(): EEPROM'dan verileri okuyarak başlangıç ayarlarını yapar.
uint32_t write_Flash(): Ayarları Flash belleğe yazar.
Tek Değişken Okuma İşlemleri
uint32_t singleRead_uint(uint8_t a, uint8_t b, uint32_t *sonuc): Belirli bir uint32 değişkenini okur.
uint32_t singleRead_float(uint8_t a, uint8_t b, float *sonuc): Belirli bir float değişkenini okur.
Tek Değişken Yazma İşlemleri
void clearByte(uint16_t a, uint16_t b): Belirtilen baytları temizler.
uint8_t singleWrite_float(uint16_t a, uint16_t b, float data_f): Belirli bir float değişkenini yazar.
uint8_t singleWrite_uint(uint16_t a, uint16_t b, uint32_t data): Belirli bir uint32 değişkenini yazar.
