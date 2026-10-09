

#include <Leanbot.h>                    // use Leanbot library

int SPEED_MAX = 1500;        
int vR, vL;     

const int NoWall = 768;


void setup() {
  Leanbot.begin();                      // initialize Leanbot
  while(1) {
      if (  (LbTouch.read(TB1A)) == HIGH && (LbTouch.read(TB1B) == HIGH)  ) break;
  }
}


void loop() {
  vL = LbIRArray.read(ir6L);
  vR = LbIRArray.read(ir7R);

  Serial.print(vL);
  Serial.print(" | ");
  Serial.println(vR);
  if(vL == NoWall && vR == NoWall  ){
    LbMotion.runLR(1000, 1000);
    delay(2000);
    LbMotion.runLR(500, -500);
    LbMotion.waitRotationDeg(180);
    LbMotion.runLR(1000, 1000);
    delay(1000);
  }
  else{
    Robot_WallFollow(vL, vR);
  }
  delay(100);
}

void Robot_WallFollow(int vL, int vR) {                                  

  const int KK = 700;                               // tỉ lệ điều chỉnh
  int deltaS = map(vL - vR, 0, KK, 0, SPEED_MAX);  

  if (deltaS > 0) {  
    LbMotion.runLR(SPEED_MAX - deltaS, SPEED_MAX);
  }

  else {
    LbMotion.runLR(SPEED_MAX, SPEED_MAX + deltaS);
  }
 
}