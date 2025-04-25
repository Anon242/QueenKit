#include <QueenUnisense3v1.h>

#define DEVICE_ID 1
Board queen(DEVICE_ID);

//=================================================================================== 
//      Filename  : !FILENAMEFLAG
//      Author    : MeGum
//      Github    : https://github.com/Anon242/QueenKit
//      Created   : !CREATEDFLAG
//      Version   : 1.0
//      Notes     : 
//                : 
//=================================================================================== 

void setup() 
{ 
  queen.init(onMessage); 
}

void loop() 
{ 
  queen.loop(); 
}

void onMessage() 
{  
  // Получаем буфер ADC
  uint16_t *adcBuffer = queen.inADC();
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t bitIndex = 16 + 10 * i;
    // Назначаем PWM из шины
    queen.pwmOuts(queen.getBits(bitIndex, 10), i + 1);
    // Отправляем данные ADC в шину
    queen.setBits(bitIndex, 10, adcBuffer[i]);
  }
  // Отправляем цифровые пины в шину
  queen.setBits(0, 16, queen.in());
}
