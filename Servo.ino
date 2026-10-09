#include <Leanbot.h>

void setup() {
  Leanbot.begin(); //LbGripper.begin();
}

void loop() {

  if (LbTouch.read(TB2A)) {      // nếu chạm TB2A
    LbGripper.close(55);
    delay(1000);
  }

  if (LbTouch.read(TB2B)) {      // nếu chạm TB2A
    LbGripper.open();
    delay(2000);
  }

}