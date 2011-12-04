/********************************/
/*  Rob Seward 2008-2009        */
/*  Derek Chafin 2010-2011      */
/*  v1.5                        */
/*  4/21/2011                   */
/********************************/

#define USE_SERIAL       0
#define ECHO_TO_SERIAL   0 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()

#define BINS_SIZE 256
#define CALIBRATION_SIZE 50000

#define NO_BIAS_REMOVAL 0
#define EXCLUSIVE_OR 1
#define VON_NEUMANN 2

#define ASCII_BYTE 0
#define BINARY 1
#define ASCII_BOOL 2

#include <SD.h>

/***  Configure the RNG **************/
int bias_removal = VON_NEUMANN;
int output_format = BINARY;
int baud_rate = 19200;
/*************************************/

const int chipSelect = 4;
const int adc_pin = 8;
const int led_pin = 13;
unsigned int bins[BINS_SIZE];
boolean initializing = true;
unsigned int calibration_counter = 0;
char* name = "RANDOM.DAT";
File file;

void setup(){
  pinMode(led_pin, OUTPUT);
#if USE_SERIAL
  Serial.begin(baud_rate);
#if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START
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
#if USE_SERIAL
    Serial.println("Card failed, or not present");
#endif
    // don't do anything more:
    return;
  }
  file = SD.open(name, FILE_WRITE);
  file.seek(file.size());

#if USE_SERIAL  
  Serial.print("Logging to: ");
  Serial.println(name);
  Serial.print("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(FreeRam());
#endif
  for (int i=0; i < BINS_SIZE; i++){
    bins[i] = 0; 
  }
#if USE_SERIAL
  Serial.println("BINS Done");
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
      file.print(',');    
      file.print(out, DEC);
      
#if USE_SERIAL
#if ECHO_TO_SERIAL
      Serial.println(out, DEC);
#endif //ECHO_TO_SERIAL
#endif
    }

    if (output_format == BINARY)
    { 
      file.print(out, BYTE);
#if USE_SERIAL
#if ECHO_TO_SERIAL
      Serial.print(out, BYTE);
#endif //ECHO_TO_SERIAL
#endif
    }
    out = 0;  
    sync_counter++;
  }

  if (output_format == ASCII_BOOL)
  { 
    file.print(',');    
    file.print(input, DEC);
#if USE_SERIAL
#if ECHO_TO_SERIAL
    Serial.print(input, DEC);
#endif //ECHO_TO_SERIAL
#endif
  }
  if (sync_counter >= 2048)
  {
    file.flush();
    blinkLed();
    delay(50);
    blinkLed();
    sync_counter = 0;
  }

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
    //Serial.print("*");
    blinkLed();
  }   
}

void blinkLed(){
  digitalWrite(led_pin, HIGH);
  delay(30);
  digitalWrite(led_pin, LOW);
}














