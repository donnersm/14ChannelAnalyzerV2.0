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
******************************************************************************************************************************** 
*  Version History
*  1.0  First working version adapted from Platinum's free basic version All credits for this great idee go to him.
*  2.0  Implemented some minor changes version number update mostly related to hardware not firmware
*       However pay attention. The controls (potmeters) for Sensitivity and Peak delay have been swopped. you can adjust it 
*       in Setting.h if you are using version 1.0 of the hardware
*  ------
*  Latest Version
*  3.0
*  Complete redesign of firmware. Arduino sketch no longer has roots from Platinum. He did a great job but I've decided to go
*  a different route.
*  - Basis on FastLED Neomatrix. Now possible to use led displays.
*  - More patterns to display, including patterns that start in centre of matrix
*  - Rainbow mode screen saver has been replaced by Fire animation.
*  - Test routines to test the hardware are implemented ( press and hold the mode key for 5 seconds and follow the information
*     on the Serial Monitor 115200K
*  - Automatically change patterns every 10 seconds ( can be changed in settings section)
*  - All important setting an variables in one file: Setting.h to make adjusting more easy
*  - Extended comment to all coding.
********************************************************************************************************************************/   



/*******************************************************************************************************************************
* ** Libaries and external files                                                                                              **
********************************************************************************************************************************/
#include <FastLED_NeoMatrix.h>                    // Fastled Neomatrix driver.
#include <EasyButton.h>                           // Easybutton libary
#include "hardwaretest.h"
#include "Settings.h"                             // External file with all changeable Settings
#include "debug.h"                                // External file with debug subroutine
#include "fire.h"
#include <si5351mcu.h>                            //Si5351mcu library     


// Some definitions Do not change, to adjust, use the Settings.h file
#define BAR_WIDTH  (kMatrixWidth /(COLUMNS - 1))    // If width >= 8 light 1 LED width per bar, >= 16 light 2 LEDs width bar etc
#define TOP            (kMatrixHeight - 0)          // Don't allow the bars to go offscreen

// Peak related stuff we need
int Peakdelay;                                    // Delay before peak falls down to stack. Overruled by PEAKDELAY Potmeter
char PeakFlag[COLUMNS];                           // the top peak delay needs to have a flag because it has different timing while floating compared to falling to the stack
int PeakTimer[COLUMNS];                           // counter how many loops to stay floating before falling to stack

//Led matrix Arrays do not change unless you have more then 16 bands
byte peak[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // The length of these arrays must be >= COLUMNS
int oldBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // So if you have more then 16 bands, you must add zero's to these arrays
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Misc. Stuff
int colorIndex = 0;                               // Yep, we need this to keep track of colors
uint8_t colorTimer = 0;                           // Needed to change color periodically
long LastDoNothingTime = 0;                       // only needed for screensaver
int DemoModeMem=0;                                   // to remember what mode we are in when going to demo, in order to restore it after wake up
bool AutoModeMem=false;                                // same story
bool DemoFlag=false;                               // we need to know if demo mode was manually selected or auto engadged. 

// Button stuff
int buttonPushCounter = DefaultMode;               // Timer for Psuh Button
bool autoChangePatterns = false;                  // press the mode button 3 times within 2 seconds to auto change paterns.

// Defining some critical components
EasyButton modeBtn(Switch1);                      // The mode switch
Si5351mcu FrequencyBoard;                         //Si5351mcu Board

/********************************************************************************************************************************
  * ** // FastLED_NeoMaxtrix - see https://github.com/marcmerlin/FastLED_NeoMatrix for Tiled Matrixes, Zig-Zag and so forth                                                                         **
  ********************************************************************************************************************************/
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, kMatrixWidth, kMatrixHeight,
    NEO_MATRIX_BOTTOM        + NEO_MATRIX_LEFT +
    NEO_MATRIX_COLUMNS       + NEO_MATRIX_PROGRESSIVE +
    NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS);

/********************************************************************************************************************************
 * ** Header stuff done. now lets make some functions                                                                          **
 ********************************************************************************************************************************/

/********************************************************************************************************************************
 * ** Setup routine                                                                          **
 ********************************************************************************************************************************/
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.addLeds<CHIPSET, LOGO_PIN, COLOR_ORDER>(LogoLeds, NUM_LEDS_LOGO).setCorrection(TypicalSMD5050);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);
  FastLED.setBrightness(BRIGHTNESSMARK);
  FastLED.clear();

  modeBtn.begin();
  modeBtn.onPressed(changeMode);                             // When mode key is pressed, call changeMode sub routine
  modeBtn.onPressedFor(LONG_PRESS_MS, Run_Diagnostics);     // when pressed for the set time( default 3 secs, can be changed in definition of LONG_PRESS_MS), run diagnstics
  modeBtn.onSequence(5, 2000, startAutoMode);               // enable automode if pressed 5 times, within 2 seconds

  // Now let's set up the frequency board to output the needed frequencies
  dbgprint("Init of frequency board");
  FrequencyBoard.init(25000000L);
  FrequencyBoard.setFreq(0, 104570);
  FrequencyBoard.setFreq(1, 166280);
  FrequencyBoard.setPower(0, SIOUT_8mA);
  FrequencyBoard.setPower(1, SIOUT_8mA);
  FrequencyBoard.enable(0);
  FrequencyBoard.enable(1);

  dbgprint("Init of Freq board done.");
  dbgprint ("Configuring datalines for MSGEQ7");


  //Now let's configure the datalines for the MSGEQ7's and prepare them for running mode

  pinMode      (STROBE_PIN,    OUTPUT); //MSGEQ7 strobe pin configure as output pin
  pinMode      (RESET_PIN,     OUTPUT); //MSGEQ7 reset pin configure as output pin
  pinMode      (LED_PIN,      OUTPUT); //Connection to LEDSTRIP configure as output pin

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
}
/********************************************************************************************************************************
 * ** END OF setup routine                                                                                                     **
 ********************************************************************************************************************************/




