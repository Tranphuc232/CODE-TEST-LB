#include <Leanbot.h>
#include <Wire.h>             // Wire Library Reference: https://www.arduino.cc/en/Reference/Wire
 
String serCommand; 
 
void setup() {
  Serial.begin(115200);
  LbTouch.read(TB1A);
  
  Wire.begin();
  Wire.setClock( I2C_CLK_400kHz ); 
  MPU_begin();
  APDS_begin();
  DCMotor_begin();
  VL53L0X_begin();
}
 
void loop() {
  while ( testLoop(0) ) {
    MPU_Report();  
    Mx4466_Report();  
    APDS_Report();  
    Mx4466_Report();
    VL53L0X_Report();
    Mx4466_Report();
  }
  
  serCommand = Serial.readStringUntil( '\n' );
  if (checkCommand(F( "DCMotor" )))  return DCMotor_control();
}
 
boolean checkCommand (const __FlashStringHelper* command) {
  return  serCommand.startsWith( command );
}
 
void reportInit(const __FlashStringHelper* moduleName, boolean initOK) {
  SeriaX.printtb( moduleName, F( "Init" ));
  Serial.println( initOK ? F( "Ok" ) : F( "Error" ));        
}
 
boolean testLoop(byte delayMs) {
  delay(delayMs);
  return  ( Serial.available() <= 0 );
}
 
 
/////////////////////////////////////////////
// APDS9960 Sensor
/////////////////////////////////////////////
 
#include <Arduino_APDS9960.h>
 
boolean APDS_initOK = false;
 
void APDS_begin() {
  APDS_initOK = APDS.begin();
  reportInit(F("APDS9960"), APDS_initOK);
 
  if (! APDS_initOK) return;
 
  APDS.colorAvailable();
  APDS.proximityAvailable();
  APDS.gestureAvailable();
}
 
void APDS_Report() {
  if (! APDS_initOK) return;
 
  while ( testLoop(0) ) {
    int r, g, b, c;
    APDS.readColor(r, g, b, c);
    APDS.colorAvailable();
   
    int proximity = APDS.readProximity();
    APDS.proximityAvailable();
   
    SeriaX.printtb(F( "APDS9960" "\t" "RGBC" ), r, g, b, c);
    SeriaX.printtb(F( "Prox" ), proximity);
 
    APDS_ReportGesture();
    Serial.println();
 
    if ( proximity >= 200 )   return;
    delay( 100 );
  }
}
 
void APDS_ReportGesture() {
  if ( ! APDS.gestureAvailable())  return;
 
  SeriaX.printtb(F( "gesture" ));
 
  int gesture = APDS.readGesture();
  switch (gesture) {
    case GESTURE_UP   :  Serial.print(F( "DOWN"  ));  break;
    case GESTURE_DOWN :  Serial.print(F( "UP"    ));  break;
    case GESTURE_LEFT :  Serial.print(F( "RIGHT" ));  break;
    case GESTURE_RIGHT:  Serial.print(F( "LEFT"  ));  break;
  }
}
 
 
/////////////////////////////////////////////
// MAX4466 Sensor
/////////////////////////////////////////////
 
void Mx4466_Report() {
  long sum = 0, ssum = 0;
  const int N_SAMPLES = 256;
 
  for (int i = 0; i < N_SAMPLES; i++) {
    analogRead(A4);  
    int soundSample = analogRead(A6) - 512;
    sum  += soundSample;
    ssum += soundSample * soundSample;
  }
 
  long Mean = 512 + sum / N_SAMPLES;
  long Var  = (ssum - ((sum * sum) / N_SAMPLES)) / N_SAMPLES;
 
  SeriaX.printtb(F( "MAX4466" "\t" "Mean"     ), Mean );
  SeriaX.printtb(F(                "Variance" ), Var  );      
  Serial.println();
}
 
 
/////////////////////////////////////////////
// MPU6050 Sensor
/////////////////////////////////////////////
 
#define   MPU6050_INCLUDE_DMP_MOTIONAPPS20
#include "MPU6050.h"
#include "MPU6050_6Axis_MotionApps_V6_12.h"
 
MPU6050 mpu;
 
boolean MPU_initOK = false;
 
void MPU_begin() {
  mpu.initialize();
  MPU_initOK = mpu.testConnection() && (mpu.dmpInitialize() == 0);
  reportInit(F("MPU6050"), MPU_initOK);
 
  if (!MPU_initOK) return;
 
  mpu.setDMPEnabled(true);
}
 
void MPU_Report() {
  if (!MPU_initOK) return;
 
  uint8_t fifoBuffer[64];  
  if ( ! mpu.dmpGetCurrentFIFOPacket(fifoBuffer))  return;
 
  int16_t temperatureRaw = mpu.getTemperature();  
//int temperatureC_100 = 3653 + (temperatureRaw * 10) / 34;          //  100 * (36.53 + temperatureRaw/340.0)
  int temperatureC_100 = 3653 + (temperatureRaw * 5*15) / 256;       //  5/17 ~= (5*15) / 256
  
  VectorInt16 a, g;     // Accel, Gyro
  int16_t q[4];         // Quaternion
  mpu.dmpGetAccel(&a, fifoBuffer);
  mpu.dmpGetGyro(&g, fifoBuffer);
  mpu.dmpGetQuaternion(q, fifoBuffer);
 
  SeriaX.printtb(F( "MPU6050" ));
  SeriaX.printtb(F( "Axyz"  ), a.x, a.y, a.z );  
  SeriaX.printtb(F( "Gxyz"  ), g.x, g.y, g.z );   
  SeriaX.printtb(F( "Qwxyz" ), q[0], q[1], q[2], q[3] );  
  SeriaX.printtb(F( "Temp" ));   SeriaX.printX100( temperatureC_100 );
  Serial.println();
}
 
 
/////////////////////////////////////////////
// DCMotor
/////////////////////////////////////////////
 
void DCMotor_begin() {
  Leanbot.DCMotor.setPower(0);
}
 
void DCMotor_control() {
  int powerLevel = serCommand.substring(8).toInt();                 // DCMotor 123
  Leanbot.DCMotor.setPower( powerLevel );                           // Update DC Motor power and direction
}
 
 
///////////////////////////////////////////
// VL53L0x
///////////////////////////////////////////
 
#include "Adafruit_VL53L0X.h"
Adafruit_VL53L0X VL53L0X = Adafruit_VL53L0X();
 
boolean VL53L0X_initOK = false;
 
void VL53L0X_begin() {
  VL53L0X_initOK = VL53L0X.begin();
  reportInit(F( "VL53L0x" ), VL53L0X_initOK );
}
 
void VL53L0X_Report() {
  if ( ! VL53L0X_initOK )  return;
 
  VL53L0X_RangingMeasurementData_t measure;
  VL53L0X.rangingTest( &measure, false );                            // pass in 'true' to get debug data printout!
   
  SeriaX.printtb(F( "VL53L0x" ), measure.RangeMilliMeter );
  Serial.println();
}