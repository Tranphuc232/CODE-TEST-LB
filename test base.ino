#include <Leanbot.h>

void setup() {
  Serial.begin(115200);
}
void beep() {
  Leanbot.tone(440*4, 200);
  delay(200);
}

void loop() {
  if (LbTouch.onPress(TB1A)) {
    Serial.println("onPress TB1A");
        beep();
  }

  if (LbTouch.onPress(TB1B)) {
    Serial.println("onPress  TB1B");
        beep();
  }

  if (LbTouch.onPress(TB2A)) {
    Serial.println("onPress   TB2A");
        beep();
  }

  if (LbTouch.onPress(TB2B)) {
    Serial.println("onPress    TB2B");
        beep(); 
  }
}