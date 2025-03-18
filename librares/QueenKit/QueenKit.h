
#pragma once
#include <Arduino.h>

#define ROUND_MASK 63
#define SNAKE_LENGTH 35

#if defined(__AVR_ATmega2560__)
#define QUEENSERIAL Serial1
#elif defined(__AVR_ATmega328P__)
#define QUEENSERIAL Serial
#else
#define QUEENSERIAL Serial
#endif

#ifndef SWITCH
#define SWITCH false
#endif

/**
 * @brief Класс для работы с нашей системой связи по шине
 *
 * Перед #include класса QueenKit необходимо создать #define QUEENSERIAL
 * [Serial || Serial1 || ...] и #define SWITCH [true || false]. В setup вызвать
 * QueenKit init(), в loop вызывать QueenKit loop().
 */
class QueenKit {
public:
  uint8_t id; // ID Платы

  /**
   * @brief Назначаем слушатель
   *
   * @param function функция на которую будем ссылатся
   */
  void attachFunction(void (*function)()) { atatchedF = function; }

  /**
   * @brief Запуск Serial
   *
   */
  void init() {
    QUEENSERIAL.begin(250000); // RS423
  }

  /**
   * @brief Метод для прослушивания Serial и дальнейшей обработки данных
   */
  void loop() {
    if (QUEENSERIAL.available()) {
      priem();
    }

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
  uint32_t getBits(uint32_t arrPos, uint32_t bits) {
    uint32_t result = 0;
    uint32_t byteIndex = arrPos / 8;
    uint32_t bitIndex = arrPos % 8;
    for (uint32_t i = 0; i < bits; i++) {
      if (bitIndex == 8) {
        byteIndex++;
        bitIndex = 0;
      }
      result |= ((inRPI[byteIndex] >> bitIndex) & 0x01) << i;
      bitIndex++;
    }
    return result;
  }

  /**
   * @brief Добавить данные в нагрузку шины
   *
   * @param bits Сколько битов
   * @param bytes Данные
   */
  void setBits(uint32_t arrPos, uint32_t bits, uint32_t bytes) {
    for (uint32_t i = 0; i < bits; i++) {
      uint32_t byteIndex = (arrPos + i) / 8;
      uint32_t bitIndex = (arrPos + i) % 8;
      dataBoard[byteIndex] = (dataBoard[byteIndex] & ~(1 << bitIndex)) |
                             (((bytes >> i) & 0x0001) << bitIndex);
    }
  }

private:
  /**
   * @brief Ссылка на метод который будет вызываться когда придут данные
   * @warning вызывать setBits() и getBits() только в методе на который
   * ссылаемся (onMessage)
   */
  void (*atatchedF)();

  /**
   * @brief Получить последний индекс буфера массива
   *
   * @return uint8_t
   */
  uint8_t head() { return (tail + SNAKE_LENGTH) & ROUND_MASK; }

  /**
   * @brief Получение нагрузки из шины
   *
   */
  void priem() {
    // Пишем в голову принятый байт
    uint8_t headIndex = head();
    roundBuffer[headIndex] = QUEENSERIAL.read();
    // если голова равна проценту и хвост равен звездочке то проверяем дальше
    if (roundBuffer[headIndex] == 0x025 && roundBuffer[tail] == 0x02A) {

      uint8_t busID = roundBuffer[(tail + 2) & ROUND_MASK];
      // если номер совпадает с ID нашего устройства то проверяем дальше
      if (busID == id) {
        crcBuff[0] = busID;
        uint8_t tailSum = tail + 3;

        for (int z = 0; z < 32; z++) {
          uint8_t roundMaskIndex = (tailSum + z) & ROUND_MASK;
          crcBuff[z + 1] = roundBuffer[roundMaskIndex];
          inRPI[z] = roundBuffer[roundMaskIndex];
        }

        // если контрольная сумма соответствет то действуем дальше
        if (roundBuffer[(tail + 1) & ROUND_MASK] == crc8(crcBuff, 33)) {
          // Вызываем ссылку на функцию которую мы указали при init()
          (*atatchedF)();
          // Уходим в функцию формирования выходного массива
          formation_out();
        }
      }
    }
    tail = ((tail + 1) & ROUND_MASK); // сдвигаем хвост змеи вперед
  }

  // Если на плате стоит автопереключение
#if SWITCH == true
  void transmit() { QUEENSERIAL.write(outRPI, 36); }
#else // Если на плате нет автопереключения, переключаем программно
  void transmit() {
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
  void formation_out() {
    // Сохраняем id и заполняем outRPI
    outRPI[2] = id;
    crcBuff[0] = id;
    // Заполняем outRPI и CRC
    for (int i = 0; i < 32; i++) {
      outRPI[i + 3] = dataBoard[i];
      crcBuff[i + 1] = dataBoard[i];
    }
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
  uint8_t crc8(const uint8_t *data, uint8_t length) {
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
