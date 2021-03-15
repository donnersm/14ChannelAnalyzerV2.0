//   Project: 14 Band Spectrum Analyzer using WS2812B/SK6812
//   Target Platform: Arduino Mega2560 or Mega2560 PRO MINI
//   The original code has been modified by PLATINUM to allow a scalable platform with many more bands.
//   It is not possible to run modified code on a UNO,NANO or PRO MINI. due to memory limitations.
//   The library Si5351mcu is being utilized for programming masterclock IC frequencies.
//   Special thanks to Pavel Milanes for his outstanding work. https://github.com/pavelmc/Si5351mcu
//   Analog reading of MSGEQ7 IC1 and IC2 use pin A0 and A1.
//   Clock pin from MSGEQ7 IC1 and IC2 to Si5351mcu board clock 0 and 1
//   Si5351mcu SCL and SDA use pin 20 and 21
//   See the Schematic Diagram for more info
//   Programmed and tested by PLATINUM
//   Version 1.0
//***************************************************************************************************
#include <avr/wdt.h>
#include <Adafruit_NeoPixel.h>
#include <si5351mcu.h>    //Si5351mcu library
Si5351mcu Si;             //Si5351mcu Board
#define PULSE_PIN     13
#define NOISE         20 //was 50
#define ROWS          20  //num of row MAX=20
#define COLUMNS       14  //num of column
#define DATA_PIN      9   //led data pin
#define STROBE_PIN    6   //MSGEQ7 strobe pin
#define RESET_PIN     7   //MSGEQ7 reset pin
#define NUMPIXELS    ROWS * COLUMNS
int Intensity = 20;
int pauzeTop = 1;
int Sensitivity = 1;
int mode = 0;
//bool modechange=false;
bool Showmode = false;
char barRed = 0, barGreen = 255, barBlue = 100;
char topRed = 205, topGreen = 50, topBlue = 0;
char randomRed = 0, randomGreen = 0, randomBlue = 0;
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 200;    // the debounce time; increase if the output flickers
long LastDoNothingTime = 0;
long DoNothingTime = 60000;
/*
   Mode 0 = green column with red top holding
   Mode 1 = blue column with red top holding
   Mode 2 =
   Mode 3 =
   Mode 4 =
   Mode 5 =
   Mode 6 =
   Mode 7 =
   Mode 8 =
   Mode 9 =
   Mode 10 =
*/


