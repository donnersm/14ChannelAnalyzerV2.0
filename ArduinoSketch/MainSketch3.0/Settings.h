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
char   version[]="3.00";                            // Define version number for reference only
// Debugging
#define DEBUG_BUFFER_SIZE 100                       // Debug buffer size
int  DEBUG = 1 ;                                    // When debug=1, extra information is printed to serial port. Turn of if not needed--> DEBUG=0

// Ledstrip Logo
#define LOGO_PIN        10                          // second ledstrip for logo.
#define NUM_LEDS_LOGO   10                          // how many leds on your logo. You can define more leds then connected, that will result in wider gradient.

// Ledstrips/ matrix main display
#define LED_PIN         9                           // This is the data pin of your led matrix, or ledstrips.
#define COLUMNS         14                          // Number of bands on display, this is not the same as display width...because display can be 28 ( double pixels per bar)
//const uint8_t                                                    // if you have more then 16 bands, you will need to change the Led Matrix Arrays in the main file.
#define kMatrixWidth  14                    // Matrix width --> number of columns in your led matrix
#define kMatrixHeight  20                   // Matrix height --> number of leds per column   
#define SERPENTINE     false                        // Set to false if you're LEDS are connected end to end, true if serpentine

// Some definitions Do not change, to adjust, use the Settings.h file
#define BAR_WIDTH  (kMatrixWidth /(COLUMNS - 1))    // If width >= 8 light 1 LED width per bar, >= 16 light 2 LEDs width bar etc
#define TOP            (kMatrixHeight - 0)          // Don't allow the bars to go offscreen
#define NUM_LEDS   (kMatrixWidth * kMatrixHeight)   // Total number of LEDs use the Setting.h file to change

// Ledstrips or pixelmatrix
#define CHIPSET         WS2812B                     // LED strip type -> Same for both ledstrip outputs( Matrix and logo)
#define BRIGHTNESSMAX 255                           // Max brightness of the leds...carefull...to bright might draw to much amps!
#define COLOR_ORDER     GRB                         // If colours look wrong, play with this
#define LED_VOLTS       5                           // Usually 5 or 12
#define MAX_MILLIAMPS   2000                        // Careful with the amount of power here if running off USB port, This will effect your brightnessmax. Currentlimit overrules it.
                                                    // If your power supply or usb can not handle the set current, arduino will freeze due to power drops

// ADC Filter
#define NOISE           20                          // Used as a crude noise filter on the adc input, values below this are ignored

//Controls
#define SENSITIVITYPOT  3                           // Potmeter for sensitivity input 0...5V (0-3.3V on ESP32)
#define BRIGHTNESSPOT   2                           // Potmeter for Brightness input 0...5V (0-3.3V on ESP32)  
#define PEAKDELAYPOT    4                           // Potmeter for Peak Delay Time input 0...5V (0-3.3V on ESP32)
#define Switch1         59                          // Connect a push button to this pin to change patterns
#define LONG_PRESS_MS   3000                        // Number of ms to count as a long press on the switch

// MSGEQ7 Connections
#define STROBE_PIN    6                             //MSGEQ7 strobe pin
#define RESET_PIN     7                             //MSGEQ7 reset pin

int BRIGHTNESSMARK= 100;                            // Default brightnetss, however, overruled by the Brightness potmeter
int AMPLITUDE    =   2000;                          // Depending on your audio source level, you may need to alter this value. it's controlled by the Sensitivity Potmeter

// Peak related stuff
#define Fallingspeed  30                            // This is the time it takes for peak tiels to fall to stack, this is not the extra time that you can add by using the potmeter
                                                    // for peakdelay. Because that is the extra time it levitates before falling to the stack    
#define AutoChangetime  10                          // If the time  in seconds between modes, when the patterns change automatically, is to fast, you can increase this number            

CRGB leds[NUM_LEDS];                               // Leds on the Ledstrips/Matrix of the actual Spectrum analyzer lights.
CRGB LogoLeds[NUM_LEDS_LOGO];                      // Leds on the ledstrip for the logo

#define NumberOfModes 13                           // The number of modes, remember it starts counting at 0,so if your last mode is 11 then the total number of modes is 12
#define DefaultMode 1                              // This is the mode it will start with after a reset or boot
#define DemoAfterSec  6000                           // if there is no input signal during this number of seconds, the unit will go to demo mode
#define DemoTreshold  10                             // this defines the treshold that will get the unit out of demo mode



