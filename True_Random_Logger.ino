/********************************/
/*  Rob Seward 2008-2009        */
/*  Derek Chafin 2010-2011      */
/*  v1.5                        */
/*  4/21/2011                   */
/********************************/

#define BINS_SIZE 256
#define CALIBRATION_SIZE 50000

#define NO_BIAS_REMOVAL 0
#define EXCLUSIVE_OR 1
#define VON_NEUMANN 2

#define ASCII_BYTE 0
#define BINARY 1
#define ASCII_BOOL 2

/*Data Capture Options*/
#define SERIAL_ONLY      0
#define SD_ONLY          1
#define BOTH             2

/*SD Card Debug Options*/
#define NO_DEBUG         0
#define SERIAL_DEBUG     1
#define LED_DEBUG        2


/***  Configure the RNG **************/
#define DEBUG_MODE  SERIAL_DEBUG
#define DATA_MODE  SERIAL_ONLY
int bias_removal = VON_NEUMANN;
int output_format = ASCII_BYTE;
int baud_rate = 19200;
/*************************************/

const int adc_pin = A0;
const int led_pin = 13;
unsigned int bins[BINS_SIZE];
boolean initializing = true;
unsigned int calibration_counter = 0;

#if DATA_MODE  !=  SERIAL_ONLY
#include <SD.h>
const int chipSelect = 4;
char* name = "RANDOM.DAT";
File file;
#endif

void setup(){
  pinMode(led_pin, OUTPUT);
#if DEBUG_MODE == SERIAL_DEBUG || DATA_MODE  !=  SD_ONLY
Serial.begin(baud_rate);
#endif  

#if DEBUG_MODE == SERIAL_DEBUG
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif

#if DATA_MODE  !=  SERIAL_ONLY

#if DEBUG_MODE == SERIAL_DEBUG
  Serial.print("Initializing SD card...");
#endif
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  //Hardware SS pin on the Arduino Mega is pin 53
  pinMode(53, OUTPUT);
#else
  pinMode(10, OUTPUT);
#endif
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
#if DEBUG_MODE  ==  SERIAL_DEBUG
    Serial.println("Card failed, or not present");
#endif

#if DEBUG_MODE  ==  LED_DEBUG
    blinkLedDelay(1000);
    delay(1000);
    blinkLedDelay(1000);
    delay(1000);
    blinkLedDelay(1000);
#endif
// don't do anything more:
    return;
  }

  file = SD.open(name, FILE_WRITE);
  file.seek(file.size());

#if DEBUG_MODE  ==  SERIAL_DEBUG  
  Serial.print("Logging to: ");
  Serial.println(name);
//  Serial.print("Free RAM: ");       // This can help with debugging, running out of RAM is bad
//  Serial.println(FreeRam());
#endif

#endif //DATA_MODE != SERIAL_ONLY

  for (int i=0; i < BINS_SIZE; i++){
    bins[i] = 0; 
  }
#if DEBUG_MODE  ==  SERIAL_DEBUG
  Serial.println("BINS Done");
#endif

#if DEBUG_MODE  ==  LED_DEBUG
  blinkLedDelay(250);
  delay(250);
  blinkLedDelay(250);
  delay(250);
  blinkLedDelay(250);
#endif
}

void loop(){
  byte threshold;
  int adc_value = analogRead(adc_pin);
  byte adc_byte = adc_value >> 2;
  if(calibration_counter >= CALIBRATION_SIZE){
    threshold = findThreshold();
    initializing = false;
  }
  if(initializing){
    calibrate(adc_byte);
    calibration_counter++;
  }
  else{
    processInput(adc_byte, threshold);
  }

}

void processInput(byte adc_byte, byte threshold){
  boolean input_bool;
  input_bool = (adc_byte < threshold) ? 1 : 0;
  switch(bias_removal){
  case VON_NEUMANN:
    vonNeumann(input_bool); 
    break;
  case EXCLUSIVE_OR:
    exclusiveOr(input_bool);
    break;
  case NO_BIAS_REMOVAL:
    buildByte(input_bool);
    break;
  }
}

void exclusiveOr(byte input){
  static boolean flip_flop = 0;
  flip_flop = !flip_flop;
  buildByte(flip_flop ^ input);
}

void vonNeumann(byte input){
  static int count = 1;
  static boolean previous = 0;
  static boolean flip_flop = 0;

  flip_flop = !flip_flop;

  if(flip_flop){
    if(input == 1 && previous == 0){
      buildByte(0);
    }
    else if (input == 0 && previous == 1){
      buildByte(1); 
    }
  }
  previous = input;
}

void buildByte(boolean input){
  static int sync_counter = 0;
  static int byte_counter = 0;
  static byte out = 0;

  if (input == 1){
    out = (out << 1) | 0x01;
  }
  else{
    out = (out << 1); 
  }
  byte_counter++;
  byte_counter %= 8;
  if(byte_counter == 0){
    if (output_format == ASCII_BYTE)
    {
#if DATA_MODE != SERIAL_ONLY      
      file.print(',');    
      file.print(out, DEC);
#endif

#if DATA_MODE != SD_ONLY
      Serial.println(out, DEC);
#endif
    }

    if (output_format == BINARY)
    { 
#if DATA_MODE != SERIAL_ONLY
      file.write(out);
#endif

#if DATA_MODE != SD_ONLY
      Serial.write(out);
#endif
    }
    out = 0;  
    sync_counter++;
  }

  if (output_format == ASCII_BOOL)
  {
#if DATA_MODE != SERIAL_ONLY
    file.print(',');    
    file.print(input, DEC);
#endif

#if DATA_MODE != SD_ONLY
    Serial.print(input, DEC);
#endif
  }

#if DATA_MODE != SD_ONLY
  Serial.flush();
#endif

#if DATA_MODE != SERIAL_ONLY
  if (sync_counter >= 2048)
  {
    file.flush();
    blinkLed();
    delay(50);
    blinkLed();
    sync_counter = 0;
  }
#endif
}


void calibrate(byte adc_byte){
  bins[adc_byte]++;  
  printStatus();
}

unsigned int findThreshold(){
  unsigned long half;
  unsigned long total = 0;
  int i;

  for(i=0; i < BINS_SIZE; i++){
    total += bins[i];
  }	

  half = total >> 1;
  total = 0;
  for(i=0; i < BINS_SIZE; i++){
    total += bins[i];
    if(total > half){
      break;
    }	
  }
  return i;
}

//Blinks an LED after each 10th of the calibration completes
void printStatus(){
  unsigned int increment = CALIBRATION_SIZE / 10;
  static unsigned int num_increments = 0; //progress units so far
  unsigned int threshold;

  threshold = (num_increments + 1) * increment;
  if(calibration_counter > threshold){
    num_increments++;
#if DEBUG_MODE == SERIAL_DEBUG
    Serial.print("*");
#endif
    blinkLed();
  }   
}

void blinkLedDelay(int time)
{
  digitalWrite(led_pin, HIGH);
  delay(time);
  digitalWrite(led_pin, LOW);
}

void blinkLed(){
  digitalWrite(led_pin, HIGH);
  delay(30);
  digitalWrite(led_pin, LOW);
}














