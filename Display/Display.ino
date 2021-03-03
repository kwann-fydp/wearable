// display libraries
#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Constructor to initialize the display 
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Arduino
//U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 16, /* data=*/ 17, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, pure SW emulated I2C
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 16, /* data=*/ 17);   // ESP32 Thing, HW I2C with pin remapping

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


// global state variables
int batLvl = 0;
int i; // counter for loop
bool conWifi = true;
bool conBT = true;
// String RoomNumber = "Room 212"; this is NOT how to do strings in C... (I forgot lol)
// String StatusMessage = "Measuring Oxygen";

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
	u8g2.drawStr(ms,40,"Measuring");
	u8g2.drawStr(ms,60,"Oxygen...");
}
 
void setup(void) {
	u8g2.begin();
}
 
void loop(void) {
	// picture loop
	u8g2.firstPage();
	do {
	 draw();
	} while( u8g2.nextPage() );

	// rebuild the picture after some delay
	delay(1000);
}