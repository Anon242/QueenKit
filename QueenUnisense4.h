#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом
// библиотеки Авто переключение приема предачи
#define SWITCH true
////////////////////////////////////////////////////////////
#define _QueenUnisense4
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
    setupPorts();
    setupADC();
    //  На 3 и 15 пине есть фейковое начальное срабатывание PWM, отключаем и
    //  включаем.
    disableFakePWM();
  }

  // Переопределение loop для считывания adcIndex
  void loop() {

    ms = millis();
    if (pinsStatusChanged()){
      PORTG |= (1 << DDG1); 
    }

    if(ms - oldMs >= 190){
      PORTG &= ~(1 << DDG1); 
      oldMs = ms;
    }

    QueenKit::loop();
    loopADC();
  }

  // Функция чтения байтов на входе платы
  inline uint32_t in() {

    uint8_t A = ((PINA & 0x01) << 7) | ((PINA & 0x02) << 5) |
                ((PINA & 0x04) << 3) | ((PINA & 0x08) << 1) |
                ((PINA & 0x10) >> 1) | ((PINA & 0x20) >> 3) |
                ((PINA & 0x40) >> 5) | ((PINA & 0x80) >> 7);
    uint8_t K = ((PINK & 0x01) << 7) | ((PINK & 0x02) << 5) |
                ((PINK & 0x04) << 3) | ((PINK & 0x08) << 1) |
                ((PINK & 0x10) >> 1) | ((PINK & 0x20) >> 3) |
                ((PINK & 0x40) >> 5) | ((PINK & 0x80) >> 7);
    uint8_t F = ((PINF & 0x01) << 7) | ((PINF & 0x02) << 5) |
                ((PINF & 0x04) << 3) | ((PINF & 0x08) << 1) |
                ((PINF & 0x10) >> 1) | ((PINF & 0x20) >> 3) |
                ((PINF & 0x40) >> 5) | ((PINF & 0x80) >> 7);

    return ~(((uint32_t)F << 24) | ((uint32_t)K << 16) | ((uint32_t)A << 8) |
             (uint32_t)PINJ);
  }

  inline void out(uint16_t x) // Функция вывода байта на выход платы
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
  ////////////// ADC //////////////
  int8_t adcIndex = 0;
  uint16_t adcBuffer[16];
  ////////////// ADC //////////////

  long oldMs;
  long ms;

  // Ports
  void setupPorts() {
    // Inputs
    DDRF = 0b00000000;
    DDRK = 0b00000000;
    DDRA = 0b00000000;
    DDRJ = 0b00000000;

    // Outputs
    // Digital
    DDRC = 0b11111111;
    PORTC = 0b00000000;
    DDRD |= (1 << DDD4) | (1 << DDD5) | (1 << DDD6) | (1 << DDD7);
    // Digital and PWM
    DDRL |= (1 << DDL0) | (1 << DDL1) | (1 << DDL3) | (1 << DDL4) |
            (1 << DDL5) | (1 << DDL6) | (1 << DDL7);
    // PWM
    DDRB |= (1 << DDB4) | (1 << DDB5) | (1 << DDB6) | (1 << DDB7);
    DDRH |= (1 << DDH3) | (1 << DDH4) | (1 << DDH5) | (1 << DDH6);
    DDRE |= (1 << DDE0) | (1 << DDE3) | (1 << DDE4) | (1 << DDE5);
    DDRG |= (1 << DDG5);
    // PWM
    TCCR0A |= (1 << WGM01) | (1 << WGM00) | (1 << COM0B1) | (1 << COM0A1);
    OCR0A = 0;
    OCR0B = 0;
    TCCR1A |= (1 << WGM10) | (1 << COM1B1) | (1 << COM1A1);
    OCR1A = 0;
    OCR1B = 0;
    TCCR2A |= (1 << WGM20) | (1 << COM2B1) | (1 << COM2A1);
    OCR2A = 0;
    OCR2B = 0;
    TCCR3A |= (1 << WGM30) | (1 << COM3B1) | (1 << COM3A1) | (1 << COM3C1);
    OCR3A = 0;
    OCR3B = 0;
    OCR3C = 0;
    TCCR4A |= (1 << WGM40) | (1 << COM4B1) | (1 << COM4A1) | (1 << COM4C1);
    OCR4A = 0;
    OCR4B = 0;
    OCR4C = 0;
    TCCR5A |= (1 << WGM50) | (1 << COM5B1) | (1 << COM5A1) | (1 << COM5C1);
    OCR5A = 0;
    OCR5B = 0;
    OCR5C = 0;
  }

  uint8_t oldPINF = 0;
  uint8_t oldPINK = 0;
  uint8_t oldPINA = 0;
  uint8_t oldPINJ = 0;

  bool pinsStatusChanged() {
    bool result = false;

    if (PINF != oldPINF || PINK != oldPINK || PINA != oldPINA ||
        PINJ != oldPINJ)
      result = true;

    oldPINF = PINF;
    oldPINK = PINK;
    oldPINA = PINA;
    oldPINJ = PINJ;

    return result;
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

  struct RegisterLocation {
    volatile uint8_t *port;
    volatile uint8_t *ddr;
    uint8_t pin;
  };

  const RegisterLocation pinRXIn = {&PORTD, &DDRD, PD2};

  void ledStartup() {
    *pinRXIn.ddr |= (1 << pinRXIn.pin);


    *pinRXIn.port |= (1 << pinRXIn.pin);

    for (uint8_t z = 0; z < 7; z++) {
      delay(36 * 7);
      *pinRXIn.port ^= (1 << pinRXIn.pin);

    }

    *pinRXIn.ddr &= ~(1 << pinRXIn.pin);

  }
};
