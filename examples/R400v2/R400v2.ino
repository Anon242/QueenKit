#include <R400v2.h>

#define DEVICE_ID 1
Board queen(DEVICE_ID);

//=================================================================================== 
//      Filename  : !FILENAMEFLAG
//      Author    : !AUTHORFLAG
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

void onMessage() {
  queen.out(queen.getBits(0, 4));
  queen.setBits(0, 3, queen.in());
}
