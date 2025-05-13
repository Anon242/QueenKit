
#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>

#define ROUND_MASK 63
#define SNAKE_LENGTH 35

#if defined(__AVR_ATmega2560__)
#define QUEENSERIAL Serial1
#elif defined(__AVR_ATmega328P__)
#define QUEENSERIAL Serial
#else
#error Неизвестный микроконтроллер
#endif

#ifndef SWITCH
#define SWITCH false
#endif

/**
 * @brief Класс для работы с нашей системой связи по шине
 *
 * Перед #include класса QueenKit создать Queen
 * #define SWITCH [true || false]. В setup вызвать
 * QueenKit init(), в loop вызывать QueenKit loop().
 */
class QueenKit {
protected:
  /**
   * @brief Назначаем слушатель
   *
   * @param function функция на которую будем ссылатся
   */
  void attachFunction(void (*function)()) { atatchedF = function; }
public:
  uint8_t id; // ID Платы
  /**
   * @brief Запуск Serial
   *
   */
  void init() {
    delay(100);
    ledStartup(); // Мигаем лампочками
    QUEENSERIAL.begin(250000); // RS423
  }
  
  /**
   * @brief Метод для прослушивания Serial и дальнейшей обработки данных
   */
  void loop() {
    priem();
#if SWITCH == false
    PORTD &= ~(1 << 2);
#endif
  }

  /**
   * @brief Получить полезную нагрузку
   *
   *  Получает uint32_t значение битов данных из шины
   *
   * @param arrPos С какой позиции получать биты
   * @param bits Сколько битов
   * @return uint32_t
   */
  inline uint32_t getBits(uint32_t arrPos, uint32_t bits) {
    uint32_t byteIndex = arrPos >> 3;
    uint32_t bitIndex = arrPos & 7;

    uint64_t chunk;
    memcpy(&chunk, &inRPI[byteIndex], sizeof(uint64_t));

    return (chunk >> bitIndex) & ((1ULL << bits) - 1);
  }
  /**
   * @brief Добавить данные в нагрузку шины
   *
   * @param bits Сколько битов
   * @param bytes Данные
   */
  inline void setBits(uint32_t arrPos, uint32_t bits, uint32_t bytes) {
    uint32_t byteIndex = arrPos >> 3;
    uint32_t bitIndex = arrPos & 7;

    uint64_t chunk;
    memcpy(&chunk, &dataBoard[byteIndex], sizeof(uint64_t));

    const uint64_t mask = ((1ULL << bits) - 1) << bitIndex;
    chunk = (chunk & ~mask) | ((bytes << bitIndex) & mask);

    memcpy(&dataBoard[byteIndex], &chunk, sizeof(uint64_t));
  }

  void softReset() {
    asm volatile ("jmp 0");
  }

private:
  /**
   * @brief Ссылка на метод который будет вызываться когда придут данные
   * @warning вызывать setBits() и getBits() только в методе на который
   * ссылаемся (onMessage)
   */
  void (*atatchedF)();

  /**
   * @brief Запускаются мигания лампочек у плат, на которых это возможно на 2 сек
   * 
   */
  virtual void ledStartup() = 0;

  /**
   * @brief Запуск ошибки на сек 5 с миганием лампочек и затем горячаяя перезагрузка платы
   * 
   */
  virtual void ledError() = 0;
  


  /**
   * @brief Получить последний индекс буфера массива
   *
   * @return uint8_t
   */
  inline uint8_t head() { return (tail + SNAKE_LENGTH) & ROUND_MASK; }

  /**
   * @brief Получение нагрузки из шины
   * Почему priem? Так исторически сложилось
   */
  void priem() {
    // Данных нет - выходим
    if (!QUEENSERIAL.available())
      return;
    // Пишем в голову принятый байт
    uint8_t headIndex = head();
    roundBuffer[headIndex] = QUEENSERIAL.read();

    // Eсли голова равна проценту и хвост равен звездочке то проверяем дальше
    if (!((roundBuffer[headIndex] == 0x25) && (roundBuffer[tail] == 0x2A))) {
      tailShift();
      return;
    }

    // Если совпадает айди запроса с нашим айди
    const uint8_t busID = roundBuffer[(tail + 2) & ROUND_MASK];
    if (busID != id) {
      tailShift();
      return;
    }

    crcBuff[0] = busID;
    const uint8_t tailSum = tail + 3;

    // Высасываем полезные данные в отдельный массив
    for (int z = 0; z < 32; z++) {
      uint8_t roundMaskIndex = (tailSum + z) & ROUND_MASK;
      crcBuff[z + 1] = roundBuffer[roundMaskIndex];
      inRPI[z] = roundBuffer[roundMaskIndex];
    }

    // Если контрольная сумма соответствет то действуем дальше
    if (roundBuffer[(tail + 1) & ROUND_MASK] != crc8(crcBuff, 33)) {
      tailShift();
      return;
    }
    // Вызываем ссылку на функцию которую мы указали при init()
    (*atatchedF)();
    // Уходим в функцию формирования выходного массива
    formation_out();

    // Двигаем хвост змеи
    tailShift();
  }

  /**
   * @brief Двигаем хвост змеи на 1
   */
  inline void tailShift() { tail = ((tail + 1) & ROUND_MASK); }

  // Если на плате стоит автопереключение
#if SWITCH == true
  inline void transmit() { QUEENSERIAL.write(outRPI, 36); }
#else // Если на плате нет автопереключения, переключаем программно
inline void transmit() {
    delayMicroseconds(70);
    PORTD |= (1 << 2);
    QUEENSERIAL.write(outRPI, 36);
    delayMicroseconds(980);
    PORTD &= ~(1 << 2);
  }
#endif

