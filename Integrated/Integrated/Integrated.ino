/*
  Wearable SpO2 monitor for overdose event detection & alerting
  By: KWANN (Team 12, UW Biomedical Engineering Capstone 2021)
  Date: March 19, 2021
*/


/*
  ~~ PINOUT ~~
  Power (All): +3.3V
  SpO2 Sensor:
    - SCL: 22
    - SDA: 21
  LCD:
    - SCL: 17
    - SDA: 16
 */


#include <Wire.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "WiFi.h"
#include "ESP32Ping.h"

#include "Icons.h"
#include "spo2_data.h"
#include "classifier.h"


/**********************************/
/******** CONFIG VARIABLES ********/
/**********************************/

bool use_simulated_data = true; // Look for .csv files of SpO2 values instead of sensor
int simulation_read_delay = 250; // ms
int simulation_length = 152; // length of data buffer

bool static_sleep_mode = false; // TESTING: set this to true to have static sleep mode
int wifi_connect_wait_time = 10; // seconds

const char* backend_url = "https://limitless-thicket-44970.herokuapp.com";
const int backend_http_port = 80;


/**********************************/
/******** STATE VARIABLES *********/
/**********************************/

// functional state
bool sleep_mode = false;
bool config_mode = false;
bool manual_alert_active = false;
bool measured_alert_active = false;

// device status
int batt_level = 0;
bool bluetooth_connection = false;
int device_id = 1; // TODO: should be set via configuration workflow

// display state
bool display_alert = false;

// WIFI
bool wifi_connection = false;

// CHANGE
const char* ssid = "";
const char* password = "";



/**********************************/
/******* SPO2 SENSOR SETUP ********/
/**********************************/

MAX30105 particleSensor;

// Sensor Parameters //
// (Default: brightness 60, pulse width 411, sample rate 100, adcRange 4096)
byte ledBrightness = 60; //Options: 0=Off to 255=50mA
byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
int sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
int pulseWidth = 69; //Options: 69, 118, 215, 411
int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

// Data structs //
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

int32_t bufferLength = 100; // (sampleRate / sampleAverage) * 4 (hold 4 seconds of data)
int32_t chunk_size = bufferLength / 2;



/**********************************/
/*********** LCD SETUP ************/
/**********************************/

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(
  U8G2_R0,
  /* clock=*/ 17, // ESP port 17
  /* data=*/ 16, // ESP port 16
  /* reset=*/ U8X8_PIN_NONE
);

// display constants
#define iw 12 // icon width
#define ih 12 // icon height
#define ms 4 // margin spacing
#define sw 128 // screen width
#define sh 64 // screen height


/**********************************/
/*********** FUNCTIONS ************/
/**********************************/

void drawBattery(void) {
  // draw battery icon right now doing a 33% interval mapping with integers from 0 to 3.
  switch(batt_level) {
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

  if (wifi_connection) {
    u8g2.drawXBMP( sw-2*iw-2*ms, ms, iw, ih, wifi_bits);
  }

  if (bluetooth_connection) {
    u8g2.drawXBMP( sw-3*iw-3*ms, ms, iw, ih, bt_bits);
  }

  // draw Room number in top left
  u8g2.setFont(u8g2_font_lastapprenticebold_tr);
  u8g2.drawStr(ms,16,"Room 212");

  // draw Status message in bottom left
  u8g2.setFont(u8g2_font_logisoso16_tr);
  if (manual_alert_active || measured_alert_active) {
    u8g2.drawXBMP( ms, 24, 36, 36, alert_bits);
    u8g2.drawStr(ms*2 + 36 + ms,40,"Alerting");
    u8g2.drawStr(ms*2 + 36 + ms,60,"Nurse...");
  } else {
    u8g2.drawStr(ms,40,"Measuring");
    u8g2.drawStr(ms,60,"Oxygen...");
  }
}
 


void setup() {
  /* Debug */
  Serial.begin(115200);

  /* Check power switch position & set sleep_mode appropriately */
  // TODO


  /* Set up sensor */

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }
  
  //Configure sensor with these parameters
  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );


  /* Set up WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int counter = 0;
  int num_attempts = wifi_connect_wait_time * 2;
  while (WiFi.status() != WL_CONNECTED && counter++ < num_attempts) {
      delay(500);
      Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connection = true;
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

//    // PING TEST
//    bool lcl_ping_success = Ping.ping("192.168.22.146", 5); // local device on network
//    if(!lcl_ping_success){
//        Serial.println("Local ping failed.");
//    } else {
//      Serial.println("Local ping succesful.");
//    }
//  
//    bool remote_ping_success = Ping.ping("www.google.com", 5); //  remote host
//    if(!remote_ping_success){
//        Serial.println("Remote ping failed.");
//    } else {
//      Serial.println("Remote ping succesful.");
//    }
    
  } else {
    Serial.println("WiFi Connection failed");
  }

  /* Set up LCD */
  u8g2.begin();
}



