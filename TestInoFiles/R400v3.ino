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

void setup() 
{
  queen.init(onMessage); 
}

void loop() 
{
   queen.loop(); 
}

void onMessage() {
  queen.out(queen.getBits(0, 4));
  queen.setBits(0, 3, queen.in());
}
