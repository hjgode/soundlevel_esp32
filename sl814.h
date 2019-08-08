#ifndef _SL814_H_
#define _SL814_H_

#include <Arduino.h>
//#include "splDaten.h"

struct splDaten{
  volatile bool bValid;
  volatile float level;
  volatile bool isFast;
  volatile bool isA;
  volatile uint8_t baselevel;
};

class SL814class{
  private:
    static int state;
    Stream* _serial;
    float parse_packet(byte buf[4]);
    splDaten parse_packet2(byte buf[4]);
    float decode_packet(byte daten[4]);
    bool receive_data(float* daten);
    volatile float soundLevel;
    bool readData(float * daten);
    void setKey(uint8_t bKey);
  public:
    SL814class(Stream* serialcom);
    float getMeasure();
    splDaten getDaten();
};



#endif
