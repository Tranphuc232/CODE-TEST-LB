/* 
The program selects the motor rotation direction and saves it to EEPROM memory
 
   Steps:
     1. Open and load the program
     2. Use touch button TB2A to change the direction of motor rotation
     3. Observe the effect: if Leanbot moves forward, it means it is in the correct direction of rotation
     4. Use TB2B touch button to stop Leanbot
     5. Touch TB1A and TB1B buttons simultaneously to save the current rotation direction into EEPROM memory
*/
 
#include <Leanbot.h>
 
 
/*------------------------------------------------------------------*/
void setup() {
  Leanbot.begin();
}
 
/*------------------------------------------------------------------*/
void loop() {
  static byte motionDirection = MOTION_DIRECTION_FORWARD;
 
  if (LbTouch.readBits() == TB2A) {
    beep(1);
 
    static bool loadConfig = false;
    if ( ! loadConfig ) {
      loadConfig = true;
      LbHwConfig.readMotionDirectionData(motionDirection);
    } else {
      motionDirection = (motionDirection == MOTION_DIRECTION_FORWARD) ? MOTION_DIRECTION_BACKWARD : MOTION_DIRECTION_FORWARD;
    }
 
    Serial.print("motionDirection: 0x");
    Serial.println(motionDirection, HEX);
    LbStepper_setConfig(motionDirection == MOTION_DIRECTION_BACKWARD);
 
    LbMotion.runLR(+400, +400);         // move forward
    while (LbTouch.readBits() != 0);    // wait until release
  }
 
 
  if (LbTouch.readBits() == TB2B) {
    beep(1);
    LbMotion.stopAndWait();
    while (LbTouch.readBits() != 0);    // wait until release
  }
 
 
  if (LbTouch.readBits() == TB1A + TB1B) {  // write to EEPROM by touching both TB1A and TB1B
    Serial.println(F("Start writting to EEPROM..."));
 
    LbMotion.stopAndWait();
 
    beep_long();
    LbHwConfig.writeMotionDirectionData(motionDirection);
  }
}
 
/*------------------------------------------------------------------*/
void beep(int repeats) {
  while (repeats--) {
    Leanbot.tone(440*4, 25);
    delay(50);
  }
}
 
void beep_long() {
  Leanbot.tone(130*1, 2000);
}
 
/*------------------------------------------------------------------*/