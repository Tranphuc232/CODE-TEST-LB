#include <Leanbot.h>
#include <anyrtttl.h>
 
String command;
byte lineValue;
const int V_MIN = 350;
const int V_MAX = 2000;
const int V_MEAN = 1000;
const int Threshold = 100;
const int displayTime = 500;
bool checkDemofromWeb = false;
bool checkGripperCalibration = false;
bool checkDoManualCalibration = false;
 
struct sLbGripperCalibration {
  int deg00L;
  int deg90L;
  int deg00R;
  int deg90R;
};
sLbGripperCalibration old;
sLbGripperCalibration newData;
int degL = 0;
int degR = 0;
byte oldLedMode;
byte oldStepperDir;
bool checkNextStep = true;
bool checkSaveToEEPROM = false;
 
void setup() {
  Leanbot.begin();
  LbIRLine.setThreshold(Threshold, Threshold, Threshold, Threshold);
}
 
void loop() {
  if (checkSaveToEEPROM) gripperCalibration_check();
  if (!checkDemofromWeb) lbTouch_check();
  serial_checkCommand();
  printAllSensors();
  LbDelay(100);
}
 
/*******************************************************************************
 Serial Command Handling
*******************************************************************************/
 
void serial_checkCommand(){
  if (Serial.available() <= 0) return;
  char cmd = Serial.read();
 
  switch (cmd) {
    case 'F':  Leanbot_setSpeed(+4, +4);         break;    // Forward
    case 'B':  Leanbot_setSpeed(-4, -4);         break;    // Backward
    case 'L':  Leanbot_setSpeed(-4, +4);         break;    // Left
    case 'R':  Leanbot_setSpeed(+4, -4);         break;    // Right
    case 'G':  Leanbot_setSpeed(+2, +4);         break;    // Forward Left
    case 'I':  Leanbot_setSpeed(+4, +2);         break;    // Forward Right
    case 'H':  Leanbot_setSpeed(-2, -4);         break;    // Back Left
    case 'J':  Leanbot_setSpeed(-4, -2);         break;    // Back Right
    case 'S':  Leanbot_setSpeed( 0,  0);         break;    // Stop 
    case 'q':  Leanbot_SetVelocity(10);          break;
    case '0' ... '9':
                Leanbot_SetVelocity(cmd - '0');  break;
    case 'W':  ledOn();                          break;
    case 'w':  ledOff();                         break;
    case 'U':  ;                                 break;    // back light on
    case 'u':  ;                                 break;    // back light off
    case 'V':  hornOn();                         break;
    case 'v':  hornOff();                        break;
    case 'X':  LbGripper.close();                break;
    case 'x':  LbGripper.open();                 break;
    case '.':  serial_handleCommandLine();       break;    // All Command Lines starts with '.'
    case ',':  serial_handleGripperCalibration();break;    // All Gripper Calibration Commands starts with ','
    default:                                     break;
  }
}
 
void serial_handleCommandLine(){
  command = Serial.readStringUntil('\n');
 
  if      (command == F("Motion"))              MotionDemo();
  else if (command.startsWith(F("Buzzer")))     BuzzerDemo();
  else if (command == F("RGBLeds"))             RGBLedsDemo();
  else if (command == F("Objectfollow"))        ObjectfollowDemo();
  else if (command == F("StraightMotion"))      StraightMotionDemo();
  else if (command.startsWith(F("Gripper")))    GripperDemo();
  else if (command.startsWith(F("LineFollow"))) LineFollowDemo();
  else if (command == F("IRLine"))              IRLineManualCalibration();
  else if (command == F("RemoteControl"))       checkDemofromWeb = true;
 
  Serial.println(command);                      // Report Finish of Demo action
  LbDelay(100);
}
 
void serial_handleGripperCalibration(){
  command = Serial.readStringUntil('\n');
  if (command == "GripperCalibration"){
   LbHwConfig.read_220822 (old.deg00L, old.deg90L, old.deg00R, old.deg90R, oldLedMode, oldStepperDir);
   PrintOldCalibData();
  }
  else if(command == "Step1"){
    checkNextStep = true;
    CalibrateOpenPosition();                
  }
  else if(command == "Step2"){
    if(checkNextStep){
    newData.deg00L = degL;
    newData.deg00R = degR;
    }
    CalibrateClosePosition();                
  }
  else if(command == "Step3"){
    checkNextStep = false;
    newData.deg90L = degL;
    newData.deg90R = degR;
    PrintCalibrationResult();
  }
  else if(command == "Step4"){
    WriteCalibrationResultToEEPROM();
  }
  else{
    if(command[0] == 'L'){
    String StrdegL = "";
    String StrdegR = "";
    size_t i = 3;
    while(command[i] != ' '){
      StrdegL += command[i];
      i++;
    }
    while(i<command.length()){
      StrdegR += command[i];
      i++;
    }
    degL = atoi(StrdegL.c_str());
    degR = atoi(StrdegR.c_str());
    
    LbGripper.writeLR(degL, degR);
    PrintCurrentDegree();
    }
  }
  beep(1);
}
 
