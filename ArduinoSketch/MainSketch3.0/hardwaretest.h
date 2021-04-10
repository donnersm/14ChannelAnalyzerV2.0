/********************************************************************************************************************************
 *
 *  Project:         14 Band Spectrum Analyzer
 *  Target Platform: Arduino Mega2560 or Mega2560 PRO MINI
 *  
 *  Version: 3.00
 *  Hardware setup: https://github.com/donnersm/14ChannelAnalyzerV2.0/tree/main/Documentation
 *  Spectrum analyses done with analog chips MSGEQ7
 *  
 *  Mark Donners
 *  The Electronic Engineer
 *  Website:   www.theelectronicengineer.nl
 *  facebook:  https://www.facebook.com/TheelectronicEngineer
 *  youtube:   https://www.youtube.com/channel/UCm5wy-2RoXGjG2F9wpDFF3w
 *  github:    https://github.com/donnersm
 *  
 ********************************************************************************************************************************/

#pragma once
#include "debug.h"

// This  contains the subroutines to test all the hardware.

void WaitForKeyRelease() {
  while (digitalRead(Switch1) == LOW) // waiting until key released)
  {
    delay(500);
    dbgprint("in loop 1");
  }
  delay(500);
}

void WaitforKeyPress() {
  // move on the next test when key is pressed
  while (digitalRead(Switch1) == HIGH) {}; // wait until key is pressed
}

void Matrix_Flag() {

  int ledcounter = 0;
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight / 3; j++) {
      leds[ledcounter] = CRGB::Blue;
      ledcounter++;
    }
    for (int j = 0; j < kMatrixHeight / 3; j++) {
      leds[ledcounter] = CRGB::White;
      ledcounter++;
    }
    for (int j = 0; j < kMatrixHeight / 3; j++) {
      leds[ledcounter] = CRGB::Red;
      ledcounter++;
    }
    // if number of rows can not be devided by 3 then compensate
    for (int i = 0; i < (kMatrixHeight - (kMatrixHeight / 3) * 3); i++) {
      leds[ledcounter] = CRGB::Red;
      ledcounter++;
    }

  }
  FastLED.show();
}

void Matrix_Rainbow() {
  int thisSpeed = 10;
  uint8_t thisHue = beat8(thisSpeed, 255); // A simple rainbow march.
  // uint8_t thisHue = beatsin8(100,0,255);  
  fill_rainbow(leds, NUM_LEDS, thisHue, 10);
  FastLED.show();
}

void Logo_Blink() {
  for (int j = 0; j < NUM_LEDS_LOGO; j++) {
    LogoLeds[j] = CRGB::Red;
  }
  delay(1000);
  FastLED.show();
  for (int j = 0; j < NUM_LEDS_LOGO; j++) {
    LogoLeds[j] = CRGB::Black;
  }
  delay(1000);
  FastLED.show();
}
void Frequency_Test() {
  const int pulsePin = 54; //8 Input signal connected to Pin 12 of Arduino
  const int pulsePin2 = 55; //8 Input signal connected to Pin 12 of Arduino
  int pulseHigh; // Integer variable to capture High time of the incoming pulse
  int pulseLow; // Integer variable to capture Low time of the incoming pulse
  float pulseTotal; // Float variable to capture Total time of the incoming pulse
  int frequency; // Calculated Frequency
  int frequency2; // Calculated Frequency
  pinMode(pulsePin, INPUT);
  pinMode(pulsePin2, INPUT);

  while (digitalRead(Switch1) == HIGH) { // first calculate the frequency of the adc signal o
    pulseHigh = pulseIn(pulsePin, HIGH);
    pulseLow = pulseIn(pulsePin, LOW);
    pulseTotal = pulseHigh + pulseLow; // Time period of the pulse in microseconds
    frequency = 1000000 / pulseTotal; // Frequency in Hertz (Hz)
    // now calculate the frequency of the adc signal 1
    pulseHigh = pulseIn(pulsePin2, HIGH);
    pulseLow = pulseIn(pulsePin2, LOW);
    pulseTotal = pulseHigh + pulseLow; // Time period of the pulse in microseconds
    frequency2 = 1000000 / pulseTotal; // Frequency in Hertz (Hz)
    // now print both
    dbgprint("Measured frequency channel 1: %d Hz  channel 2: %d Hz", frequency, frequency2);
    // dbgprint("press and hold the mode button to exit"); 
    delay(500);
  }

}
