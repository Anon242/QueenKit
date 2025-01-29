#include <R408v2.h>

#define DEVICE_ID 1
Board queen(DEVICE_ID);

//===================================================================================
//      Filename  : R408v2
//      Author    : MeGum
//      Created   : 20.01.2025 16:19:30
//      Version   : 1.0
//      Notes     :
//                :
//===================================================================================

void setup() { queen.init(onMessage); }

void loop() { queen.loop(); }

void onMessage() {
  queen.out(queen.getBits(0, 8));
  queen.setBits(0, 8, queen.in());
}