/*******************************************************************************
 Sensor Printing Functions
*******************************************************************************/
 
void printAllSensors() {
  printTouch();
  Serial.print(F(" - "));
  printIR();
  Serial.print(F(" - "));
  printPing();
  Serial.print(F(" - "));
  printGripper();
  Serial.println();
}
 
void printTouch() {
  const eLbTouchButton touchPins[] = {TB1A, TB1B, TB2A, TB2B};
  Serial.print(F("TB "));
  for (int i = 0; i < 4; i++) {
    Serial.print(LbTouch.read(touchPins[i]) == HIGH ? F("1") : F("0"));
  }
}
 
void printIR(){
  Serial.print(F("IR "));                      
  LbIRLine.print(LbIRLine.read(100));
  Serial.print(F(" "));
 
  eLbIRSensor irValues[8] = {ir6L, ir4L, ir2L, ir0L, ir1R, ir3R, ir5R, ir7R};
  int RawValues[8];
  for (int i = 0; i < 8; i++) {
    RawValues[i] = map(LbIRArray.read(irValues[i]), 0, 768, 0, 255);
    Serial.print(RawValues[i]);
    if (i < 7) Serial.print(F(" ")); 
  }
}
 
void printPing(){
  int distance = Leanbot.pingCm(); 
  Serial.print(distance);           
  Serial.print(F(" cm"));
}
 
void printGripper(){
  Serial.print(F("GR "));
  int angleL = LbGripper.readL();
  int angleR = LbGripper.readR();
  Serial.print(angleL);
  Serial.print(F(" "));
  Serial.print(angleR);
}
 
/*******************************************************************************
 Demo Functions
*******************************************************************************/
 
void MotionDemo(){
  LbMotion.runLRrpm(30, 30);
  LbMotion.waitDistanceMm(100);
  LbMotion.runLRrpm(-30, -30);
  LbMotion.waitDistanceMm(100);
  LbMotion.runLRrpm(30, -30);
  LbMotion.waitRotationDeg(90);
  LbMotion.runLRrpm(-30, 30);
  LbMotion.waitRotationDeg(90);
  LbMotion.stopAndWait();
}
 
void BuzzerDemo(){
  const char* RTTTLnotes = command.substring(7).c_str();
  anyrtttl::blocking::play(LBPIN_BUZZER, RTTTLnotes );
}
 
void RGBLedsDemo(){
  RGBLedsShow(0x555555, BITMAP( ledO, ledA, ledF, ledE, ledD, ledC, ledB));
  RGBLedsShow(0xFF0000, BITMAP( ledO, ledA                              ));
  RGBLedsShow(0xFFFF00, BITMAP( ledO, ledA, ledF                        ));
  RGBLedsShow(0x00FF00, BITMAP( ledO, ledA, ledF, ledE                  ));
  RGBLedsShow(0x00FFFF, BITMAP(       ledA, ledF, ledE, ledD            ));
  RGBLedsShow(0x0000FF, BITMAP(       ledA, ledF, ledE, ledD, ledC      ));
  RGBLedsShow(0xFF00FF, BITMAP(       ledA, ledF, ledE, ledD, ledC, ledB));
  for (int i = 0; i < 4; i++) {
    RGBLedsShowRandom();
  }
}
 
void LineFollowDemo(){
  if ( ! checkDoManualCalibration) splitThreshold();
  do {
    runFollowLine();
    printAllSensors();
    LbDelay(50);
  } while ( lineValue != 0b0000 && lineValue != 0b1111 );
  LbMotion.stopAndWait();
}
 
void GripperDemo(){
  if (command.length() == 7){
    printAllSensors();
    LbGripper.close();
    printAllSensors();
    LbDelay(1000);
    LbGripper.open();
    printAllSensors();
    LbDelay(50);
  } 
  else GripperTest();
}
 
