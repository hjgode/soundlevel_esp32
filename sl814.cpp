#include <Arduino.h>
#include "sl814.h"

#define TRUE true
#define FALSE false

//#define MY_DEBUG

void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
       Serial.print("0x");
       for (int i=0; i<length; i++) {
         if (data[i]<0x10) {Serial.print("0");}
         Serial.print(data[i],HEX);
         Serial.print(" ");
       }
}

/* States */
enum states{
  SEND_INIT,
  GET_INIT_REPLY,
  SEND_PACKET_REQUEST,
  GET_PACKET,
};

enum keys{
  KEY_UP=0x20,
  KEY_DOWN=0x30,
  KEY_AC=0x40,
  KEY_FASTSLOW=0x50,
};

int SL814class::state=0;

void SL814class::setKey(uint8_t bKey){
  //if SL-814 is not in A and Fast mode
  /*
  Send key	
    0x10 0xKK 0x0d	
  Reply
    0xKK+1 0x0d	
  This command has the same effect as if a certain key/button on the SL-814 had been pressed. 
  The known value encodings for the key (0xKK) are: 
  0x20 = up arrow key, 
  0x30 = down arrow key, 
  0x40 = A/C key, 
  0x50 = fast/slow key. 
  There doesn't seem to be a key code for the "MAX" button or the power button. Thanks to Chris Hoogenboom for the info about this command.
   */
  #ifdef MY_DEBUG  
  char msg[128];
  #endif
  uint8_t ret=0;
  uint8_t buf[]={0x10, bKey, 0x0d};
  bool bOK=true;
  if((ret=_serial->write(buf, 3)) < 3) {
    #ifdef MY_DEBUG
    sprintf(msg, "setAmode error: %d.", ret);
    Serial.println(msg);  
    #endif
    bOK=false;
  }
  if(bOK){
    ret=_serial->readBytes(buf, 2);
    if(ret==2){
      if(buf[0]==++bKey && buf[1]==0x0d){
      #ifdef MY_DEBUG
      sprintf(msg, "setAmode OK");
      Serial.println(msg);  
      #endif
        
      }
    }
    else{
      #ifdef MY_DEBUG
      sprintf(msg, "setAmode=%i, 0x%02x, 0x%02x, 0x%02x", ret, buf[0], buf[1], buf[2]); 
      Serial.println(msg);  
      #endif
        
    }
  }
}

float SL814class::parse_packet(byte buf[4])
{
  Serial.println("parse_packet");

  float* floatval=0;
  uint16_t intval=0;
//  uint8_t level = 0
//  uint8_t level_bits;

  /* Byte 0 [7:7]: 0 = A, 1 = C */
//  bool is_a = ((buf[0] & (1 << 7)) == 0);

  /* Byte 0 [6:6]: Unknown/unused? */

  /* Byte 0 [5:4]: Level (00 = 40, 01 = 60, 10 = 80, 11 = 100) */
/*
  level_bits = (buf[0] >> 4) & 0x03;
  if (level_bits == 0)
    level = 40;
  else if (level_bits == 1)
    level = 60;
  else if (level_bits == 2)
    level = 80;
  else if (level_bits == 3)
    level = 100;
*/
  /* Byte 0 [3:3]: 0 = fast, 1 = slow */

//  bool is_fast = ((buf[0] & (1 << 3)) == 0);

  /* Byte 0 [2:0]: value[10..8] */
  /* Byte 1 [7:0]: value[7..0] */
  intval = (buf[0] & 0x7) << 8;
  intval |= buf[1];

  *floatval = (float)intval;

  /* The value on the display always has one digit after the comma. */
  *floatval /= 10;

  return *floatval;
}

splDaten SL814class::parse_packet2(byte buf[4])
{
  #ifdef MY_DEBUG
  Serial.println("parse_packet2");
  #endif
  
  splDaten sDaten;
  float floatval;
  uint16_t intval;
  uint8_t level = 0, level_bits;

  /* Byte 0 [7:7]: 0 = A, 1 = C */
  bool is_a = ((buf[0] & (1 << 7)) == 0);
  if(!is_a)
    setKey(KEY_AC);

  /* Byte 0 [6:6]: Unknown/unused? */

  /* Byte 0 [5:4]: Level (00 = 40, 01 = 60, 10 = 80, 11 = 100) */
  level_bits = (buf[0] >> 4) & 0x03;
  if (level_bits == 0)
    level = 40;
  else if (level_bits == 1)
    level = 60;
  else if (level_bits == 2)
    level = 80;
  else if (level_bits == 3)
    level = 100;

  /* Byte 0 [3:3]: 0 = fast, 1 = slow */
  bool is_fast = ((buf[0] & (1 << 3)) == 0);
  if(!is_fast)
    setKey(KEY_FASTSLOW);

  /* Byte 0 [2:0]: value[10..8] */
  /* Byte 1 [7:0]: value[7..0] */
  intval = (buf[0] & 0x7) << 8;
  intval |= buf[1];

  floatval = (float)intval;

  /* The value on the display always has one digit after the comma. */
  floatval /= 10;

  sDaten.bValid=true;
  sDaten.level=floatval;
  sDaten.isFast=is_fast;
  sDaten.isA=is_a;
  sDaten.baselevel=level;
  
  return sDaten;
}

float SL814class::decode_packet(byte daten[4]){
  return parse_packet(daten);
}