struct Point {
  char x, y;
  char  r, g, b;
  bool active;
};
struct TopPoint {
  int position;
  int peakpause;
};
Point spectrum[ROWS][COLUMNS];
TopPoint peakhold[COLUMNS];
int spectrumValue[COLUMNS];
long int counter = 0;
int long pwmpulse = 0;
bool toggle = false;
int long time_change = 0;
int effect = 0;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  Si.init(25000000L);
  Si.setFreq(0, 104570);
  Si.setFreq(1, 166280);
  Si.setPower(0, SIOUT_8mA);
  Si.setPower(1, SIOUT_8mA);
  Si.enable(0);
  Si.enable(1);
  pinMode      (STROBE_PIN,    OUTPUT);
  pinMode      (RESET_PIN,     OUTPUT);
  pinMode      (DATA_PIN,      OUTPUT);
  pinMode      (PULSE_PIN,     OUTPUT);
  digitalWrite(PULSE_PIN, HIGH);
  delay(100);
  digitalWrite(PULSE_PIN, LOW);
  delay(100);
  digitalWrite(PULSE_PIN, HIGH);
  delay(100);
  digitalWrite(PULSE_PIN, LOW);
  delay(100);
  digitalWrite(PULSE_PIN, HIGH);
  delay(100);
  pixels.setBrightness(20); //set Brightness

  pixels.begin();
  pixels.show();
  pinMode      (STROBE_PIN, OUTPUT);
  pinMode      (RESET_PIN,  OUTPUT);
  digitalWrite (RESET_PIN,  LOW);
  digitalWrite (STROBE_PIN, LOW);
  delay        (1);
  digitalWrite (RESET_PIN,  HIGH);
  delay        (1);
  digitalWrite (RESET_PIN,  LOW);
  digitalWrite (STROBE_PIN, HIGH);
  delay        (1);
  pinMode(59, INPUT_PULLUP); // switch
  randomSeed(analogRead(0));
}
void loop()
{
  counter++;
  clearspectrum();
  if (millis() - pwmpulse > 3000) {
    toggle = !toggle;
    digitalWrite(PULSE_PIN, toggle);
    pwmpulse = millis();
  }
  digitalWrite(RESET_PIN, HIGH);
  delayMicroseconds(3000);
  digitalWrite(RESET_PIN, LOW);
  for (int i = 0; i < COLUMNS; i++) {

    digitalWrite(STROBE_PIN, LOW);
    delayMicroseconds(1000);
    spectrumValue[i] = analogRead(0)-NOISE;
    if (spectrumValue[i] < 120)spectrumValue[i] = 0;
 
    spectrumValue[i] = constrain(spectrumValue[i], 0, Sensitivity);
    spectrumValue[i] = map(spectrumValue[i], 0, Sensitivity, 0, ROWS); i++; //1023
    spectrumValue[i] = analogRead(1)-NOISE;
    if (spectrumValue[i] < 120)spectrumValue[i] = 0;
        
    spectrumValue[i] = constrain(spectrumValue[i], 0, Sensitivity);
    spectrumValue[i] = map(spectrumValue[i], 0, Sensitivity, 0, ROWS);
    if (spectrumValue[i] > 6)LastDoNothingTime = millis();
    digitalWrite(STROBE_PIN, HIGH);
  }

  // Read de set intensity
  Intensity = analogRead(2);
  Intensity = map(Intensity, 0, 1023, 150, 0);
  pixels.setBrightness(Intensity); //set Brightness

  // read pauzetijd van de top
  pauzeTop = analogRead(3);
  pauzeTop = map(pauzeTop, 0, 1023, 100, 0);

  //read the sensitivity
  Sensitivity = analogRead(4);
  Sensitivity = map(Sensitivity, 0, 1023, 120, 1023);

  // read switch
  if (digitalRead(59) == LOW) {
    if ( (millis() - lastDebounceTime) > debounceDelay) {
      mode++;
      if (mode == 8)mode = 0;
      ChangeMod();
      lastDebounceTime = millis();
    }
  }

  // do nothing timer....showdown if no input for long time
  if ((millis() - LastDoNothingTime) > DoNothingTime) {
    //mode++;
    //     if (mode == 8)mode = 0;
    //   ChangeMod();
    // Showmode=true;
    rainbow(25);
    LastDoNothingTime = millis();

  }
  //if(Showmode==true)StartDemo();
  // enter demo mode because long time no input

  for (int j = 0; j < COLUMNS; j++) {
    // randomRed=random(255);
    //randomGreen=random(255);
    //randomBlue=random(255);
    for (int i = 0; i < spectrumValue[j]; i++) {

      if (mode == 6) { // random colors om all tiles

        spectrum[i][j].active = 1;
        spectrum[i][j].r = random(255);          //COLUMN Color red
        spectrum[i][j].g = random(255);        //COLUMN Color green
        spectrum[i][j].b = random(255);          //COLUMN Color blue

      }
      else if (mode == 4) { // random color per column
        spectrum[i][j].active = 1;

        switch (j) {
          case 0:
            spectrum[i][j].r = 0;          //COLUMN Color red
            spectrum[i][j].g = 0;        //COLUMN Color green
            spectrum[i][j].b = 255;          //COLUMN Color blue
            break;
          case 1:
            spectrum[i][j].r = 0;          //COLUMN Color red
            spectrum[i][j].g = 255;        //COLUMN Color green
            spectrum[i][j].b = 0;          //COLUMN Color blue
            break;
          case 2:
            spectrum[i][j].r = 0;          //COLUMN Color red
            spectrum[i][j].g = 255;        //COLUMN Color green
            spectrum[i][j].b = 255;          //COLUMN Color blue
            break;
          case 3:
            spectrum[i][j].r = 255;          //COLUMN Color red
            spectrum[i][j].g = 0;        //COLUMN Color green
            spectrum[i][j].b = 0;          //COLUMN Color blue
            break;
          case 4:
            spectrum[i][j].r = 255;          //COLUMN Color red
            spectrum[i][j].g = 0;        //COLUMN Color green
            spectrum[i][j].b = 255;          //COLUMN Color blue
            break;
          case 5:
            spectrum[i][j].r = 255;          //COLUMN Color red
            spectrum[i][j].g = 255;        //COLUMN Color green
            spectrum[i][j].b = 0;          //COLUMN Color blue
            break;
          case 6:
            spectrum[i][j].r = 255;          //COLUMN Color red
            spectrum[i][j].g = 80;        //COLUMN Color green
            spectrum[i][j].b = 205;          //COLUMN Color blue
            break;
          case 7:
            spectrum[i][j].r = 50;          //COLUMN Color red
            spectrum[i][j].g = 100;        //COLUMN Color green
            spectrum[i][j].b = 255;          //COLUMN Color blue
            break;
          case 8:
            spectrum[i][j].r = 255;          //COLUMN Color red
            spectrum[i][j].g = 255;        //COLUMN Color green
            spectrum[i][j].b = 0;          //COLUMN Color blue
            break;
          case 9:
            spectrum[i][j].r = 215;          //COLUMN Color red
            spectrum[i][j].g = 40;        //COLUMN Color green
            spectrum[i][j].b = 120;          //COLUMN Color blue
            break;
          case 10:
            spectrum[i][j].r = 120;          //COLUMN Color red
            spectrum[i][j].g = 100;        //COLUMN Color green
            spectrum[i][j].b = 40;          //COLUMN Color blue
            break;
          case 11:
            spectrum[i][j].r = 50;          //COLUMN Color red
            spectrum[i][j].g = 10;        //COLUMN Color green
            spectrum[i][j].b = 90;          //COLUMN Color blue
            break;
          case 12:
            spectrum[i][j].r = 90;          //COLUMN Color red
            spectrum[i][j].g = 150;        //COLUMN Color green
            spectrum[i][j].b = 55;          //COLUMN Color blue
            break;
          case 13:
            spectrum[i][j].r = 95;          //COLUMN Color red
            spectrum[i][j].g = 155;        //COLUMN Color green
            spectrum[i][j].b = 25;          //COLUMN Color blue
            break;
        }


      }
      else {
        spectrum[i][j].active = 1;
        spectrum[i][j].r = barRed;          //COLUMN Color red
        spectrum[i][j].g = barGreen;        //COLUMN Color green
        spectrum[i][j].b = barBlue;          //COLUMN Color blue
      }
    }


    if (spectrumValue[j] - 1 > peakhold[j].position)
    {
      spectrum[spectrumValue[j] - 1][j].r = 0;
      spectrum[spectrumValue[j] - 1][j].g = 0;
      spectrum[spectrumValue[j] - 1][j].b = 0;
      peakhold[j].position = spectrumValue[j] - 1;
      peakhold[j].peakpause = pauzeTop;// 1; //set peakpause
    }
    else
    {
      spectrum[peakhold[j].position][j].active = 1;
      spectrum[peakhold[j].position][j].r = topRed;  //Peak Color red
      spectrum[peakhold[j].position][j].g = topGreen;  //Peak Color green
      spectrum[peakhold[j].position][j].b = topBlue;    //Peak Color blue
    }


  }

  flushMatrix();
  if (counter % 3 == 0)topSinking(); //peak delay



}
void topSinking()
{
  for (int j = 0; j < ROWS; j++)
  {
    if (peakhold[j].position > 0 && peakhold[j].peakpause <= 0) peakhold[j].position--;
    else if (peakhold[j].peakpause > 0) peakhold[j].peakpause--;
  }
}
void clearspectrum()
{
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      spectrum[i][j].active = false;
    }
  }
}
void flushMatrix()
{
  for (int j = 0; j < COLUMNS; j++)
  {



    for (int i = 0; i < ROWS; i++)
    {
      if (spectrum[i][j].active)
      {
        pixels.setPixelColor(j * ROWS + i, pixels.Color(
                               spectrum[i][j].r,
                               spectrum[i][j].g,
                               spectrum[i][j].b));
      }
      else
      {
        pixels.setPixelColor( j * ROWS + i, 0, 0, 0);
      }
    }

  }
  pixels.show();
}
void ChangeMod()
{
  switch (mode) {
    case 0:
      barRed = 0;
      barGreen = 255;
      barBlue = 0;

      topRed = 255;
      topGreen = 0;
      topBlue = 0;
      break;

    case 1:
      barRed = 0;
      barGreen = 0;
      barBlue = 255;

      topRed = 0;
      topGreen = 255;
      topBlue = 0;
      break;

    case 2:
      barRed = 255;
      barGreen = 0;
      barBlue = 0;

      topRed = 0;
      topGreen = 0;
      topBlue = 255;
      break;

    case 3:
      barRed = 255;
      barGreen = 0;
      barBlue = 0;

      topRed = 255;
      topGreen = 0;
      topBlue = 0;
      break;

    case 4:
      //each column diff color implemented elsewhere
      break;

    case 5:
      // rainbow accross
      break;
    case 6:
      // random collors implemented elsewhere
      break;
  }

}
void StartDemo()
{
  // rainbow should start
  rainbow(25);
}