  /**
   * @brief Формирование данных перед отправкой
   *
   */
  inline void formation_out() {
    // Сохраняем id и заполняем outRPI
    outRPI[2] = id;
    crcBuff[0] = id;
    // Заполняем outRPI и CRC
    memcpy(&outRPI[3], dataBoard, 32);
    memcpy(&crcBuff[1], dataBoard, 32);
    // Вычисляем контрольную сумму
    outRPI[1] = crc8(crcBuff, 33);
    outRPI[35] = 0x25;

    transmit();
  }

  /**
   * @brief Метод для получения контрольной суммы
   * @warning Использует табличный метод, необходима заранее высчитанная
   * таблица значений с полиномом 0x31. Начальный crc строго 0xFF.
   * @param data массив данных
   * @param length длинна массива
   * @return uint8_t контрольная сумма
   */
  inline uint8_t crc8(const uint8_t *data, uint8_t length) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < length; i++) {
      crc = crc8Table[crc ^ data[i]];
    }
    return crc;
  }

  /**
   * @brief Начальный индекс буфферного массива
   *
   */
  uint8_t tail = 0;
  uint8_t outRPI[36] = {0x2A, 0x01, 0x10, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x25};
  uint8_t dataBoard[32];
  uint8_t inRPI[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t roundBuffer[64];
  uint8_t crcBuff[33];

  /// Заранее вычесленные crc8 с полиномом 0x31 (значение crc8 должно
  /// начинатся с 0xFF)
  const uint8_t crc8Table[256] = {
      0x0,  0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea,
      0x7d, 0x4c, 0x1f, 0x2e, 0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4,
      0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0xf,  0x5c, 0x6d, 0x86, 0xb7, 0xe4, 0xd5,
      0x42, 0x73, 0x20, 0x11, 0x3f, 0xe,  0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8,
      0xc5, 0xf4, 0xa7, 0x96, 0x1,  0x30, 0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f,
      0xb8, 0x89, 0xda, 0xeb, 0x3d, 0xc,  0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa,
      0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13, 0x7e, 0x4f, 0x1c, 0x2d,
      0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6, 0xa5, 0x94, 0x3,  0x32, 0x61, 0x50,
      0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c, 0x2,  0x33, 0x60, 0x51,
      0xc6, 0xf7, 0xa4, 0x95, 0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0xd,  0x5e, 0x6f,
      0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6, 0x7a, 0x4b, 0x18, 0x29,
      0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2, 0xa1, 0x90, 0x7,  0x36, 0x65, 0x54,
      0x39, 0x8,  0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3,
      0x44, 0x75, 0x26, 0x17, 0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x9,  0x5a, 0x6b,
      0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2, 0xbf, 0x8e, 0xdd, 0xec,
      0x7b, 0x4a, 0x19, 0x28, 0x6,  0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91,
      0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad,
      0x3a, 0xb,  0x58, 0x69, 0x4,  0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93,
      0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a, 0xc1, 0xf0, 0xa3, 0x92,
      0x5,  0x34, 0x67, 0x56, 0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef,
      0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15, 0x3b, 0xa,  0x59, 0x68,
      0xff, 0xce, 0x9d, 0xac,
  };
};




class QueenPlayer{
    public:

    QueenPlayer(){
        #if defined _R400v2
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R400v3
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R404v4
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R404v5
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R408v2
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R408v3
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R416
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _QueenUnisense4
            // Serial 2
        #elif defined _QueenUnisense3v1
            playerSerial = new SoftwareSerial(40, 41);
        #else 
            #error Плата не поддерживается.
        #endif
    }

    void init(){
        playerBegin();
    }

    void play(uint8_t track, uint8_t volume){
        if (player_track != track) {
          player_track = track;
          if (player_track > 0)
            player(0x03, player_track);
          else if (player_track == 0)
            player(0x0E, 0x00);
        }
        if (player_volume != volume) {
          player_volume = volume;
          player(0x06, player_volume < 30 ? player_volume : 30);
        }
    }

    private:
    SoftwareSerial* playerSerial; 
    uint8_t player_track = 0;
    uint8_t player_volume = 0;

    
    void playerBegin(){
        #if defined _QueenUnisense4
            Serial2.begin(9600);
        #else
            playerSerial->begin(9600); // init player
        #endif
    }

    uint8_t player_buffer[10] = {
        0x7E, // [0] start byte, always 0x7E
        0xFF, // [1] version, always 0xFF
        0x06, // [2] length, always 0x06
        0x00, // [3]*command: 0x03 - track, 0x06 - volume, 0x0E - pause
        0x00, // [4] feedback, lways 0x00
        0x00, // [5] high byte, always 0x00
        0x00, // [6]*low byte (parameter) track or volume
        0xFE, // [7] ???
        0x00, // [8]*checksumm
        0xEF  // [9] end byte, always 0xEF
    };

    void player( uint8_t command, uint8_t value )
{
  player_buffer[ 3 ] = command;
  player_buffer[ 6 ] = value;
  uint8_t checksum = 0;
  for ( int i = 2; i < 8; i ++ ) checksum += player_buffer[i];
  player_buffer[8] = (uint8_t) ~checksum;
  
    #if defined _QueenUnisense4
        Serial2.write(player_buffer, 10);
    #else
        playerSerial->write( player_buffer, 10 );
    #endif
} 
};       
