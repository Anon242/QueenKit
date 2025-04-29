#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом библиотеки
// Авто переключение приема предачи
#define SWITCH true 
#define _R400v3
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

  inline uint8_t in() // Функция чтения байта на входе платы
  {
    return ~(((PINC & 0x08) >> 3) | ((PINC & 0x04) >> 1) |
             ((PINC & 0x02) << 1));
  }

  inline void out(uint8_t x) // Функция вывода байта на выход платы
  {
    PORTB = (PORTB & 0xF9) | ((x & 0x03) << 1);
    PORTD = (PORTD & 0x9F) | ((x & 0x0C) << 3);
  }

private:
  void setPorts() {
    DDRC = 0b00000000;
    DDRD |= 0b01100000;
    DDRB |= 0b00000110;
  }
};
