#include <FastLED.h>

#define NUM_LEDS 8

class Led
{
    private:
        CRGB leds[NUM_LEDS];
    public:
        void setup();
        void blink();
};