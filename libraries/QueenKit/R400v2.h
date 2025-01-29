#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом библиотеки
// Авто переключение приема предачи
#define SWITCH false 
#include "QueenKit.h"

class Board : public QueenKit {
public:
  Board(uint8_t id) {
    // Назначем id
    QueenKit::id = id;
  }

  // Функция задаем порты, включаем Serial для шины
  void init(void (*function)() = [](){}) {
    QueenKit::init();
    // Назначаем attached function
    attachFunction(function);
    // Настраиваем порты
    setPorts();
  }

  uint8_t in() // Функция чтения байта на входе платы
  {
    uint8_t in_C = PINC;
    return ~(((in_C & 0x08) >> 3) | ((in_C & 0x04) >> 1) |
             ((in_C & 0x02) << 1) | ((in_C & 0x20) >> 2));
  }

  void out(uint8_t x) // Функция вывода байта на выход платы
  {
    PORTB = (PORTB & 0xF9) | ((x & 0x03) << 1);
    PORTD = (PORTD & 0x9F) | ((x & 0x0C) << 3);
    PORTC = (PORTC & 0xEF) | (x & 0x10); // pc4 out
  }

private:
  void setPorts() {
    DDRC = 0b01010000;
    DDRD |= 0b11110100;
    DDRB = 0b00100110;
    PORTC = 0b0100000;
  }
};
