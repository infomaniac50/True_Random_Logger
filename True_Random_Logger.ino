/*
Original RNG circuit and code by Rob Seward 2008-2009
Project Page: http://robseward.com/misc/RNG2/
License: http://creativecommons.org/licenses/by-nc/2.5/
*/

/*
True_Random_Logger.ino - Generates and saves output from a custom hardware RNG.
Author: Derek Chafin
Version: 1.5.1
Modified: April 6, 2013
Project Page: http://www.coding-squared.com/blog/2011/12/arduino-hardware-random-number-generator/
License
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/.
*/

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