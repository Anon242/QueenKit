// Настройки конфигурации разведенной платы, обязательны перед вызовом библиотеки
// Авто переключение приема предачи
#define SWITCH true 
////////////////////////////////////////////////////////////
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
    setupPorts();
  }

  uint16_t in() // Функция чтения байта на входе платы
  {
    uint16_t ins = 0x0000;
    // DDRB = 0b00000000;//portB IN
    // DDRC = 0b00010000;//portC IN
    PORTD &= 0x7F; // OE enable
    asm("nop");
    asm("nop");
    ins |= (uint16_t)(((uint8_t)PINB & 0x0F) | (((uint8_t)PINC & 0x0F) << 4));
    asm("nop");
    asm("nop");
    PORTD |= 0x80; // OE disable
    asm("nop");
    asm("nop");
    PORTD &= 0xBF; // OE2 enable
    asm("nop");
    asm("nop");
    ins |= (uint16_t)(((uint8_t)PINB & 0x0F) | (((uint8_t)PINC & 0x0F) << 4))
           << 8;
    PORTD |= 0x40; // OE2 disable
    return ~ins;
  }

  void out(uint16_t x) // Функция вывода байта на выход платы
  {
    PORTD |= 0xC0;                                          // OE1,2 disable
    DDRB |= 0b00001111;                                     // portB out
    DDRC |= 0b00001111;                                     // portC out
    PORTB = (PORTB & 0xF0) | (uint8_t)(x & 0x000F);         // 1nd 4bit write
    PORTC = (PORTC & 0xF0) | (uint8_t)((x & 0x00F0) >> 4);  // 2nd 4bit write
    PORTD |= 0x10;                                          // CLK1 OUT ENABLE
    PORTD &= 0xEF;                                          // CLK1 OUT DISABLE
    PORTB = (PORTB & 0xF0) | (uint8_t)((x & 0x0F00) >> 8);  // 3nd 4bit write
    PORTC = (PORTC & 0xF0) | (uint8_t)((x & 0xF000) >> 12); // 4nd 4bit write
    PORTD |= 0x20;                                          // CLK2 OUT ENABLE
    PORTD &= 0xDF;                                          // CLK2 OUT DISABLE
    DDRB &= ~0b00001111;                                    // portB IN
    DDRC &= ~0b00001111;                                    // portC IN
  }

private:
  void setupPorts() {
    DDRC = 0b01110000;
    DDRD = 0b11110100;
    DDRB = 0b00000110;
    PORTC |= (1 << 5);

    out(0xFFFF);
    delay(100);
    out(0x0000);
  }
};
