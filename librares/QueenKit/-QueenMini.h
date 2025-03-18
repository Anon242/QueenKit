#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом
// библиотеки Авто переключение приема предачи
#define SWITCH true
// Atmel2560 у нас разведен на Serial1, atmel328p на Serial !!!
#define QUEENSERIAL Serial2
////////////////////////////////////////////////////////////
#include "QueenKit.h"

class Board : public QueenKit {
public:
  Board(uint8_t id = 16) {
    // Назначем id
    QueenKit::id = 16;
  }

  // Функция задаем порты, включаем Serial для шины
  void init(void (*function)() = []() {}) {
    QueenKit::init();
    // Назначаем attached function
    QueenKit::atatchedF = *function;

    // Настраиваем порты
    setupPorts();
    setupADC();
    //  На 3 и 15 пине есть фейковое начальное срабатывание PWM, отключаем и
    //  включаем.
    disableFakePWM();

    /////////////////////////////////////////////!!!!!!!!!!!!!!!!!!!
    Serial1.begin(250000);
  }

  // Переопределение loop для считывания adcIndex
  void loop() {
    QueenKit::loop();
    if (Serial2.available() > 0)
      priemrpi(); /////////////////////////////////////////////!!!!!!!!!!!!!!!!!!!
    loopADC();
  }

  void priemrpi() {
    roundBufferRPI[headrpi()] = Serial2.read();
    if (roundBufferRPI[tailrpi] == 0x51) {
      if (roundBufferRPI[(tailrpi + 1) & ROUND_MASK] == 0x42) {
        if (roundBufferRPI[(tailrpi + 2) & ROUND_MASK] == 0x52) {
          uint8_t busID = = roundBufferRPI[(tailrpi + 5) & ROUND_MASK];
          // Если номер совпадает с ID нашего устройства то проверяем дальше
          if (busID == id) {
            if (roundBuffer[(tailrpi + 7) & ROUND_MASK] == crc8(crcBuff, 33)) {
              // Начинаем доить из RPI
              for (int i = 0; i < 32; i++)
                dataRPI[i] = roundBufferRPI[(tailrpi + 8 + i) & ROUND_MASK];
              //  Вкл/выкл ШИМ каналы
              for (int i = 3; i < 6; i++)
                pwmOuts(getBits(16 + 10 * (i - 3), 10), i + 1);
              // читаем показания аналоговых входов (между делом)
              readADC();
              setBits(0, 16, in());
            }
          }
        }
      }
    }
    tailrpi = ((tailrpi + 1) & ROUND_MASK); // сдвигаем хвост змеи вперед
  }
  uint8_t headrpi() {
    // прибавляем к хвосту условную длину змейки и накладывем маску от
    // переполнения. Получаем индекс головы
    return (tailrpi + SNAKE_LENGTHRPI) & ROUND_MASK;
  }
  // Функция чтения байтов на входе платы
  uint16_t in() {
    return ~(PINA | (((PINJ & 0x01) << 7) | ((PINJ & 0x02) << 5) |
                     ((PINJ & 0x04) << 3) | ((PINJ & 0x08) << 1) |
                     ((PINJ & 0x10) >> 1) | ((PINJ & 0x20) >> 3) |
                     ((PINJ & 0x40) >> 5) | ((PINJ & 0x80) >> 7)) >>
                        8);
  }

  void out(uint16_t x) // Функция вывода байта на выход платы
  {
    uint8_t iHByte = x >> 8;
    uint8_t hByte = ((iHByte & 0x01) << 7) | ((iHByte & 0x02) << 5) |
                    ((iHByte & 0x04) << 3) | ((iHByte & 0x08) << 1) |
                    ((iHByte & 0x10) >> 1) | ((iHByte & 0x20) >> 3) |
                    ((iHByte & 0x40) >> 5) | ((iHByte & 0x80) >> 7);

    // 1 - 8
    PORTC = ((x & 0x01) << 7) | ((x & 0x02) << 5) | ((x & 0x04) << 3) |
            ((x & 0x08) << 1) | ((x & 0x10) >> 1) | ((x & 0x20) >> 3) |
            ((x & 0x40) >> 5) | ((x & 0x80) >> 7);
    // 9 - 12
    PORTD = ((PORTD & 0b00001111) | (hByte & 0b11110000));
    // 13 - 16
    PORTL = (PORTL & 0b00111100) | ((hByte & 0b00000011) << 6) |
            ((hByte & 0b00001100) >> 2);
  }

  void turnLED(bool on) {
    if (on) {
      PORTG |= (1 << DDG1); // led
    } else {
      PORTG &= ~(1 << DDG1); // led
    }
  }

  uint16_t *inADC() { return adcBuffer; }

