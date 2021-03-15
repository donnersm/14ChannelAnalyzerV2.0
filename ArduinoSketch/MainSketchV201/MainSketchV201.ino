//   Project: 14 Band Spectrum Analyzer using WS2812B/SK6812
//   Target Platform: Arduino Mega2560 or Mega2560 PRO MINI
//   The library Si5351mcu is being utilized for programming masterclock IC frequencies.
//   Special thanks to Pavel Milanes for his outstanding work. https://github.com/pavelmc/Si5351mcu
//   Analog reading of MSGEQ7 IC1 and IC2 use pin A0 and A1.
//   Clock pin from MSGEQ7 IC1 and IC2 to Si5351mcu board clock 0 and 1
//   Si5351mcu SCL and SDA use pin 20 and 21
//   See the Schematic Diagram for more info
//   Version 2.00  Adjusted to new hardware.Funtionally speaking, in hardware, Potmeter for Sense and speed are swopped!
//                 Improved comment / organised structure but no changes to functionality
//   Version 2.01  Build a subroutine for Diagnostics to assist in testing the complete setup and functionality.
//   to go to diagnostig mode, press and hold the mode key while giving a reset.
//   Use serial monitor at 115200 
//   The Idea of a 14 channel acrylic spectrum analyzer was set in to the world by Plantinum
//   I'm not sure if he was the first but he published the design and code on youtube
//   I redesigned hardware and code to my needs and also made it public. 
//   This is my version 2 of this setup.
//   Mark Donners AkA The Electronic Engineer
//   March 2021 
//***************************************************************************************************
#include <avr/wdt.h>
#include <Adafruit_NeoPixel.h>
#include <si5351mcu.h>    //Si5351mcu library
Si5351mcu FrequencyBoard;             //Si5351mcu Board
//#define PULSE_PIN     13  // only for debugging so i can toggle this output when i want, it has no function in the software
#define NOISE         20 //was 50 
#define ROWS          20  //num of row MAX=20 Change this if you have fewer leds per column
#define COLUMNS       14  //num of column.
#define DATA_PIN      9   //led data pin This is the dataline to the LEDstrips of the columns
#define STROBE_PIN    6   //MSGEQ7 strobe pin
#define RESET_PIN     7   //MSGEQ7 reset pin
#define NUMPIXELS    ROWS * COLUMNS // This is the calculation of total number of leds in the analyzer
char version[6] = "2.01"; // this is the active version
int Intensity = 20; // the overal brightness of the leds...if you make it brighter..the maximum current that is drawn from the power supply will increase!
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

#define DEBUG_BUFFER_SIZE 150 // Debug buffer size
int  DEBUG = 0 ;              // Debug on/off

/*
0: Blue with orange top
1: Blue with green top
2: Red with blue top
3: All red
4: Random color per column but all red top
5: Red top only
6: Random colors on all tiles but all red top
7: Green with red top
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
//*******************************************************************************************//
//*******************************************************************************************//
//*******************************************************************************************//

void setup()
{ 
 Serial.begin ( 115200 ) ;                              // For debug
  Serial.println() ;
  dbgprint("Init of frequency board");
  // Setup the frequency board
  FrequencyBoard.init(25000000L);
  FrequencyBoard.setFreq(0, 104570);
  FrequencyBoard.setFreq(1, 166280);
  // FrequencyBoard.setFreq(0, 9000);
  //FrequencyBoard.setFreq(1, 10000);
  FrequencyBoard.setPower(0, SIOUT_8mA);
  FrequencyBoard.setPower(1, SIOUT_8mA);
  FrequencyBoard.enable(0);
  FrequencyBoard.enable(1);
 
  dbgprint("Init of Freq board done.");
dbgprint ("Configuring datalines for MSGEQ7");
  
  //Configure outputs
  pinMode      (STROBE_PIN,    OUTPUT); //MSGEQ7 strobe pin configure as output pin
  pinMode      (RESET_PIN,     OUTPUT); //MSGEQ7 reset pin configure as output pin
  pinMode      (DATA_PIN,      OUTPUT); //Connection to LEDSTRIP configure as output pin
  pinMode(59, INPUT_PULLUP); // switch needs pullup resistor on input


  dbgprint("Init of MSGEQ7 IC's");
  // initialize the Analyzer Ic's
  digitalWrite (RESET_PIN,  LOW);
  digitalWrite (STROBE_PIN, LOW);
  delay        (1);
  digitalWrite (RESET_PIN,  HIGH);
  delay        (1);
  digitalWrite (RESET_PIN,  LOW);
  digitalWrite (STROBE_PIN, HIGH);
  delay        (1);
  dbgprint("Setup Pixel Leds");
  // setup WS2812 LEDS
  pixels.setBrightness(20); //set Brightness
  pixels.begin();
  pixels.show();
  randomSeed(analogRead(0)); // Shuffle the random generator algoritm

  // now is the time to see if we need to enable test mode.
  //In testmode we can do several self test to help diagnose if device is not working
  // for example is the mode switch = low than do something
  if (digitalRead(59) == LOW)Run_Diagnostics();
  
}



