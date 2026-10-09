#include <Leanbot.h>
 
 
/*------------------------------------------------------------------*/
void setup() {
  Leanbot.begin();
 
  LbGripper.setCalibration(0, 90, 0, 90);  // đưa về trạng thái mặc định
 
}

 
void loop() {
  for(int i =0; i<5; i++)  {         // Nhấc thả vật 5 lần
    LbGripper.close();
    LbGripper.open();
  }
  
  LbGripper.setCalibration(0, 90, 0, 90);  // đưa về trạng thái mặc định
  LbMission.end();                   // dừng lại, kết thúc nhiệm vụ
}