#include <Leanbot.h>                    // use Leanbot library
int v = 2047;
void setup() {
  Leanbot.begin();                      // initialize Leanbot
}

void loop() {

  LbMotion.runLR( +v, +v );
  LbMotion.waitDistanceMm( 1000 ); 
  LbDelay(100);
  LbMotion.runLR( -v, -v );
  LbMotion.waitDistanceMm( -930 );
  LbMotion.runLR(0, 0);
 
LbDelay(100);
}