void changeMode() {
  dbgprint("Button pressed");
  if (FastLED.getBrightness() == 0) FastLED.setBrightness(BRIGHTNESSMARK);  //Re-enable if lights are "off"
  autoChangePatterns = false;
  buttonPushCounter = (buttonPushCounter + 1) % NumberOfModes; //%6
  dbgprint("mode: %d\n", buttonPushCounter);
   if(DemoFlag==true)                 // in case we are in demo mode.... and manual go out of it.
   {
    dbgprint("demo is true");
    buttonPushCounter=DemoModeMem;     // go back to mode we had before demo mode
    LastDoNothingTime = millis();      // reset that timer to prevent going back to demo mode the same instance
    DemoFlag=false;
                   
   }
}

void startAutoMode() {
  autoChangePatterns = true;
  Matrix_Flag();                  //this is to show user that automode was engaged. It will show dutch flag for 2 seconds
  delay(2000);
  dbgprint(" Patterns will change after few seconds ");
  dbgprint(" You can reset by pressing the mode button again");
}

void brightnessOff() {
  FastLED.setBrightness(0);  //Lights out
}

/********************************************************************************************************************************
 * ** Main Loop                                                                                                                **
 ********************************************************************************************************************************/
void loop() {

  if (buttonPushCounter!=12)FastLED.clear(); // not for demo mode

  rainbow_wave(10, 10);                   // Call subroutine for logo update

  //get our user input
  AMPLITUDE = map(analogRead(SENSITIVITYPOT), 0, 1023, 50, 1023);         // read sensitivity potmeter and update amplitude setting
  BRIGHTNESSMARK = map(analogRead(BRIGHTNESSPOT), 0, 1023, BRIGHTNESSMAX, 10); // read brightness potmeter
  Peakdelay = map(analogRead(PEAKDELAYPOT), 0, 1023, 150, 1);             // update the Peakdelay time with value from potmeter
  FastLED.setBrightness(BRIGHTNESSMARK);                                  // update the brightness
  modeBtn.read();                                                         // what the latest on our mode switch?


  for (int i = 0; i < COLUMNS; i++)bandValues[i] = 0;                     // Reset bandValues[]

  // now reset the MSGEQ7's and use strobe to read out all current band values and store them in bandValues array
  digitalWrite(RESET_PIN, HIGH);
  delayMicroseconds(3000);
  digitalWrite(RESET_PIN, LOW);
  for (int i = 0; i < COLUMNS; i++)
  {
    digitalWrite(STROBE_PIN, LOW);
    delayMicroseconds(1000);
    bandValues[i] = analogRead(0) - NOISE;
    if (bandValues[i] < 120)bandValues[i] = 0;
    bandValues[i] = constrain(bandValues[i], 0, AMPLITUDE);
    bandValues[i] = map(bandValues[i], 0, AMPLITUDE, 0, kMatrixHeight); i++; //1023
    bandValues[i] = analogRead(1) - NOISE;
    if (bandValues[i] < 120)bandValues[i] = 0;
    bandValues[i] = constrain(bandValues[i], 0, AMPLITUDE);
    bandValues[i] = map(bandValues[i], 0, AMPLITUDE, 0, kMatrixHeight);
    if (bandValues[i] > DemoTreshold && i>1)LastDoNothingTime = millis(); // if there is signal in any off the bands[>2] then no demo mode 
    digitalWrite(STROBE_PIN, HIGH);
  }
   
  // Process the  data from bandValues and transform them into bar heights
  for (byte band = 0; band < COLUMNS; band++)
  { // Scale the bars for the display
    int barHeight = bandValues[band];
    if (barHeight > TOP) barHeight = TOP;
    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2; // Fast Filter, more rapid movement
    // barHeight = ((oldBarHeights[band] * 2) + barHeight) / 3; // minimum filter makes changes more smooth

    // Move peak up
    if (barHeight > peak[band])
    {
      peak[band] = min(TOP, barHeight);
      PeakFlag[band] = 1;
    }
    /*
       Mode 1: TriBar each Column is devided into 3 sections, Bottom,Middle and Top, Each section has different color
       Mode 2: Each Column different color, white peaks
       Mode 3: Each Colomn has the same gradient from a color pattern, white peaks
       Mode 4: All is red color, blue peaks
       Mode 5: All is blue color, red peaks
       Mode 6: Center Bars following defined color pattern, Red White Yellow
       Mode 7: Center Bars following defined color pattern ---> White Red
       Mode 8: Center Bars following defined color pattern ---> Pink White Yellow
       Mode 9: Peaks only, color depends on level (height)
       Mode 10: Peaks only, blue color
       Mode 11: Peaks only, color depends on level(height), following the tribar pattern
       Mode 12: Fire, doesn't response to music
       Mode 0: Gradient mode, colomns all have the same gradient but gradient changes following a rainbow pattern
    */

   // if there hasn't been much of a input signal for a longer time ( see settings ) go to demo mode
   if ((millis() - LastDoNothingTime) > DemoAfterSec && DemoFlag==false)
   { dbgprint("In loop 1:  %d", millis() - LastDoNothingTime);
    DemoFlag=true;
    // first store current mode so we can go back to it after wake up
    DemoModeMem=buttonPushCounter;
    AutoModeMem=autoChangePatterns;
    autoChangePatterns=false;
    buttonPushCounter=12;
    dbgprint("Automode is turned of because of demo");
   } 
   // Wait,signal is back? then wakeup!     
    else if (DemoFlag==true &&   (millis() - LastDoNothingTime) < DemoAfterSec   )   
    { dbgprint("In loop 2:  %d", millis() - LastDoNothingTime);
      // while in demo the democounter was reset due to signal on one of the bars.
      // So we need to exit demo mode.
      buttonPushCounter=DemoModeMem; // restore settings
      dbgprint ("automode setting restored to: %d",AutoModeMem);
      autoChangePatterns=AutoModeMem;// restore settings
      DemoFlag=false;  
    }
    
    // Now visualize those bar heights
    switch (buttonPushCounter) {
    case 0:
      changingBars(band, barHeight);
      break;
    case 1: 
      TriBar(band, barHeight);
      TriPeak(band); 
      break;
    case 2:
      rainbowBars(band, barHeight);
      NormalPeak(band, PeakColor1);
      break;
    case 3:
      purpleBars(band, barHeight);
      NormalPeak(band, PeakColor2);
      break;
    case 4:
      SameBar(band, barHeight); 
      NormalPeak(band, PeakColor3);
      break;
    case 5:
      SameBar2(band, barHeight); 
      NormalPeak(band, PeakColor4);
      break;
    case 6:
      centerBars(band, barHeight); 
      break;
    case 7:
      centerBars2(band, barHeight);
      break;
    case 8:
      centerBars3(band, barHeight); 
      break;
    case 9:
      outrunPeak(band);
        // no bars
      break;
    case 10:
      NormalPeak(band, PeakColor5);
      // no bars
      break;
    case 11:
      // no bars
      TriPeak2(band);
      break;
    case 12:
      make_fire(); // go to demo mode
      break;
    } 
    oldBarHeights[band] = barHeight;  // Save oldBarHeights for averaging later
  }

  // Decay peak
  EVERY_N_MILLISECONDS(Fallingspeed) {
    for (byte band = 0; band < COLUMNS; band++) {
      if (PeakFlag[band] == 1) {
        PeakTimer[band]++;
        if (PeakTimer[band] > Peakdelay) {
          PeakTimer[band] = 0;
          PeakFlag[band] = 0;
        }
      } else if (peak[band] > 0) {
        peak[band] -= 1;
      }
    }
    colorTimer++;
  }
  EVERY_N_MILLISECONDS(10) colorTimer++;                         // Used in some of the patterns
  EVERY_N_SECONDS(AutoChangetime) {
    if (autoChangePatterns) buttonPushCounter = (buttonPushCounter + 1) % (NumberOfModes - 1); //timer to autochange patterns when enabled but exclude demo mode
   dbgprint("Change=true?:%d Now in mode:%d",autoChangePatterns,buttonPushCounter);
  }

  FastLED.show();
}
/********************************************************************************************************************************
 * ** END SUB  Main Loop                                                                                         **
 ********************************************************************************************************************************/