void ObjectfollowDemo(){
  int d = Leanbot.pingCm();
  const int limit = 20;
  const int offset = 2;
 
  while ((d != 0) && (d < 50)) {
    if (d > limit + offset)      LbMotion.runLR(V_MAX, V_MAX);  
    else if (d < limit - offset) LbMotion.runLR(-V_MAX, -V_MAX);
    printAllSensors();
    LbDelay(50);
    d = Leanbot.pingCm();
  }
 
  LbMotion.runLR(0, 0); 
}
 
void StraightMotionDemo(){
  LbMotion.runLR( +2000, +2000 );    
  LbMotion.waitDistanceMm( 1000 );      
  LbDelay(100);
  LbMotion.runLR( -2000, -2000 );       
  LbMotion.waitDistanceMm( -930 );      
  LbMotion.runLR(0, 0);
}
 
/*******************************************************************************
 Leanbot_Touch_Check
******************************************************************************/
 
void lbTouch_check() {
  if (LbTouch.onPress(TB1A)) {
    if (LbMotion.isMoving())      Leanbot_setSpeed(0, 0);
    else {
      delay(100);
      if (LbTouch.onPress(TB1B))  Leanbot_setSpeed(+2, +2);
      else                        Leanbot_setSpeed(+0, +2);
    }
    beepSingle();
    delay(50);
    return;
  }
 
  if (LbTouch.onPress(TB1B)) {
    if (LbMotion.isMoving())      Leanbot_setSpeed(0, 0);
    else {
      delay(100);
      if (LbTouch.onPress(TB1A))  Leanbot_setSpeed(+2, +2);
      else                        Leanbot_setSpeed(+2, +0);
    }
    beepSingle();
    delay(50);
    return;
  }
 
  if (LbTouch.read(TB2A) && LbTouch.read(TB2B)) {
    beepSingle();
    LbGripper_Toggle();
  }
}
 
/*******************************************************************************
 Leanbot_Motion
*******************************************************************************/
 
int velocity  = V_MAX / 1;
int motorLDir = 0;
int motorRDir = 0;
 
void Leanbot_run() {
  LbMotion.runLR(velocity * motorLDir / 4, velocity * motorRDir / 4);
}
 
void Leanbot_setSpeed(int speedL, int speedR) {
  motorLDir = speedL;
  motorRDir = speedR;
  Leanbot_run();
}
 
void Leanbot_SetVelocity(int speed) {
  velocity = map(speed, 0, 10, V_MIN, V_MAX);
  Leanbot_run();
}
 
/*******************************************************************************
 Leanbot_Beep
*******************************************************************************/
 
#define BEEP_FREQ   1500
 
void hornOn() {
  Leanbot.tone(BEEP_FREQ, 5000);      // play sound at 1500Hz for 5000ms
}
 
void hornOff() {
  Leanbot.noTone();
}
 
void beepSingle() {
  Leanbot.tone(BEEP_FREQ, 50);        // play sound at 1500Hz for 50ms
}
 
/*******************************************************************************
 Leanbot_RGB
*******************************************************************************/
 
void ledOn() {
  LbRGB.fillColor(CRGB::White);       // white setting for all lights
  LbRGB.show();                       // light status update
}
 
void ledOff() {
  LbRGB.fillColor(CRGB::Black);       // black setting for all lights
  LbRGB.show();                       // light status update
}
 
void RGBLedsShow(long color, byte shape) {
  LbRGB.clear();
  LbRGB.fillColor(color, shape);
  LbRGB.show();
  LbDelay(displayTime);
}
 
void RGBLedsShowRandom() {
  LbRGB[ledO] = CRGB(random8(), random8(), random8());
  LbRGB[ledA] = CRGB(random8(), random8(), random8());
  LbRGB[ledF] = CRGB(random8(), random8(), random8());
  LbRGB[ledE] = CRGB(random8(), random8(), random8());
  LbRGB[ledD] = CRGB(random8(), random8(), random8());
  LbRGB[ledC] = CRGB(random8(), random8(), random8());
  LbRGB[ledB] = CRGB(random8(), random8(), random8());
  LbRGB.show();
  LbDelay(displayTime);
}
 
/*******************************************************************************
 Leanbot_IRLine
*******************************************************************************/
 
int threshold(int value, int thresholdValue) {
  return (value > thresholdValue) ? 1 : 0;
}
 
void IRLineManualCalibration(){
  LbIRLine.doManualCalibration(TB1A);
  checkDoManualCalibration = true;
}
 