//******************************************************************************************//
//******************************  Main Function ********************************************//
//******************************************************************************************//
void loop()
{ 
  counter++;
  clearspectrum();
  if (millis() - pwmpulse > 3000) {
    toggle = !toggle;
   // digitalWrite(PULSE_PIN, toggle); //only for debugging
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
  pauzeTop = analogRead(4);
  pauzeTop = map(pauzeTop, 0, 1023, 100, 0);

  //read the sensitivity
  Sensitivity = analogRead(3);
  Sensitivity = map(Sensitivity, 0, 1023, 120, 1023);

  // read switch
  if (digitalRead(59) == LOW) {
    if ( (millis() - lastDebounceTime) > debounceDelay) {
      mode++;
      if (mode == 8)mode = 0;
      dbgprint("mode set to: %d",mode);
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
   // digitalWrite(PULSE_PIN, toggle); //only for debugging
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
 //   digitalWrite(PULSE_PIN, toggle); //only for debugging
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

void Run_Diagnostics()
{
  DEBUG = 1;  //this is needed to fully use the dbgprint function
  dbgprint("                                                                                      ");
  dbgprint("******************************Diagnostic Mode ****************************************");
  dbgprint("**************************************************************************************");
  dbgprint("*    Arduino Sketch Version %s                                                    *",version);   
  dbgprint("*                                                                                    *");
  dbgprint("* The Colors of the Dutch Flag will now alternate on all leds                        *");
  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }

  pixels.setBrightness(20);

  delay(500);
  dbgprint("* Showing first color of Dutch National Flag                                         *");
  dbgprint("* Press the Mode button to exit                                                      *");
  delay(500);
  while (digitalRead(59) == HIGH)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, 100, 0, 0);
    }
    pixels.show();
  }

  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }
  dbgprint("* Showing Second color of Dutch National Flag                                        *");
  dbgprint("* Press the Mode button to exit                                                      *");
  delay(500);
  while (digitalRead(59) == HIGH)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, 100, 100, 100);
    }
    pixels.show();
  }

  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }
  dbgprint("* Showing last color of Dutch National Flag                                          *");
  dbgprint("* Press the Mode button to exit                                                      *");
  delay(500);
  while (digitalRead(59) == HIGH)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, 0, 0, 100);
    }
    pixels.show();
  }
  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }



  
  dbgprint("*                                                                                    *");
  dbgprint("**************************** Diagnostic LED Test Results *****************************");
  dbgprint("* You where able to see the red, white and blue leds?                                *");
  dbgprint("* All leds where on? No defective ones?                                              *");
  dbgprint("* So far you have tested the Pixel leds and the mode switch                          *");
  dbgprint("**************************************************************************************");
  dbgprint("");
   for (int i = 0; i < (ROWS*4); i++) // all tiles in the first two columns
    {
      pixels.setPixelColor(i, 0, 100, 0); //  to green
    }
    pixels.show();
  
  // Now testing the freq board outputs
  const int pulsePin = 54;  //8 Input signal connected to Pin 12 of Arduino
  const int pulsePin2 = 55; //8 Input signal connected to Pin 12 of Arduino
  int pulseHigh;  // Integer variable to capture High time of the incoming pulse
  int pulseLow; // Integer variable to capture Low time of the incoming pulse
  float pulseTotal; // Float variable to capture Total time of the incoming pulse
  int frequency;  // Calculated Frequency
  int frequency2; // Calculated Frequency
  pinMode(pulsePin, INPUT);
  pinMode(pulsePin2, INPUT);
  dbgprint("                                                                                      ");
  dbgprint("*********************** Frequency Board Test *****************************************");
  dbgprint("*To test the outputs of the frequency board Remove both MSGEQ7 Ic's from the socket  *");
  dbgprint("*place a 1K resistor in each socket between pin 3(output) and pin 8(clock)           *");
  dbgprint("*Channel 0 should output a frequency around 5 Khz. while channel 1 will give 10Khz   *");
  dbgprint("*This is not accurate measurement and only a indication. Value +/- 500Hz is fine     *");
  dbgprint("*If a channel gives you a measurement of 0, it means it is not working               *");
  dbgprint("*Providing that you connected it correctly for this measurement                      *");
  dbgprint("*                                                                                    *");
  dbgprint("************************ Frequency Board Test ****************************************");
  dbgprint("                                                                                      ");
  FrequencyBoard.setFreq(0, 5000);  // Set channel 0 to 5Khz
  FrequencyBoard.setFreq(1, 10000); // Set channel 1 to 10Khz
  while (digitalRead(59) == HIGH)
  {// first calculate the frequency of the adc signal o
    pulseHigh = pulseIn(pulsePin, HIGH);
    pulseLow = pulseIn(pulsePin, LOW);
    pulseTotal = pulseHigh + pulseLow;  // Time period of the pulse in microseconds
    frequency = 1000000 / pulseTotal; // Frequency in Hertz (Hz)
   // now calculate the frequency of the adc signal 1
    pulseHigh = pulseIn(pulsePin2, HIGH);
    pulseLow = pulseIn(pulsePin2, LOW);
    pulseTotal = pulseHigh + pulseLow;  // Time period of the pulse in microseconds
    frequency2 = 1000000 / pulseTotal;  // Frequency in Hertz (Hz)
    // now print both
    dbgprint("Measured frequency channel 1: %d Hz  channel 2: %d Hz", frequency, frequency2);
    // dbgprint("press and hold the mode button to exit"); 
    delay(500);
  }

  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }

  for (int i = 0; i < (ROWS*8); i++) // all tiles in the first 4 rows to green
    {
      pixels.setPixelColor(i, 0, 100, 0); //  to green
    }
    pixels.show();


  dbgprint("******************************Amplifier test ****************************************");
  dbgprint("*This will print both adc values until you press the mode button.                   *");
  dbgprint("*Press the mode button to begin                                                     *");
  dbgprint("******************************Amplifier test ****************************************");
  while (digitalRead(59) == HIGH) {};
  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }

  while (digitalRead(59) == HIGH)
  {
    dbgprint("ADC value 0:  %d   ADC Value 1:  %d", analogRead(0), analogRead(1));
  };
  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }

  for (int i = 0; i < (NUMPIXELS); i++) // all tiles to green
    {
      pixels.setPixelColor(i, 0, 100, 0); //  to green
    }
    pixels.show();


  dbgprint("When you press the mode button, the system will go to normal operation mode but with the debug feedback on.");
  while (digitalRead(59) == HIGH){};
  while (digitalRead(59) == LOW)
  {
    // waiting until released)
    delay(500);
  }

  

}

 //**************************************************************************************************
//                                          D B G P R I N T                                        *
//**************************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.        *
// Print only if DEBUG flag is true.  Always returns the formatted string.                         *
// Usage dbgprint("this is the text you want: %d", variable);

//**************************************************************************************************
char* dbgprint ( const char* format, ... )
{
  static char sbuf[DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "Debug: " ) ;                           // Yes, print prefix
    Serial.println ( sbuf ) ;                          // and the info
  }
  return sbuf ;                                        // Return stored string
}
