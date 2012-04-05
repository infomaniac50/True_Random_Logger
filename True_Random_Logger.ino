

/********************************/
/*  Rob Seward 2008-2009        */
/*  Derek Chafin 2010-2011      */
/*  v1.5                        */
/*  4/21/2011                   */
/********************************/

#include <Random.h>
#include <SD.h>
/***  Configure the RNG **************/
long baud_rate = 9600;
/*************************************/

const int error_pin = 7;
const int status_pin = 8;
const int chipSelect = 10;

Random bits1(A0, VON_NEUMANN);
Random bits2(A1, VON_NEUMANN);
Random bits3(A2, VON_NEUMANN);
Random bits4(A3, VON_NEUMANN);
File dataFile;

union bl
{
  byte b[4];
  unsigned long l;
} abl;

void setup(){
  Serial.begin(baud_rate);
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  pinMode(status_pin, OUTPUT);
  pinMode(error_pin, OUTPUT);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    blinkLed(error_pin, 2000);
    blinkLed(error_pin, 2000);
    blinkLed(error_pin, 2000);
    // don't do anything more
    return;
  }
  Serial.println("card initialized.");
  blinkLed(status_pin, 1000);
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open("random.dat", FILE_WRITE);
}

void loop(){
  byte data;
  static int sync_counter = 0;
  data = bits1.process();
  abl.b[0] = data;
  
  data = bits2.process();
  abl.b[1] = data;
  
  data = bits3.process();
  abl.b[2] = data;
  
  data = bits4.process();    
  abl.b[3] = data;

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.write(abl.b[0]);
    sync_counter++;
    dataFile.write(abl.b[1]);
    sync_counter++;
    dataFile.write(abl.b[2]);
    sync_counter++;
    dataFile.write(abl.b[3]);
    sync_counter++;
    
    if (sync_counter >= 2048)
    {
      dataFile.flush();
      blinkLed(status_pin, 30);
      sync_counter = 0;
    }    
  }  
  else {
    Serial.println("error opening random.dat");
    blinkLed(error_pin, 500);
  } 
  
//  Serial.println(abl.l);
//  Serial.flush();
}

void blinkLed(int pin, int timeout){
  digitalWrite(pin, HIGH);
  delay(timeout);
  digitalWrite(pin, LOW);
  delay(timeout);
}
