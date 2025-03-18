#include <R404v5.h>

#define DEVICE_ID 1
Board queen(DEVICE_ID);

//===================================================================================
//      Filename  : R404v5
//      Author    : MeGum
//      Created   : 20.01.2025 16:17:35
//      Version   : 1.0
//      Notes     :
//                :
//===================================================================================

void setup() { queen.init(onMessage); }

void loop() { queen.loop(); }

void onMessage() {
  queen.out(queen.getBits(0, 7));
  queen.setBits(0, 5, queen.in());
}
