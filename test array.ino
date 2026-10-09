#include <Leanbot.h>
void setup() {
  Leanbot.begin();                  // khởi tạo Leanbot
}


void beep(int frw) {
  Leanbot.tone(440*4, frw);
  delay(100);
}

void loop() {
  // đọc giá trị 8 cảm biến ir0L đến ir7R
  int v0L = LbIRSensor.read(sir0L);
  int v1R = LbIRSensor.read(sir1R);
  int v2L = LbIRSensor.read(sir2L);
  int v3R = LbIRSensor.read(sir3R);
  int v4L = LbIRSensor.read(sir4L);
  int v5R = LbIRSensor.read(sir5R);
  int v6L = LbIRSensor.read(sir6L);
  int v7R = LbIRSensor.read(sir7R);
  
  byte line = LbIRLine.read(60);
  byte lineValue = LbIRLine.value();
 
  int gap = 0;
  Serial.print(v6L + 0*gap);
  Serial.print('\t');
  Serial.print(v4L + 1*gap);
  Serial.print('\t');
  Serial.print(v2L + 2*gap);
  Serial.print('\t');
  Serial.print(v0L + 3*gap);
  Serial.print('\t');
  Serial.print(v1R + 4*gap);
  Serial.print('\t');
  Serial.print(v3R + 5*gap);
  Serial.print('\t');
  Serial.print(v5R + 6*gap);
  Serial.print('\t');
  Serial.print(v7R + 7*gap);
  Serial.print('\t');  
  LbIRLine.println(line);

  delay(100);                       // thời gian trễ giữa mỗi lần đọc
}