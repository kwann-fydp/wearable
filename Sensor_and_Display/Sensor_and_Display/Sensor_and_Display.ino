// display libraries
#include <Arduino.h>
#include <U8g2lib.h>

#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Wire.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

// Constructor to initialize the display 
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Arduino
//U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 16, /* data=*/ 17, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, pure SW emulated I2C
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 16, /* data=*/ 17);   // ESP32 Thing, HW I2C with pin remapping
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 17, /* data=*/ 16, /* reset=*/ U8X8_PIN_NONE);    // Custom

// display constants
#define iw 12 // icon width
#define ih 12 // icon height
#define ms 4 // margin spacing
#define sw 128 // screen width
#define sh 64 // screen height

// wifi icon
static const unsigned char wifi_bits[] PROGMEM = {
  0xf0, 0x00, 0xfc, 0x03, 0x0e, 0x07, 0x06, 0x06, 0x03, 0x0c, 0xf1, 0x08,
  0xf9, 0x09, 0x0d, 0x0b, 0x04, 0x02, 0x04, 0x02, 0x60, 0x00, 0x60, 0x00  };

// full battery icon
static const unsigned char batF_bits[] PROGMEM = {
  0xf0, 0x00, 0x9c, 0x03, 0xf4, 0x02, 0xf4, 0x02, 0x04, 0x02, 0xf4, 0x02,
   0xf4, 0x02, 0x04, 0x02, 0xf4, 0x02, 0xf4, 0x02, 0xfc, 0x03, 0xfc, 0x03   };

// 66% battery icon
static const unsigned char bat66_bits[] PROGMEM = {
  0xf0, 0x00, 0x9c, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xf4, 0x02,
   0xf4, 0x02, 0x04, 0x02, 0xf4, 0x02, 0xf4, 0x02, 0xfc, 0x03, 0xfc, 0x03  };

// 33% battery icon
static const unsigned char bat33_bits[] PROGMEM = {
  0xf0, 0x00, 0x9c, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0xf4, 0x02, 0xf4, 0x02, 0xfc, 0x03, 0xfc, 0x03  };

// empty battery icon
static const unsigned char batE_bits[] PROGMEM = {
  0xf0, 0x00, 0x9c, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xfc, 0x03, 0xfc, 0x03  };

// bluetooth icon
static const unsigned char bt_bits[] PROGMEM = {
   0x60, 0x00, 0xe0, 0x00, 0xa0, 0x01, 0x24, 0x03, 0xa8, 0x01, 0xf0, 0x00,
   0xf0, 0x00, 0xa8, 0x01, 0x24, 0x03, 0xa0, 0x01, 0xe0, 0x00, 0x60, 0x00 };


static const unsigned char alert_bits[] PROGMEM = {
   0xf8, 0xff, 0xff, 0xff, 0x01, 0xfc, 0xff, 0xff, 0xff, 0x03, 0xfe, 0xff,
   0xff, 0xff, 0x07, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff,
   0xff, 0xf1, 0xff, 0x0f, 0xff, 0xff, 0xe0, 0xff, 0x0f, 0xff, 0x7f, 0xe0,
   0xff, 0x0f, 0xff, 0x7f, 0xc0, 0xff, 0x0f, 0xff, 0x3f, 0xc0, 0xff, 0x0f,
   0xff, 0x3f, 0x80, 0xff, 0x0f, 0xff, 0x1f, 0x80, 0xff, 0x0f, 0xff, 0x1f,
   0x00, 0xff, 0x0f, 0xff, 0x0f, 0x06, 0xfe, 0x0f, 0xff, 0x07, 0x06, 0xfe,
   0x0f, 0xff, 0x07, 0x06, 0xfc, 0x0f, 0xff, 0x03, 0x06, 0xfc, 0x0f, 0xff,
   0x03, 0x06, 0xf8, 0x0f, 0xff, 0x01, 0x06, 0xf8, 0x0f, 0xff, 0x01, 0x00,
   0xf0, 0x0f, 0xff, 0x00, 0x00, 0xe0, 0x0f, 0x7f, 0x00, 0x06, 0xe0, 0x0f,
   0x7f, 0x00, 0x06, 0xc0, 0x0f, 0x3f, 0x00, 0x00, 0xc0, 0x0f, 0x3f, 0x00,
   0x00, 0x80, 0x0f, 0x3f, 0x00, 0x00, 0x80, 0x0f, 0x7f, 0x00, 0x00, 0xc0,
   0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff,
   0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xfe, 0xff, 0xff,
   0xff, 0x07, 0xfc, 0xff, 0xff, 0xff, 0x03, 0xf8, 0xff, 0xff, 0xff, 0x01 };


// global state variables
int batLvl = 0;
int i; // counter for loop
bool conWifi = true;
bool conBT = true;
// String RoomNumber = "Room 212"; this is NOT how to do strings in C... (I forgot lol)
// String StatusMessage = "Measuring Oxygen";

bool displayAlert = false;
long displayAlertToggleTime;


MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t chunk_size;
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

byte pulseLED = 11; //Must be on PWM pin
byte readLED = 13; //Blinks with each data read




// get functions for device
int getBatteryLevel(void) {
  // insert battery logic
  return batLvl;
}

bool getWifiStatus(void) {
  // insert Wi-Fi logic
  return conWifi;
}

bool getBTStatus(void) {
  // insert Bluetooth logic
  return conBT;
}