/****************************************************************************
 * Colors of bars and peaks in different modes, changeable to your likings  *
 ****************************************************************************/
// Colors mode 0
#define ChangingBar_Color   y * (255 / kMatrixHeight) + colorTimer, 255, 255
// no peaks

// Colors mode 1 These are the colors from the TRIBAR 
#define TriBar_Color_Top      0 , 255, 255    // Red CHSV
#define TriBar_Color_Bottom   95 , 255, 255   // Green CHSV
#define TriBar_Color_Middle   45, 255, 255    // Yellow CHSV

#define TriBar_Color_Top_Peak      0 , 255, 255    // Red CHSV
#define TriBar_Color_Bottom_Peak   95 , 255, 255   // Green CHSV
#define TriBar_Color_Middle_Peak   45, 255, 255    // Yellow CHSV

// Colors mode 2
#define RainbowBar_Color  (x / BAR_WIDTH) * (255 / COLUMNS), 255, 255
#define PeakColor1  0, 0, 255       // white CHSV

// Colors mode 3
#define PeakColor2  0, 0, 255       // white CHSV
DEFINE_GRADIENT_PALETTE( purple_gp ) {
  0,   0, 212, 255,   //blue
255, 179,   0, 255 }; //purple
CRGBPalette16 purplePal = purple_gp;


// Colors mode 4
#define SameBar_Color1      0 , 255, 255      //red  CHSV
#define PeakColor3  160, 255, 255   // blue CHSV

// Colors mode 5
#define SameBar_Color2      160 , 255, 255    //blue  CHSV
#define PeakColor4  0, 255, 255   // red CHSV

// Colors mode 6
DEFINE_GRADIENT_PALETTE( redyellow_gp ) {  
  0,   200, 200,  200,   //white
 64,   255, 218,    0,   //yellow
128,   231,   0,    0,   //red
192,   255, 218,    0,   //yellow
255,   200, 200,  200 }; //white
CRGBPalette16 heatPal = redyellow_gp;
// no peaks

// Colors mode 7
DEFINE_GRADIENT_PALETTE( outrun_gp ) {
  0, 141,   0, 100,   //purple
127, 255, 192,   0,   //yellow
255,   0,   5, 255 };  //blue
CRGBPalette16 outrunPal = outrun_gp;
// no peaks

// Colors mode 8
DEFINE_GRADIENT_PALETTE( mark_gp2 ) {
  0,   255,   218,    0,   //Yellow
 64,   200, 200,    200,   //white
128,   141,   0, 100,   //pur
192,   200, 200,    200,   //white
255,   255,   218,    0,};   //Yellow
CRGBPalette16 markPal2 = mark_gp2;

// Colors mode 9
// no bars only peaks
DEFINE_GRADIENT_PALETTE( mark_gp ) {
  0,   231,   0,    0,   //red
 64,   200, 200,    200,   //white
128,   200, 200,    200,   //white
192,   200, 200,    200,   //white
255,   231, 0,  0,};   //red
CRGBPalette16 markPal = mark_gp;

// Colors mode 10
// no bars only peaks
#define PeakColor5  160, 255, 255   // blue CHSV

// These are the colors from the TRIPEAK mode 11
// no bars
#define TriBar_Color_Top_Peak2      0 , 255, 255    // Red CHSV
#define TriBar_Color_Bottom_Peak2   95 , 255, 255   // Green CHSV
#define TriBar_Color_Middle_Peak2   45, 255, 255    // Yellow CHSV

/******************************************************************
* Setting below are only related to the demo Fire mode            *
*******************************************************************/


#define FPS 15              /* Refresh rate 15 looks good*/

/* Flare constants */
const uint8_t flarerows = 8;  //2  /* number of rows (from bottom) allowed to flare */
const uint8_t maxflare = 4;//8;     /* max number of simultaneous flares */
const uint8_t flarechance = 50; /* chance (%) of a new flare (if there's room) */
const uint8_t flaredecay = 14;  /* decay rate of flare radiation; 14 is good */

/* This is the map of colors from coolest (black) to hottest. Want blue flames? Go for it! */
const uint32_t colors[] = {
  0x000000,
  0x100000,
  0x300000,
  0x600000,
  0x800000,
  0xA00000,
  0xC02000,
  0xC04000,
  0xC06000,
  0xC08000,
  0x807080
};