/********************************************************************************************************************************
 * ** SUB Rountines related to Paterns and Peaks                                                                               **
 ********************************************************************************************************************************/

void rainbowBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix -> drawPixel(x, y, CHSV(RainbowBar_Color));
    }
  }
}

void SameBar(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix -> drawPixel(x, y, CHSV(SameBar_Color1)); //red

    }
  }
}

void SameBar2(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix -> drawPixel(x, y, CHSV(SameBar_Color2)); //blue

    }
  }
}

void TriBar(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      if (y < 4) matrix -> drawPixel(x, y, CHSV(TriBar_Color_Top));     //Top red
      else if (y > 8) matrix -> drawPixel(x, y, CHSV(TriBar_Color_Bottom)); //green
      else matrix -> drawPixel(x, y, CHSV(TriBar_Color_Middle));      //yellow
    }
  }
}

void purpleBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix -> drawPixel(x, y, ColorFromPalette(purplePal, y * (255 / (barHeight + 1))));
    }
  }
}

void changingBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix -> drawPixel(x, y, CHSV(ChangingBar_Color));
    }
  }
}

void centerBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    if (barHeight < 0) barHeight = 1; // at least a white line in the middle is what we want
    int yStart = ((kMatrixHeight - barHeight) / 2);
    for (int y = yStart; y <= (yStart + barHeight); y++) {
      int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
      matrix -> drawPixel(x, y, ColorFromPalette(heatPal, colorIndex));
    }
  }
}

