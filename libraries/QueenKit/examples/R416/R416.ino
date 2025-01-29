#include <R416.h>

#define DEVICE_ID 1
Board queen(DEVICE_ID);

//===================================================================================
//      Filename  : R416
//      Author    : MeGum
//      Created   : 20.01.2025 16:22:01
//      Version   : 1.0
//      Notes     :
//                :
//===================================================================================

void setup() { queen.init(onMessage); }

void loop() { queen.loop(); }

void onMessage() {
  queen.out(queen.getBits(0, 16));
  queen.setBits(0, 16, queen.in());
}