void rainbow(int wait) {
  pixels.setBrightness(20);

  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
        if (millis() - pwmpulse > 3000) {
    toggle = !toggle;
    digitalWrite(PULSE_PIN, toggle);
    pwmpulse = millis();
  }
      
      digitalWrite(RESET_PIN, HIGH);
  delayMicroseconds(3000);
  digitalWrite(RESET_PIN, LOW);
  
       delayMicroseconds(1000);
    digitalWrite(STROBE_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STROBE_PIN, LOW);
    delayMicroseconds(1000);
    digitalWrite(STROBE_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STROBE_PIN, LOW);
  //  delayMicroseconds(1000);
  //  digitalWrite(STROBE_PIN, HIGH);
   // delayMicroseconds(1000);
    //digitalWrite(STROBE_PIN, LOW);
    spectrumValue[1] = analogRead(1);
    
    
    spectrumValue[1] = constrain(spectrumValue[1], 0, Sensitivity);
    spectrumValue[1] = map(spectrumValue[1], 0, Sensitivity, 0, ROWS); 
if (spectrumValue[1] >6){return;}
    digitalWrite(STROBE_PIN, HIGH);
  
    
    
    
    
    
    
    for (int i = 0; i < 280; i++) { // For each pixel in strip...
      
      /*
       * First check to see if there is activity on one of the bands. If true then reset
       *

       */
 // if (millis() - pwmpulse > 3000) {
 //   toggle = !toggle;
 //   digitalWrite(PULSE_PIN, toggle);
 //   pwmpulse = millis();
//  }

  

      //***
      int pixelHue = firstPixelHue + (i * 65536L / 280);
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
   }
    pixels.show(); // Update strip with new contents
   // delay(wait);  // Pause for a moment
  }
}
