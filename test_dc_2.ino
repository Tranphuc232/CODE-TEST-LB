#include <Leanbot.h>                    // use Leanbot library

void setup() {
  Leanbot.begin();                      // initialize Leanbot
}

void loop() {

  LbMotion.runLR( +2047, +2047 );
  LbMotion.waitDistanceMm( 1000 ); 
  LbDelay(100);
  LbMotion.runLR( -2047, -2047 );
  LbMotion.waitDistanceMm( -930 );
  LbMotion.runLR(0, 0);
 
LbDelay(100);
}