void splitThreshold(){
  int thresholds[4];
  for (int i = 0; i < 4; i++) {
  int startIndex = 11 + (i * 4);  // 11, 15, 19, 23
  thresholds[i] = map(command.substring(startIndex, startIndex + 3).toInt(), 0, 255, 0, 768);
  }
  LbIRLine.setThreshold(thresholds[0], thresholds[1], thresholds[2], thresholds[3]);
}
 
void runFollowLine() {
  for (int i = 0; i < 30; i++) {
    lineValue = LbIRLine.read();
    LbIRLine.displayOnRGB(CRGB::DarkCyan);
 
    switch (lineValue) {
      case 0b0100:
      case 0b1110:
        LbMotion.runLR(0, +V_MEAN);
        break;
      case 0b1100:
      case 0b1000:
        LbMotion.runLR(-V_MEAN, +V_MEAN);
        break;
      case 0b0010:
      case 0b0111:
        LbMotion.runLR(+V_MEAN, 0);
        break;
      case 0b0011:
      case 0b0001:
        LbMotion.runLR(+V_MEAN, -V_MEAN);
        break;
      default:
        LbMotion.runLR(+V_MEAN, +V_MEAN);
    }
 
    if (LbIRLine.isBlackDetected())
      break;
 
    LbMotion.waitDistanceMm(1);
  }
}
 
/*******************************************************************************
 Leanbot_Gripper
*******************************************************************************/
 
void LbGripper_Toggle() {
  static bool GripperIsClose = false;
 
  if (GripperIsClose) {
    LbGripper.open();
    GripperIsClose = false;
  } else {
    LbGripper.close();
    GripperIsClose = true;
  }
}
 
void GripperTest() {
  int angleL  = command.substring(10, 13).toInt();
  int angleR  = command.substring(14, 17).toInt();
  LbGripper.moveToLR(angleL, angleR, 500);
}
 
/*---------------------------------- Grippers Calibration--------------------------------*/
 
void gripperCalibration_check(){
  if(LbTouch.read(TB1A) && LbTouch.read(TB1B)){
    beep(3);
    Serial.println(F("TB1A + TB1B touched. Calibration settings saved. Calibration Done."));
    LbHwConfig.write_220822(newData.deg00L, newData.deg90L, newData.deg00R, newData.deg90R, 2, MOTION_DIRECTION_FORWARD);
  }
}
 
void CalibrateOpenPosition() {
  Serial.print(F("OpenPosition"));
  Serial.println();
  LbGripper.setCalibration(0, 90, 0, 90);
}
 
void CalibrateClosePosition() {
  Serial.print(F("ClosePosition"));
  Serial.println();
  LbGripper.setCalibration(0, 90, 0, 90);
}
 
void PrintCalibrationResult() {
  PrintNewCaliData();
  LbGripper.setCalibration(newData.deg00L, newData.deg90L, newData.deg00R, newData.deg90R);
  while(Serial.available()<=0){
    for(int i = 0; i <= 5; i++){
      LbGripper.moveToLR(i * 18, i * 18, 500);
    }
    delay(2000);
    for(int i = 5; i >= 0; i--){
      LbGripper.moveToLR(i * 18, i * 18, 500);
    }
    delay(2000);
  }
}
 
void PrintNewCaliData() {
  Serial.print(F("SetCalibration("));
  Serial.print(newData.deg00L);
  Serial.print(F(", "));
 
  Serial.print(newData.deg90L);
  Serial.print(F(", "));
 
  Serial.print(newData.deg00R);
  Serial.print(F(", "));
  
  Serial.print(newData.deg90R);
  Serial.print(F(")"));
  Serial.println();
}
 
void PrintOldCalibData() {
  Serial.print(F("GetCalibration("));
  Serial.print(old.deg00L);
  Serial.print(F(", "));
 
  Serial.print(old.deg90L);
  Serial.print(F(", "));
 
  Serial.print(old.deg00R);
  Serial.print(F(", "));
  
  Serial.print(old.deg90R);
  Serial.print(F(")"));
  Serial.println();
}
 
void PrintCurrentDegree() {
  Serial.print(F("degL = "));
  Serial.print(degL);
  Serial.print(F("\t"));
  Serial.print(F("degR = "));
  Serial.print(degR);
  Serial.println();
}
 
void WriteCalibrationResultToEEPROM() {
  Serial.println(F("Touch TB1A + TB1B to permanently save calibration settings"));
  checkSaveToEEPROM = true;
}
 
void beep(int repeats) {
  while (repeats--) {
    Leanbot.tone(440*4, 25);
    delay(50);
  }
}