splDaten SL814class::getDaten(){
#ifdef MY_DEBUG  
  char msg[128];
#endif
  int ret=0;
  splDaten sDaten;
  byte buf[4]={0x30,0x01,0x0d,0x00};
  bool bOK=true;
  
  if((ret=_serial->write(buf, 3)) < 3) {
    #ifdef MY_DEBUG
    sprintf(msg, "SEND_PACKET_REQUEST error: %d.", ret);
    Serial.println(msg);  
    #endif
    bOK=false;
  }
  if(bOK){
    ret=_serial->readBytes(buf, 4);
    if(ret==4){
      if(buf[2]==0x02 && buf[3]==0x0d){
        sDaten=parse_packet2(buf);
      }
    }
    else{
      #ifdef MY_DEBUG
      sprintf(msg, "readBytes=%i, 0x%02x, 0x%02x, 0x%02x, 0x%02x", ret, buf[0], buf[1], buf[2], buf[3]); 
      Serial.println(msg);  
      #endif
        
    }
  }
  return sDaten;
}

bool SL814class::receive_data(float* result){
  uint8_t buf[4];
  char msg[128];
  int ret=0;
  int Try=0, maxTry=5;

  /* State machine. */
  if (state == SEND_INIT) {
    Serial.println("SEND_INIT");
    /* On the first run, send the "init" command. */
    buf[0] = 0x10;
    buf[1] = 0x04;
    buf[2] = 0x0d;
    sprintf(msg, "Sending init command: %02x %02x %02x.", buf[0], buf[1], buf[2]);
    Serial.println(msg);
    if(_serial->write(buf, 3) != 3) {
//    if ((ret = serial_write_blocking(serial, buf, 3, serial_timeout(serial, 3))) < 0) {
      sprintf(msg, "Error sending init command: %d.", ret);
      Serial.println(msg);

      Serial.println("SEND_INIT...error");
      return FALSE;
    }
    state = GET_INIT_REPLY;
  } else if (state == GET_INIT_REPLY) {
    _serial->flush();
    Serial.println("GET_INIT_REPLY");
    /* If we just sent the "init" command, get its reply. */
//    if ((ret = serial_read_blocking(serial, buf, 2, 0)) < 0) {
    Try=0;
    while (_serial->available()<2){
      if( Try>maxTry)
        break;
      delay(500);
      Try++;
      Serial.print("try ");
    }
    if( (ret=_serial->readBytes(buf, 2)) < 2) {
      Serial.print("Received init reply:"); Serial.print("l="); Serial.print(ret); Serial.print(": "); 
      Serial.print(buf[0], HEX); Serial.print(", "); Serial.println(buf[1],HEX);
      PrintHex8(buf, ret);
      sprintf(msg, "Error reading init reply: %d.", ret);
      Serial.println(msg);
  
      Serial.println("GET_INIT_REPLY...error");
      goto TEST;
      if(Try>=maxTry)
        return FALSE;
      else
        Try++;
    }
    sprintf(msg, "Received init reply: %02x %02x.", buf[0], buf[1]);
    Serial.println(msg);
    /* Expected reply: 0x05 0x0d */
    if (buf[0] != 0x05 || buf[1] != 0x0d) {
//      sprintf(msg, "Received incorrect init reply, retrying.");
      state = SEND_INIT;
      return FALSE;
    }
    TEST:
    state = SEND_PACKET_REQUEST;
  } else if (state == SEND_PACKET_REQUEST) {
    Serial.println("SEND_PACKET_REQUEST");
    /* Request a packet (send 0x30 ZZ 0x0d). */
    buf[0] = 0x30;
    buf[1] = 0x00; /* ZZ */
    buf[2] = 0x0d;
    sprintf(msg, "Sending data request command: %02x %02x %02x.", buf[0], buf[1], buf[2]);
    Serial.println(msg);
    if(_serial->write(buf, 3)<3) {
//    if ((ret = serial_write_blocking(serial, buf, 3, serial_timeout(serial, 3))) < 0) {
      sprintf(msg, "Error sending request command: %d.", ret);
      Serial.println(msg);

      Serial.println("SEND_PACKET_REQUEST...error");
      return FALSE;
    }
    state = GET_PACKET;
  } else if (state == GET_PACKET) {
    Serial.println("GET_PACKET");
    /* Read a packet from the device. */
    Try=0;
    while (_serial->available()<4){
      if( Try>maxTry)
        break;
      delay(500);
      Try++;
    }
    ret=_serial->readBytes(buf, 4);
//    ret = serial_read_nonblocking(serial, devc->buf + devc->buflen, 4 - devc->buflen);
    if (ret < 4) {
      sprintf(msg, "Error reading packet: %d.", ret);
      Serial.println(msg);

      Serial.println("GET_PACKET...error");
      state = SEND_PACKET_REQUEST;
      goto TEST;
    }

    sprintf(msg, "####### Received packet: %02x %02x %02x %02x.", buf[0], buf[1], buf[2], buf[3]);
    Serial.println(msg);

    /* Expected reply: AA BB ZZ+1 0x0d */
    if (buf[2] != 0x01 || buf[3] != 0x0d) {
      sprintf(msg, "Received incorrect request reply, retrying.");
      Serial.println(msg);

      state = SEND_PACKET_REQUEST;
      Serial.println("GET_PACKET incorrect reply");
      return FALSE;
    }

    float fval=decode_packet(buf);
    if (fval != 0.0){
      soundLevel=fval;
      *result=fval;
      return TRUE;
    }

    state = SEND_PACKET_REQUEST;
  } else {
    sprintf(msg, "Invalid state: %d.", state);
    Serial.println(msg);
    return false;
  }
  return false;
}

SL814class::SL814class(Stream* serialcom){
  _serial=serialcom;    
  state=SEND_INIT;  
  soundLevel=0.0;
  //  receive_data(&res);
}

float SL814class::getMeasure(){  
  float fval=0.0;
  receive_data(&fval);  
  return soundLevel;
}
    
