#include <FastLED.h>

CRGB led[7];             // Khai báo mảng đèn Led RGB


int t = 750;


void setup() { 
  // Cài đặt kết nối tới đèn Led RGB
  FastLED.addLeds<WS2812B, 13, GRB>(led, 7);
}

void loop() {
    for (int i = 0; i < 7; i++) {
      led[i] = CRGB::Red;
      FastLED.setBrightness(100);
      FastLED.show();
    }
    delay(t);   
    
   for (int i = 0; i < 7; i++) {
      led[i] = CRGB::Green;
      FastLED.setBrightness(100);
      FastLED.show();
    }
    delay(t);                        // thời gian đèn sáng

    for (int i = 0; i < 7; i++) {
        led[i] = CRGB::Blue;
        FastLED.setBrightness(100);
        FastLED.show();
      }
    delay(t);                        // thời gian đèn sáng
  
   for (int i = 0; i < 7; i++) {
      led[i] = CRGB::White;
      FastLED.setBrightness(100);
      FastLED.show();
    }
    delay(1000);   
}