void centerBars2(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    if (barHeight < 0) barHeight = 1; // at least a white line in the middel is what we want
    int yStart = ((kMatrixHeight - barHeight) / 2);
    for (int y = yStart; y <= (yStart + barHeight); y++) {
      int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
      matrix -> drawPixel(x, y, ColorFromPalette(markPal, colorIndex));
    }
  }
}

void centerBars3(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    if (barHeight < 0) barHeight = 1; // at least a white line in the middel is what we want
    int yStart = ((kMatrixHeight - barHeight) / 2);
    for (int y = yStart; y <= (yStart + barHeight); y++) {
      int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
      matrix -> drawPixel(x, y, ColorFromPalette(markPal2, colorIndex));
    }
  }
}

void NormalPeak(int band, int H, int S, int V) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix -> drawPixel(x, peakHeight, CHSV(H, S, V));
  }
}

void TriPeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (peakHeight < 4) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Top_Peak)); //Top red
    else if (peakHeight > 8) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Bottom_Peak)); //green
    else matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Middle_Peak)); //yellow
  }
}

void TriPeak2(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (peakHeight < 4) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Top_Peak2)); //Top red
    else if (peakHeight > 8) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Bottom_Peak2)); //green
    else matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Middle_Peak2)); //yellow
  }
}

void outrunPeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix -> drawPixel(x, peakHeight, ColorFromPalette(outrunPal, peakHeight * (255 / kMatrixHeight)));
  }
}
/********************************************************************************************************************************
 * ** END SUB Rountines related to Paterns and Peaks                                                                           **
 ********************************************************************************************************************************/

/********************************************************************************************************************************
 * ** Sub Routine for Diagnostics                                                                                              **
 ********************************************************************************************************************************/

void Run_Diagnostics() {
  delay(100);
  DEBUG = 1; //this is needed to fully use the dbgprint function
  dbgprint("**************************************************************************************");
  dbgprint("*   Diagnostic Mode                                                                  *");
  dbgprint("**************************************************************************************");
  dbgprint("*    Arduino Sketch Version %s                                                     *", version);
  dbgprint("*    Mark Donners, The Electronic Engineer                                           *");
  dbgprint("*    Website:   www.theelectronicengineer.nl                                         *");
  dbgprint("*    facebook:  https://www.facebook.com/TheelectronicEngineer                       *");
  dbgprint("*    youtube:   https://www.youtube.com/channel/UCm5wy-2RoXGjG2F9wpDFF3w             *");
  dbgprint("*    github:    https://github.com/donnersm                                          *");
  dbgprint("**************************************************************************************");
  dbgprint("\n");

  dbgprint("**************************************************************************************");
  dbgprint("* The Colors of the Dutch Flag will now alternate on all leds                        *");
  dbgprint("* Press the Mode button to exit                                                      *");
  dbgprint("**************************************************************************************");
  Matrix_Flag();
  WaitForKeyRelease();
  WaitforKeyPress();
  WaitForKeyRelease();
  dbgprint("\n");

  dbgprint("**************************************************************************************");
  dbgprint("* Now showing Rainbow mode                                                           *");
  dbgprint("* Press the Mode button to exit                                                      *");
  dbgprint("**************************************************************************************");
  dbgprint("\n\n");
  delay(500);

  while (1) {
    Matrix_Rainbow();
    if (digitalRead(Switch1) == LOW) break;
  }
  dbgprint("break detected");
  WaitForKeyRelease();

  dbgprint("**************************************************************************************");
  dbgprint("* Now Only the Logo Ledstrip will blink 9x in red color 1 sec on/ 1 sec off          *");

  FastLED.clear();
  for (int j = 0; j < 9; j++) {
    Logo_Blink();
    dbgprint("* Logo Blink test %d of 9 done                                                        *", j + 1);
  }
  dbgprint("* Testing of Logo Ledstrip is done                                                   *");
  dbgprint("**************************************************************************************");
  dbgprint("\n");
  WaitforKeyPress();
  WaitForKeyRelease();

  dbgprint("**************************** Diagnostic LED Test Finished  ***************************");
  dbgprint("* You where able to see the red, white and blue Flag? It's the Dutch Flag!           *");
  dbgprint("* All leds where on? No defective ones? Also, the Logo was blinking 9x in red, right?*");
  dbgprint("* Press the button again to continue                                                 *");
  dbgprint("**************************************************************************************");
  dbgprint("\n");

  WaitforKeyPress();
  WaitForKeyRelease();

  dbgprint("*********************** Frequency Board Test *****************************************");
  dbgprint("* To test the outputs of the frequency board Remove both MSGEQ7 Ic's from the socket *");
  dbgprint("* place a 1K resistor in each socket between pin 3(output) and pin 8(clock)          *");
  dbgprint("* Channel 0 should output a frequency around 5 Khz. while channel 1 will give 10Khz  *");
  dbgprint("* This is not accurate measurement and only a indication. Value +/- 500Hz is fine    *");
  dbgprint("* If a channel gives you a measurement of 0, it means it is not working              *");
  dbgprint("* To exit, press and hold the mode key for 3 seconds                                 *");
  dbgprint("************************ Frequency Board Test ****************************************");
  dbgprint("\n");
  FrequencyBoard.setFreq(0, 5000);    // Set channel 0 to 5Khz
  FrequencyBoard.setFreq(1, 10000);   // Set channel 1 to 10Khz
  Frequency_Test();

  FrequencyBoard.setFreq(0, 104570);  // back to working frequency
  FrequencyBoard.setFreq(1, 166280);  // back to working frequency

  WaitForKeyRelease();
  delay(500);
  dbgprint("****************************** Amplifier test ***************************************");
  dbgprint("* This will print both adc values until you press the mode button.                  *");
  dbgprint("* Press the mode button to begin                                                    *");
  dbgprint("****************************** Amplifier test ***************************************");
  dbgprint("\n");
  WaitforKeyPress();
  WaitForKeyRelease();
  while (digitalRead(59) == HIGH) {
    dbgprint("ADC value 0:  %d   ADC Value 1:  %d", analogRead(0), analogRead(1));
  };

  WaitForKeyRelease();
  delay(500);

  dbgprint("****************************** Potmeter test ****************************************");
  dbgprint("* This will print the mapped values potmeters until you press the mode button       *");
  dbgprint("* Sense: 50-1023 , Brightness: 10-Brightnessmax , Peak Delay: 1-150                 *");
  dbgprint("* Press the mode button to begin                                                    *");
  dbgprint("****************************** Potmeter test ****************************************");
  dbgprint("\n");
  WaitforKeyPress();
  WaitForKeyRelease();
  while (digitalRead(59) == HIGH) {
    AMPLITUDE = map(analogRead(SENSITIVITYPOT), 0, 1023, 1023, 50);     // read sensitivity potmeter and update amplitude setting
    BRIGHTNESSMARK = map(analogRead(BRIGHTNESSPOT), 0, 1023, BRIGHTNESSMAX, 10); // read brightness potmeter
    Peakdelay = map(analogRead(PEAKDELAYPOT), 0, 1023, 150, 1);       // update the Peakdelay time with value from potmeter
    dbgprint("Sense (50-1023):  %d  -  Brightness(10-Brightnessmax):  %d  -  Peak Delay(1-150): %d", AMPLITUDE, BRIGHTNESSMARK, Peakdelay);
  };

  WaitForKeyRelease();
  delay(500);
  dbgprint("* When you press the mode button, the system will go to normal operation mode but   *");
  dbgprint("* with the debug feedback on.                                                       *");
  dbgprint("******************************** END of TEST ****************************************");
  WaitforKeyPress();

}
/********************************************************************************************************************************
 * ** END Sub Routine for Diagnostics                                                                                              **
 ********************************************************************************************************************************/

