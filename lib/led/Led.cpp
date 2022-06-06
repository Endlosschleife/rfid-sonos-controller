#include <Led.h>

void Led::setup()
{
    // pinMode(DATA_PIN, OUTPUT);
    FastLED.addLeds<NEOPIXEL, GPIO_NUM_26>(leds, NUM_LEDS);
}

void Led::blink()
{
    // Turn the LED on, then pause
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    // Now turn the LED off, then pause
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(500);
    Serial.println("blink");
}