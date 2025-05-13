#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом
// библиотеки Авто переключение приема предачи
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
  void init(void (*function)() = []() {}) {

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
  void ledError() {
    *pinRXIn.ddr |= (1 << pinRXIn.pin);

    for (int i = 0; i < 30; i++) {
      *pinRXIn.port ^= (1 << pinRXIn.pin);
      if (i % 6 == 0)
        delay(600);
      delay(100);
    }

    *pinRXIn.ddr &= ~(1 << pinRXIn.pin);
  }

private:
  void setPorts() {
    DDRC = 0b00000000;
    DDRD |= 0b01100000;
    DDRB |= 0b00000110;
  }

  struct RegisterLocation {
    volatile uint8_t *port;
    volatile uint8_t *ddr;
    uint8_t pin;
  };

#define REGISTERINSIZE 3
  const RegisterLocation RegistersIn[REGISTERINSIZE] = {
      {&PORTC, &DDRC, PC3},
      {&PORTC, &DDRC, PC2},
      {&PORTC, &DDRC, PC1},
  };

  const RegisterLocation pinRXIn = {&PORTD, &DDRD, PD0};

  void ledStartup() {
    for (uint8_t i = 0; i < REGISTERINSIZE; i++) {
      *RegistersIn[i].ddr |= (1 << RegistersIn[i].pin);
    }

    *pinRXIn.ddr |= (1 << pinRXIn.pin);
    *pinRXIn.port |= (1 << pinRXIn.pin);

    for (uint8_t z = 0; z < 7; z++) {
      for (uint8_t i = 0; i < REGISTERINSIZE; i++) {
        *RegistersIn[i].port ^= (1 << RegistersIn[i].pin);

        delay(180 / REGISTERINSIZE);
      }
      *pinRXIn.port ^= (1 << pinRXIn.pin);
    }

    *pinRXIn.ddr &= ~(1 << pinRXIn.pin);

    for (uint8_t i = 0; i < REGISTERINSIZE; i++) {
      *RegistersIn[i].ddr &= ~(1 << RegistersIn[i].pin);
    }
  }
};
