#ifndef _SPL_DATEN_H_
#define _SPL_DATEN_H_
#include <Arduino.h>

class splDaten{
  public:
  bool volatile bValid=false;
  float volatile level=0.0;
  bool volatile isFast=true;
  bool volatile isA=true;
  volatile uint8_t baselevel;
  splDaten(){
    
  }
};
#endif
