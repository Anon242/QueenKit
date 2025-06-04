#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом
// библиотеки Авто переключение приема предачи
#define SWITCH true
#define _R404v4
#include "QueenKit.h"

class Board : public QueenKit {
public:
  Board(uint8_t id) {
    // Назначем id
    QueenKit::id = id;
  }

  // Функция задаем порты, включаем Serial для шины
  void init(void (*function)() = []() {}) {
    QueenKit::init();
    // Назначаем attached function
    attachFunction(function);

    // Настраиваем порты
    setPorts();
  }

  inline uint8_t in() // Функция чтения байта на входе платы
  {
    uint8_t in_D = PIND;
    uint8_t in_B = PINB;
    uint8_t in_C = PINC;

    return ~(((in_D & 0x80) >> 7) | ((in_B & 0x20) >> 4) |
             ((in_B & 0x01) << 2) | ((in_B & 0x04) << 1) |
             ((in_B & 0x02) << 3) | ((in_C & 0x10) << 1) |
             ((in_C & 0x20) << 1));
  }

  inline void out(uint8_t x) // Функция вывода байта на выход платы НЕТ OUT
  {
    PORTD = (PORTD & 0x97) | ((x & 0x01) << 6) | ((x & 0x02) << 4) | (x & 0x08);
    // PORTD = (PORTD & 0x9F) | ((x&0x0C)<<3);
    PORTC = (PORTC & 0xF0) | ((x & 0x04) >> 2) | ((x & 0x10) >> 3) |
            ((x & 0x20) >> 3) | ((x & 0x40) >> 3); // pc4 out
  }

private:
  void setPorts() {
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
    DDRC &= ~((1 << PC4) | (1 << PC5));

    DDRD |= (1 << PD6) | (1 << PD5) | (1 << PD2) | (1 << PD3);
    DDRD &= ~((1 << PD7));

    DDRB = 0b00000000;
  }


};
