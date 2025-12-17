#include <R400v3.h>

#define DEVICE_ID 1
Board queen(DEVICE_ID);

//=================================================================================== 
//      Filename  : R400v3
//      Author    : MeGum
//      Github    : https://github.com/Anon242/QueenKit
//      Created   : 2025-12-17 12:38:12
//      Version   : 1.0
//      Notes     : 
//                : 
//=================================================================================== 
int msgCount = 0;
long ms, oldMS;
void setup() 
{
  queen.init(onMessage); 
}

void loop() 
{
   queen.loop(); 
}

void onMessage() {
  queen.setBits(0, 64, queen.getBits(0, 64));
  msgCount++;
  ms = millis();
  if (ms - oldMS > 1000){
    queen.setBits(200,16,msgCount);
    msgCount = 0;
    oldMS = ms;
  }
}
