#include <FastLED.h>

#define LED_PIN_RING     3
#define LED_PIN_CENTER     5
#define NUM_LEDS_RING    12
#define NUM_LEDS_CENTER 1
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 100

CRGB leds_ring[NUM_LEDS_RING];
CRGB leds_center[NUM_LEDS_CENTER];

int pulse = 129;
int fadeAmount = 1;    

void setup() {
  delay( 1000 ); // power-up safety delay
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN_RING, COLOR_ORDER>(leds_ring,             NUM_LEDS_RING);
  FastLED.addLeds<LED_TYPE, LED_PIN_CENTER, COLOR_ORDER>(leds_center, NUM_LEDS_CENTER);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop()
{
  fadeToBlackBy( leds_ring, NUM_LEDS_RING, 20);
  fadeToBlackBy( leds_center, NUM_LEDS_CENTER, 20);
  byte dothue = 0;
  int delta = 15 ;
  for ( int i = 0; i < 80; i++) {
    leds_ring[beatsin16( i + 16, 0, NUM_LEDS_RING - 1 )] = CHSV(dothue, map( 255, 0, 1024, 0, 255), map(255, 0, 1024, 0, 255));
    leds_center[beatsin16( i + 16, 0, NUM_LEDS_CENTER - 1 )] = CHSV(dothue, map(255, 0, 1024, 0, 255), map( 255, 0, 1024, 0, 255));
    dothue = random(map( 255, 0, 1024, 0, 255) - delta, map(255, 0, 1024, 0, 255) + delta);
  }

  pulse = pulse + fadeAmount;
  if (pulse <= 128 || pulse >= 255) {
    fadeAmount = -fadeAmount;
  }

  FastLED.setBrightness(pulse);
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}