  void pwmOuts(uint8_t pwm, uint8_t out) {
    switch (out) {
    case 1:
      OCR5C = pwm; // 1
      break;
    case 2:
      OCR5B = pwm; // 2
      break;
    case 3:
      OCR5A = pwm; // 3
      break;
    case 4:
      OCR0A = pwm; // 4
      OCR0A ? TCCR0A |= (1 << COM0A1) : TCCR0A &= ~(1 << COM0A1);
      break;
    case 5:
      OCR1B = pwm; // 5
      break;
    case 6:
      OCR1A = pwm; // 6
      break;
    case 7:
      OCR2A = pwm; // 7
      OCR2A ? TCCR2A |= (1 << COM2A1) : TCCR2A &= ~(1 << COM2A1);
      break;
    case 8:
      OCR2B = pwm; // 8
      OCR2B ? TCCR2A |= (1 << COM2B1) : TCCR2A &= ~(1 << COM2B1);
      break;
    case 9:
      OCR4C = pwm; // 9
      break;
    case 10:
      OCR4B = pwm; // 10
      break;
    case 11:
      OCR4A = pwm; // 11
      break;
    case 12:
      OCR3C = pwm; // 12
      break;
    case 13:
      OCR3B = pwm; // 13
      break;
    case 14:
      OCR3A = pwm; // 13
      break;
    case 15:
      if (pwm > 127)
        PORTE |= 0x01;
      else
        PORTE &= ~0x01;
      break;
    case 16:
      OCR0B = pwm; // 15
      OCR0B ? TCCR0A |= (1 << COM0B1) : TCCR0A &= ~(1 << COM0B1);
      break;
    }
  }

private:
  ////////////// ADC //////////////
  int8_t adcIndex = 0;
  uint16_t adcBuffer[16];
  ////////////// ADC //////////////

  // Ports
  void setupPorts() {
    DDRK |= 0b00000000;

    /////////// Для ШИМ настройка портов на выход
    DDRL |= (1 << 5) | (1 << 4) | (1 << 3);
    PORTL &= ~((1 << 5) | (1 << 4) | (1 << 3));
    DDRH |= (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3);
    PORTH &= ~((1 << 6) | (1 << 5) | (1 << 4) | (1 << 3));

    ///////Настройка ШИМ/////////////////////
    DDRB |= (1 << PB7);
    OCR0A = 0;
    OCR0B = 0;
    TCCR1A |= (1 << WGM10) | (1 << COM1B1) | (1 << COM1A1);
    DDRB |= (1 << PB5) | (1 << PB6);
    OCR1A = 0;
    OCR1B = 0;
    TCCR2A |= (1 << WGM20) | (1 << COM2B1) | (1 << COM2A1);
    DDRH |= (1 << PH6);
    DDRB |= (1 << PB4);
    OCR2A = 0;
    OCR2B = 0;
    TCCR4A |= (1 << WGM40) | (1 << COM4B1) | (1 << COM4A1) | (1 << COM4C1);
    DDRH |= (1 << PH3) | (1 << PH4) | (1 << PH5);
    OCR4A = 0;
    OCR4B = 0;
    OCR4C = 0;
    TCCR5A |= (1 << WGM50) | (1 << COM5B1) | (1 << COM5A1) | (1 << COM5C1);
    DDRL |= (1 << PL3) | (1 << PL4) | (1 << PL5);
    OCR5A = 0;
    OCR5B = 0;
    OCR5C = 0;
    PORTE &= ~(1 << PE2);
  }

  ////////////// ADC //////////////
  void readingInBufferADC() {
    adcIndex = adcIndex < 0 ? 15 : adcIndex;
    // Записываем значение в массив
    // Читаем как бы снизу-вверх, а записываем значение сверху-вниз в буфер
    adcBuffer[abs(((adcIndex + 1) % 16) - 15)] = ADC;
    ADCSRB = (adcIndex & 0xF7) | (adcIndex & 0x08);
    ADMUX = (ADMUX & 0xE0) | (adcIndex & 0x07);
    ADCSRA |= (1 << ADSC); //  Запускаем АЦП
    adcIndex--;
  }

  // Настраиваем и включаем ADC
  void setupADC() {
    ADCSRA &= ~(1 << ADPS0); //  Частота дискретизации
    ADCSRA &= ~(1 << ADPS1); //  Частота дискретизации
    ADCSRA |= (1 << ADPS2);  //  Частота дискретизации

    ADMUX |= (1 << REFS0);  //  Vref напряжение питания МК
    ADMUX &= ~(1 << ADLAR); //  Правостороннее выравнивание (стандартно)
    ADCSRA |= (1 << ADEN);  // Включаем АЦП
  }

  void disableFakePWM() {
    // Отключаем ложное начальное срабатывание ШИМ 3
    OCR0A ? TCCR0A |= (1 << COM0A1) : TCCR0A &= ~(1 << COM0A1);
    // Отключаем ложное начальное срабатывание ШИМ 15
    OCR0B ? TCCR0A |= (1 << COM0B1) : TCCR0A &= ~(1 << COM0B1);
  }

  void loopADC() {
    // Входим только тогда, когда ADC посчитал один из пинов
    if (!(ADCSRA & (1 << ADSC)))
      readingInBufferADC();
  }
};