/********************************************************************************************************************************
 * ** sub function to make rainbowcolors on ledstrip                                                                                                   **
 ********************************************************************************************************************************/
void rainbow_wave(uint8_t thisSpeed, uint8_t deltaHue) {      // The fill_rainbow call doesn't support brightness levels.

  // uint8_t thisHue = beatsin8(thisSpeed,0,255);                 // A simple rainbow wave.
  uint8_t thisHue = beat8(thisSpeed, 255); // A simple rainbow march.
  fill_rainbow(LogoLeds, NUM_LEDS_LOGO, thisHue, deltaHue);   // Use FastLED's fill_rainbow routine.
} // rainbow_wave()



void make_fire() {
  uint16_t i, j;
  if (t > millis()) return;
  t = millis() + (1000 / FPS);

  // First, move all existing heat points up the display and fade
  for (i = rows - 1; i > 0; --i) {
    for (j = 0; j < cols; ++j) {
      uint8_t n = 0;
      if (pix[i - 1][j] > 0)
        n = pix[i - 1][j] - 1;
      pix[i][j] = n;
    }
  }

  // Heat the bottom row
  for (j = 0; j < cols; ++j) {
    i = pix[0][j];
    if (i > 0) {
      pix[0][j] = random(NCOLORS - 6, NCOLORS - 2);
    }
  }

  // flare
  for (i = 0; i < nflare; ++i) {
    int x = flare[i] & 0xff;
    int y = (flare[i] >> 8) & 0xff;
    int z = (flare[i] >> 16) & 0xff;
    glow(x, y, z);
    if (z > 1) {
      flare[i] = (flare[i] & 0xffff) | ((z - 1) << 16);
    } else {
      // This flare is out
      for (int j = i + 1; j < nflare; ++j) {
        flare[j - 1] = flare[j];
      }
      --nflare;
    }
  }
  newflare();

  // Set and draw
  for (i = 0; i < rows; ++i) {
    for (j = 0; j < cols; ++j) {
      matrix -> drawPixel(j, rows - i, colors[pix[i][j]]);
    }
  }
}
