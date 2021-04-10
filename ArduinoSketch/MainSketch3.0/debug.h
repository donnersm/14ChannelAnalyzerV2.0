/********************************************************************************************************************************
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
 ********************************************************************************************************************************/

#pragma once
#include "Settings.h"

//**************************************************************************************************
//                                          D B G P R I N T                                        *
//**************************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.        *
// Print only if DEBUG flag is true.  Always returns the formatted string.                         *
// Usage dbgprint("this is the text you want: %d", variable);
//**************************************************************************************************
char * dbgprint(const char * format, ...) {
  if (DEBUG) {
    static char sbuf[DEBUG_BUFFER_SIZE]; // For debug lines
    va_list varArgs; // For variable number of params
    va_start(varArgs, format); // Prepare parameters
    vsnprintf(sbuf, sizeof(sbuf), format, varArgs); // Format the message
    va_end(varArgs); // End of using parameters
    if (DEBUG) // DEBUG on?
    {
      Serial.print("Debug: "); // Yes, print prefix
      Serial.println(sbuf); // and the info
    }
    return sbuf; // Return stored string
  }
}