void loop() {

  if (sleep_mode) {
    // TODO: light sleep with wakeup from power switch
  } else if (static_sleep_mode) {
    esp_light_sleep_start(); // TESTING ONLY: this will KO the ESP until reset!
  }

  if (config_mode) {
    // TODO: config loop
  }

  if (manual_alert_active) {
    // TODO: dispatch manual alert & update screen
  }

  // TODO: if LED buffers empty, fill them

  // Draw on first pass
  long lastDraw = millis() - 1001;
  bool forceDraw = false;

  // Measurement loop
  bool break_cmd = false;
  int sim_data_index = 0;
  
  long spo2_measurement_time = 0;
  long classification_time = 0;
  long alert_dispatch_time = 0;
  
  while (true) {
    // If LCD refresh interval reached, update LCD
    if (millis() - lastDraw > 1000 || forceDraw) {
      // picture loop
      u8g2.firstPage();
      do {
       draw();
      } while( u8g2.nextPage() );
      lastDraw = millis();
      forceDraw = false;
    }

    if (use_simulated_data) {
      // Read one SpO2 value from file after simulated measurement period
      delay(simulation_read_delay); // TODO: Make interruptable
      
      if (sim_data_index == simulation_length) {
        Serial.println("\n\n\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        Serial.println("START OF SIMULATED DATASET");
        sim_data_index = 0;
      }
      spo2 = no_overdose_outliers[sim_data_index++];
      spo2_measurement_time = millis();
    } else {
      // Measure batch of sensor values & calculate SpO2
      // TODO
    }

    // Feed batch SpO2 value into classification algorithm
    bool OD_detected = classify_spo2(spo2);
    classification_time = millis();

    // If classification is positive, dispatch measured alert & update screen
    if (OD_detected) {
      // Take action once
      if (!measured_alert_active) {
        measured_alert_active = true;
        forceDraw = true;

        // Send alert to backend
        // Create WiFi client
        WiFiClient client;
        if (wifi_connection) {
          if (!client.connect(backend_url, backend_http_port)) {
            Serial.println("=> Connection to backend failed");
          } else {
            Serial.println("Connected to backend");
          }
        }

        String url = "/api/alert/";
        url += device_id;

        // Code pulled from WiFiClient demo 
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + backend_url + "\r\n" +
                     "Connection: close\r\n\r\n");
        unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 10000) {
                Serial.println(">>> Client Timeout !");
                client.stop();
                return;
            }
        }
    
        // Read all the lines of the reply from server and print them to Serial
        while(client.available()) {
            String line = client.readStringUntil('\r');
            Serial.print(line);
        }
        
        alert_dispatch_time = millis();

        // Time summary of alert
        Serial.println("\n~~~ TIME SUMMARY OF ALERT ~~~");
        Serial.print("  -> Measurement to Classification: ");
        Serial.println(classification_time - spo2_measurement_time);
        Serial.print("  -> Classification to dispatch: ");
        Serial.println(alert_dispatch_time - classification_time);
        
      }
      
      Serial.print("SpO2 read: ");
      Serial.print(spo2);
      Serial.println(" - OD detected");
      
    } else {
      Serial.print("SpO2 read: ");
      Serial.print(spo2);
      Serial.println(" - No OD detected");
    }
  }
}