void drawBattery(void) {
  // draw battery icon right now doing a 33% interval mapping with integers from 0 to 3.
  switch(getBatteryLevel()) {
    case 0:
      u8g2.drawXBMP( sw-iw-ms, ms, iw, ih, batF_bits);
      break;
    case 1:
      u8g2.drawXBMP( sw-iw-ms, ms, iw, ih, bat66_bits);
      break;
    case 2:
      u8g2.drawXBMP( sw-iw-ms, ms, iw, ih, bat33_bits);
      break;
    case 3:
      u8g2.drawXBMP( sw-iw-ms, ms, iw, ih, batE_bits);
      break;
    default:
      u8g2.drawXBMP( sw-iw-ms, ms, iw, ih, batE_bits);
  }
}

// graphic commands to redraw the complete screen should be placed here
void draw(void) {
  
  drawBattery();

  if (getWifiStatus()) {
    u8g2.drawXBMP( sw-2*iw-2*ms, ms, iw, ih, wifi_bits);
  }

  if (getBTStatus()) {
    u8g2.drawXBMP( sw-3*iw-3*ms, ms, iw, ih, bt_bits);
  }

  // draw Room number in top left
  u8g2.setFont(u8g2_font_lastapprenticebold_tr);
  u8g2.drawStr(ms,16,"Room 212");

  // draw Status message in bottom left
  u8g2.setFont(u8g2_font_logisoso16_tr);
  if (displayAlert) {
    u8g2.drawXBMP( ms, 24, 36, 36, alert_bits);
    u8g2.drawStr(ms*2 + 36 + ms,40,"Alerting");
    u8g2.drawStr(ms*2 + 36 + ms,60,"Nurse...");
  } else {
    u8g2.drawStr(ms,40,"Measuring");
    u8g2.drawStr(ms,60,"Oxygen...");
  }
}
 


void setup() {
  u8g2.begin();

  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  Serial.println(F("Attach sensor to finger with rubber band. Starting..."));
//  while (Serial.available() == 0) ; //wait until user presses a key
//  Serial.read();



/** CONFIG **/

// Default: brightness 60, pulse width 411, sample rate 100

  byte ledBrightness = 30; //Options: 0=Off to 255=50mA
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  int sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 69; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

}

void loop() {
  bufferLength = 100; // (sampleRate / sampleAverage) * 4 (4 seconds worth of data)
  chunk_size = bufferLength / 2;

  // **************************************************** //
  // ***************** INITIAL READINGS ***************** //
  // **************************************************** //
  
  //fill buffer with first 100 IR/Red samples, and determine the signal range
  for (int i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    Serial.print(i);
    Serial.print(": ");
    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
  }

  Serial.println("\nSTARTING LOOP");
//
//  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
//  int read_code = maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
//
//  switch(read_code) {
//    case 0:
//      Serial.println("\nSpO2 read valid");
//      break;
//    
//    case 1:
//      Serial.println("\nInvalid SpO2; valley index out of range");
//      break;
//
//    case 2:
//      Serial.println("\nInvalid SpO2; an_ratio out of range");
//      break;
//
//    default:
//      Serial.print("\nn_ratio_average: ");
//      Serial.println(read_code);
//      break;
//  }
//  
//  Serial.print(F(", SPO2="));
//  Serial.print(spo2, DEC);
//
//  Serial.print(F(", SPO2Valid="));
//  Serial.println(validSPO2, DEC);
//  Serial.println("\n");




  // **************************************************** //
  // ***************** HEADS-DOWN LOOP ****************** //
  // **************************************************** //

  // Draw on first pass
  long lastDraw = millis() - 1001;

  // Don't alert on first pass
  displayAlertToggleTime = millis();

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    // Temporary alert display setup - toggle btwn measure & alert every 15 seconds
    if (millis() - displayAlertToggleTime > 15000) {
      displayAlert = !displayAlert;
      displayAlertToggleTime = millis();
    }
    
    // Update screen every second
    if (millis() - lastDraw > 1000) {
      // picture loop
      u8g2.firstPage();
      do {
       draw();
      } while( u8g2.nextPage() );
      lastDraw = millis();
    }
      
    //dump the first (chunk_size) sets of samples in the memory and shift other samples to the top
    for (int i = chunk_size; i < bufferLength; i++)
    {
      redBuffer[i - chunk_size] = redBuffer[i];
      irBuffer[i - chunk_size] = irBuffer[i];
    }

    //take chunk_size samples before calculating the heart rate.
    for (int i = bufferLength-chunk_size; i < bufferLength; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

//      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
//      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
//      Serial.print(F(", ir="));
      Serial.print(",");
      Serial.println(irBuffer[i], DEC);
//
//      Serial.print(F(", HR="));
//      Serial.print(heartRate, DEC);
//
//      Serial.print(F(", HRvalid="));
//      Serial.print(validHeartRate, DEC);
//
//      Serial.print(F(", SPO2="));
//      Serial.print(spo2, DEC);
//
//      Serial.print(F(", SPO2Valid="));
//      Serial.println(validSPO2, DEC);
    }




    // **************************************************** //
    // ************ HEADS-UP CHECKS & UPDATES ************* //
    // **************************************************** //

    //After gathering 25 new samples recalculate HR and SP02
//    int read_code = maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

//    switch(read_code) {
//      case 0:
//        Serial.println("\nSpO2 read valid");
//        break;
//      
//      case 1:
//        Serial.println("\nInvalid SpO2; valley index out of range");
//        break;
//  
//      case 2:
//        Serial.println("\nInvalid SpO2; an_ratio out of range");
//        break;
//  
//      default:
//        Serial.print("\nn_ratio_avg: ");
//        Serial.println(read_code);
//        break;
//    }
    
//    Serial.print(F(", SPO2="));
//    Serial.print(spo2, DEC);
//
//    Serial.print(F(", SPO2Valid="));
//    Serial.println(validSPO2, DEC);
  }

}
