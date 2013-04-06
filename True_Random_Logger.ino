/********************************/
/*  Rob Seward 2008-2009        */
/*  Derek Chafin 2010-2013      */
/*  v1.5.1                      */
/*  4/21/2011                   */
/********************************/
#include <SD.h>
#include <Random.h>
#include <Logger.h>
#include <SyncWait.h>

/***  Configure the RNG **************/
data_formats format = BINARY;
long baud_rate = 57600;
/*************************************/

const int adc_pin = A0;
const int led_pin = 13;
const int length = 4;
const byte startBytes[length] = { 0xa9, 0xf8, 0xf7, 0x40 };

Logger logger(SERIAL_OUTPUT, format, true, 10);
Random bits(adc_pin, VON_NEUMANN);
SyncWait waiter;

void setup(){
  Serial.begin(baud_rate);
  
//  waiter.doSyncWait();
  waiter.doSyncWait(startBytes, length);
  bits.calibrate();
  if (logger.init() != 0)
  {
    return;
  }
}

void loop(){
  byte data = bits.get_byte();
  logger.logData(data);  
}
