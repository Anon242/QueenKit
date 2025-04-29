#pragma once
// Настройки конфигурации разведенной платы, обязательны перед вызовом библиотеки
// Авто переключение приема предачи
#define SWITCH true 
#include "QueenKit.h"
#define _QueenUnisense3v1

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
    setupADC();
    // на 3 и 15 пине есть фейковое начальное срабатывание PWM, отключаем и включаем
    disableFakePWM();
  }

// Переопределение loop для считывания adc 
  void loop(){
    QueenKit::loop();
    loopADC();
  }

  // Функция чтения байтов на входе платы
  inline uint16_t in() {
    
    return ~( (PINJ) | (((PINA & 0x01) << 7) | ((PINA & 0x02) << 5) |
             ((PINA & 0x04) << 3) | ((PINA & 0x08) << 1) |
             ((PINA & 0x10) >> 1) | ((PINA & 0x20) >> 3) |
             ((PINA & 0x40) >> 5) | ((PINA & 0x80) >> 7)) << 8);

  }

  inline uint16_t *inADC() { return adcBuffer; }

  void pwmOuts(uint8_t pwm, uint8_t out) {

    switch (out) {
    case 1:
      OCR5B = pwm; // 1
      break;
    case 2:
      OCR5A = pwm; // 2
      break;
    case 3:
      OCR0A = pwm; // 3
      OCR0A ? TCCR0A |= (1 << COM0A1) : TCCR0A &= ~(1 << COM0A1);
      break;
    case 4:
      OCR1B = pwm; // 4
      break;
    case 5:
      OCR1A = pwm; // 5
      break;
    case 6:
      OCR2A = pwm; // 6
      OCR2A ? TCCR2A |= (1 << COM2A1) : TCCR2A &= ~(1 << COM2A1);
      break;
    case 7:
      OCR2B = pwm; // 7
      OCR2B ? TCCR2A |= (1 << COM2B1) : TCCR2A &= ~(1 << COM2B1);
      break;
    case 8:
      OCR4C = pwm; // 8
      break;
    case 9:
      OCR4B = pwm; // 9
      break;
    case 10:
      OCR4A = pwm; // 10
      break;
    case 11:
      OCR5C = pwm; // 11
      break;
    case 12:
      OCR3C = pwm; // 12
      break;
    case 13:
      OCR3B = pwm; // 13
      break;
    case 14:
      OCR3A = pwm; // 14
      break;
    case 15:
      OCR0B = pwm; // 15
      OCR0B ? TCCR0A |= (1 << COM0B1) : TCCR0A &= ~(1 << COM0B1);
      break;
    case 16:
      if (pwm > 127)
        PORTE |= 0x01;
      else
        PORTE &= ~0x01;
      break;
    }
  }

private:
  
  ////////////// ADC //////////////
  int adc = 0;
  uint16_t adcBuffer[10];
  ////////////// ADC //////////////

  // Ports
  void setupPorts() {
    DDRA |= 0b00000000; //  Настройка цифровых входов на вход
    DDRJ |= 0b00000000;
    DDRF |= 0b00000000; //  Настройка аналоговых входов на вход
    DDRK |= 0b00000000;
    DDRC |= 0b11111111; //  Порты на выход на реле
    PORTC |= 0b00000000;
    DDRD |= (1 << 0) | (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
            (1 << 7); //  Порты на выход на реле
    PORTD &= ~((1 << 0) | (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
    DDRL |= (1 << 7) | (1 << 6);
    PORTL &= ~((1 << 7) | (1 << 6));
    /////////// Для ШИМ настройка портов на выход
    DDRL |= (1 << 5) | (1 << 4) | (1 << 3);
    PORTL &= ~((1 << 5) | (1 << 4) | (1 << 3));
    DDRB |= (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4);
    PORTB &= ~((1 << 7) | (1 << 6) | (1 << 5) | (1 << 4));
    DDRH |= (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3);
    PORTH &= ~((1 << 6) | (1 << 5) | (1 << 4) | (1 << 3));
    DDRE |= (1 << 5) | (1 << 4) | (1 << 3) | (1 << 0);
    PORTE &= ~((1 << 5) | (1 << 4) | (1 << 3) | (1 << 0));
    DDRG |= (1 << 5);
    PORTG &= ~(1 << 5);
    /////////////////////////////////////////
    ///////Настройка ШИМ/////////////////////
    DDRE |= (1 << PE2);

    TCCR0A |= (1 << WGM01) | (1 << WGM00) | (1 << COM0B1) | (1 << COM0A1);
    DDRG |= (1 << PG5);
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
    TCCR3A |= (1 << WGM30) | (1 << COM3B1) | (1 << COM3A1) | (1 << COM3C1);
    DDRE |= (1 << PE3) | (1 << PE4) | (1 << PE5) | (1 << PE0);
    OCR3A = 0;
    OCR3B = 0;
    OCR3C = 0;
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
    ///////////////////////////////////////////////
    DDRG |= (1 << 0);   //  ?
    PORTG &= ~(1 << 0); //  ?
    DDRF = 0b00000000;  //  ?
    PORTF = 0b00000000; //  ?
  }

  ////////////// ADC //////////////
  void readingInBufferADC() {
    adc = adc > 9 ? 0 : adc;
    adcBuffer[adc] = ADC; // Записываем значение в массив
    ADCSRB = (ADCSRB & 0xF7) | (adc & 0x08);
    ADMUX = (ADMUX & 0xE0) | (adc & 0x07);
    ADCSRA |= (1 << ADSC); //  Запускаем АЦП
    adc++;
  }
  /*
  
      adc = adc < 0 ? 15 : adc;
    adcBuffer[(adc+1) % 16] = ADC; // Записываем значение в массив
    ADCSRB = (ADCSRB & 0xF7) | (adc & 0x08);
    ADMUX = (ADMUX & 0xE0) | (adc & 0x07);
    ADCSRA |= (1 << ADSC); //  Запускаем АЦП
    adc--;
  
  */

  void setupADC() {
    // Настраиваем и включаем ADC
    ADCSRA &= ~(1 << ADPS0); //  Частота дискретизации
    ADCSRA &= ~(1 << ADPS1);
    ADCSRA |= (1 << ADPS2);               //  Частота дискретизации
    ADMUX |= (1 << REFS0);                 //  Vref напряжение питания МК
    ADMUX &= ~(1 << ADLAR); //  Правостороннее выравнивание (стандартно)
    ADCSRA |= (1 << ADEN);  // Включаем АЦП
  }

  void disableFakePWM(){
    // Отключаем ложное начальное срабатывание ШИМ 3
    OCR0A ? TCCR0A |= (1 << COM0A1): TCCR0A &= ~(1 << COM0A1); 
    // Отключаем ложное начальное срабатывание ШИМ 15
    OCR0B ? TCCR0A |= (1 << COM0B1): TCCR0A &= ~(1 << COM0B1); 
  }

    void loopADC() {
    // Входим только тогда, когда ADC посчитал один из пинов
    if (!(ADCSRA & (1 << ADSC)))
      readingInBufferADC();
  }
};
