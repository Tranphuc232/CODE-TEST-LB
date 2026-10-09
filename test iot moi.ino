#include <Leanbot.h>          // use Leanbot library
#include "LeanbotIoT.h"       // Leanbot IoT Library
#include <Wire.h>             // Wire Library Reference: https://www.arduino.cc/en/Reference/Wire

#define  WIFI_SSID      F( "DTT-T1" )//#define  WIFI_SSID      F( "Nova-MW3" )//
#define  WIFI_PASSWORD  F( "68686868" )

/////////////////////////////////////////////
// Main Setup and Loop
/////////////////////////////////////////////
 
String serCommand; 
 
void setup() {
 
  Serial.begin(115200);
  LbTouch.read(TB1A);
  Serial.println(F("Test IoT Modules"));         
 
  // Initialize modules
  MAX30102_begin(Wire, I2C_CLK_400kHz); // Also Initialize Wire I2C
  OLED_begin();
  BME280_begin();
}
 
void loop() {
  LbIoT.Wifi.begin( WIFI_SSID, WIFI_PASSWORD );
  WiFi_setPassword();
  WiFi_printTime();
  while ( testLoop(250) ) {
    MAX30102_printIR();
    HC_SR501_printValue();
    SoilMoisture_printValue();
   // WiFi_printTime();
    BME280_printValues();
  }
 
  serCommand = Serial.readStringUntil( '\n' );
  if (checkCommand(F( "MAX30102"      )))  return MAX30102_testLoop();
  if (checkCommand(F( "OLED"          )))  return OLED_testLoop();
  //if (checkCommand(F( "WiFi"          )))  return WiFi_setPassword();
}
 
boolean checkCommand (const __FlashStringHelper* command) {
  return  serCommand.startsWith( command );
}
 
boolean testLoop(byte delayMs) {
  delay(delayMs);
  return  ( Serial.available() <= 0 );
}
 
void reportInit(const __FlashStringHelper* moduleName, boolean initOK) {
  SeriaX.printtb( moduleName, F( "Init" ));
  Serial.println( initOK ? F( "Ok" ) : F( "Error" ));        
}
 
 
/////////////////////////////////////////////
// MAX30102 Sensor
/////////////////////////////////////////////
 
#include "MAX30105.h"         // https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library
#include "heartRate.h"
 
MAX30105 max30102;
#define FINGER_THRESHOLD 50000
 
void MAX30102_begin(TwoWire &wirePort, uint32_t i2cSpeed) {
  boolean initOK = max30102.begin(wirePort, i2cSpeed);
  reportInit(F( "MAX30102" ), initOK );
  if ( ! initOK )  return;
 
  max30102.setup();
}
 
void MAX30102_testLoop() {
  while ( testLoop(200) ) {
    MAX30102_printIR();
  }
}
 
// void MAX30102_printIR() {
//   max30102.setPulseAmplitudeIR(0x1F);
//   long irValue = max30102.getIR();
 
//   if (irValue > FINGER_THRESHOLD) {
//     MAX30102_fingerLoop();
//   }
 
//   max30102.setPulseAmplitudeIR(0x00);
//   Serial.println(F("MAX30102 No Finger"));
// }
 
void MAX30102_printIR() {
  max30102.setPulseAmplitudeIR(0x1F); // on ir

  long timeout_target = millis() + 50;
  long irValue = 0;

  while(((long)millis()) - timeout_target < 0) {
    irValue = max30102.getIR();

    if (irValue > FINGER_THRESHOLD){
      MAX30102_fingerLoop();
      break;
    }
  }
 
  max30102.setPulseAmplitudeIR(0x00); // off ir
  Serial.println(F("MAX30102 No Finger"));
}

void MAX30102_fingerLoop() {
  unsigned int heartbeatCounter = 0;
  
  while (testLoop(0)) {
    long irValue = max30102.getIR();
    if (irValue < FINGER_THRESHOLD) break;
 
    if (!checkForBeat(irValue)) continue;
 
    heartbeatCounter++;
    Leanbot.tone(440, 50); 
    SeriaX.printtb(F("MAX30102\tBeat"), heartbeatCounter);
    MAX30102_printBpm(heartbeatCounter);
    Serial.println();
  }
}
 
void MAX30102_printBpm (unsigned int heartBeat) {
  static unsigned long beat1_timestamp = 0;
  unsigned long beatN_timestamp = millis();
 
  if ( heartBeat == 1 ) {
    beat1_timestamp =  beatN_timestamp;
    return;
  }
 
  // float bpm = (heartBeat - 1) *  60000.0            / (beatN_timestamp - beat1_timestamp);
  int bpmX100  = (heartBeat - 1) * (100UL * 60 * 1000) / (beatN_timestamp - beat1_timestamp);
  SeriaX.printtb(F( "bpm" ));   SeriaX.printX100(bpmX100);
}
 
  
/////////////////////////////////////////////
// HC-SR501 Sensor
/////////////////////////////////////////////
 
#define HC_SR501_Pin A1
 
void HC_SR501_printValue() {
  boolean pirValue = digitalRead(HC_SR501_Pin);
  SeriaX.printtb(F("HC-SR501"));
  Serial.println(pirValue);
}
 
 
/////////////////////////////////////////////
// OLED Module
/////////////////////////////////////////////
 
#include <U8g2lib.h>          // U8g2 Library Reference: https://github.com/olikraus/u8g2/wiki
 
U8G2_SH1106_128X64_NONAME_1_HW_I2C oled(U8G2_R0);   // SH1106 for 1.3" OLED module
#define OLED_I2C_ADDRESS 0x3C    // I2C address of OLED
 
void OLED_begin() {
  Wire.beginTransmission( OLED_I2C_ADDRESS );
  boolean initOK = ( Wire.endTransmission() == 0 );        // endTransmission() returns 0 when the device responds
  reportInit(F( "OLED" ), initOK );
  if ( ! initOK )  return;
 
  oled.begin();
  OLED_draw();
}
 
void OLED_testLoop() {
  while ( testLoop(100) ) {
    OLED_draw();
  }
}

void OLED_draw() {
  const int WW = 128;           // Width of the screen
  const int HH = 64;            // Height of the screen
  const byte barWidth = 10;     // Width of the vertical bar
  static byte y = 0;            // Starting position of the bar (top)
  static int yStep = 2;         // Step size for vertical movement

  oled.firstPage();
  do {
    // Draw outer frame
    oled.drawFrame(0, 0, WW, HH);

    // Draw the moving vertical bar
    oled.drawBox(0, y, WW, barWidth); // Bar moving vertically across the screen
  } while (oled.nextPage());

  // Move the bar vertically
  y += yStep;

  // Reverse direction if the bar hits the top or bottom of the screen
  if ((y <= 0) || (y >= HH - barWidth)) {
    yStep = -yStep;
  }
}
 
 
/////////////////////////////////////////////
// Capacitive Soil Moisture Sensor
/////////////////////////////////////////////
 
#define SoilMoisture_Pin A0
 
void SoilMoisture_printValue() {
  SeriaX.printtb(F("SoilMoisture"));
  Serial.println(1023 - analogRead(SoilMoisture_Pin));
}
 
 
/////////////////////////////////////////////
// BME280 Sensor
/////////////////////////////////////////////
 
#include <ErriezBMX280.h>     // https://github.com/Erriez/ErriezBMX280
 
ErriezBMX280 bme280 = ErriezBMX280(0x76);
boolean BME_initOK = false;
 
void BME280_begin() {
  BME_initOK = bme280.begin();
  reportInit(F( "BME280" ), BME_initOK );
}
 
void BME280_printValues() {
  if ( ! BME_initOK )  return;
 
  float altitude_apr = bme280.readAltitudeAprx();
  int temperature100 = bme280.readTemperatureX100();
  int humidity100 = bme280.readHumidityX100();
  long pressure100 = bme280.readPressureX1();
 
  SeriaX.printtb(F( "BME280"  ));
  SeriaX.printtb(F( "Tem"  )); SeriaX.printX100( temperature100, ' '  );
  SeriaX.printtb(F( "Hum"  )); SeriaX.printX100( humidity100,    '\t' );
  SeriaX.printtb(F( "Pres" )); SeriaX.printX100( pressure100,    '\t' );
  SeriaX.printtb(F( "Alt"  ), altitude_apr    ); 
  Serial.println();
}
 
 
// /////////////////////////////////////////////
// // WiFi Module
// /////////////////////////////////////////////
 
void WiFi_setPassword() {
  String nextLine = Serial.readStringUntil( '\n' );     // WiFi Connect ??ssid??
 
  // boolean initOK = ( LbIoT.Wifi.begin( nextLine.c_str() + 13, serCommand.c_str() + 14) >= 0 );
  // reportInit(F( "WiFi" ), initOK );
  // if ( ! initOK ) return;
 
  while ( testLoop(250) ) {
    time_t t = WiFi.getTime();
    if ( year(t) >= 2024 )  return  setTime( t );     // proper year should be >= 2024
  }
}
 
void WiFi_printTime() {
  if ( timeStatus() != timeSet )  return;
  
  SeriaX.printtb(F( "WiFi"  "\t"  "UTC time" ));
  printISOTime( now() );
  Serial.println();
}
 
void printISOTime(time_t t) {         // yyyy-mm-ddThh:mm:ssZ
  Serial.print(           year(t) );
  SeriaX.print00( '-' ,  month(t) );
  SeriaX.print00( '-' ,    day(t) );
  SeriaX.print00( 'T' ,   hour(t) );
  SeriaX.print00( ':' , minute(t) );
  SeriaX.print00( ':' , second(t) );
  Serial.write(   'Z' );
}
 

