/*
  Pet Health
  Oliver Koeth, NTT DATA

  Firmware for the Arduino based prototype for a pet health gadget.
  Provides the following funtions:
  - Tracks sleep movements (for demo: a button starts / stops the sleep)
  - Tracks temperature
  - Write all data to SD card
  - Provides data to base station (sync mode enabled by button)
  - Outputs: Green LED indicates sleep, red LED indicates sync mode

  For SD Card, the following PINs are used
  - MOSI - pin 11
  - MISO - pin 12
  - CLK  - pin 13
  - CS   - pin 4
 */

#include <SD.h>

// Pin layout 
const int PIN_BUTTON_SYNC  = 7;
const int PIN_BUTTON_SLEEP = 8;
const int PIN_LED_SYNC = 5;
const int PIN_LED_SLEEP = 6;

const int PIN_CHIPSELECT = 4;

const int MODE_NONE = 0;
const int MODE_SYNC = 1;
const int MODE_SLEEP = 2;

const int REST = 500;

int mode = MODE_NONE;
int eventID = 0;
boolean statusSerial = false;
boolean statusSD = false;

// Setup
void setup() {                
	// initialize the digital pins
	pinMode(PIN_BUTTON_SYNC, INPUT);     
	pinMode(PIN_BUTTON_SLEEP, INPUT);     
	pinMode(PIN_LED_SYNC, OUTPUT);     
	pinMode(PIN_LED_SLEEP, OUTPUT);

	// Initialise serial communication
	Serial.begin(9600);
	statusSerial = true;

	// Required for SD card
	pinMode(10, OUTPUT);
	Serial.println("<log message='Initializing SD card...'/>");

	// Initialise SD Card
	if (!SD.begin(PIN_CHIPSELECT)) {
		Serial.println("<log message='SD card failes or not present...'/>");
	} else {
		Serial.println("<log message='SD card present...'/>");
		statusSD = true;
	}
}

// Log data record
void logData (String message) {
	File dataFile = SD.open("LOG.TXT", FILE_WRITE);

	if (dataFile) {
		dataFile.println(message);
		dataFile.close();
	}  
	// if the file isn't open, pop up an error:
	else {
		Serial.println("<log message='Error opening LOG.TXT...'/>");
	}
} 

// Process sync button
int prevSyncState = LOW;
void processSyncButton() {
	// Only one state at a time!
	if (mode == MODE_NONE || mode == MODE_SYNC) {		
		int syncState = digitalRead(PIN_BUTTON_SYNC);
		if (prevSyncState == LOW && syncState == HIGH) {
			if (mode == MODE_NONE) {
				mode = MODE_SYNC;
				digitalWrite(PIN_LED_SYNC, HIGH);
				logData("LOG: Sync button was pressed, sync mode enabled");
			} else if (mode == MODE_SYNC) {
				mode = MODE_NONE;
				digitalWrite(PIN_LED_SYNC, LOW);
				logData("LOG: Sync button was pressed, sync mode disabled");
			}
			// Wait for button to come to rest again
			delay (REST);
		}
	}

}

// Process sleep button
int prevSleepState = LOW;
void processSleepButton() {
	// Only one state at a time!
	if (mode == MODE_NONE || mode == MODE_SLEEP) {		
		int sleepState = digitalRead(PIN_BUTTON_SLEEP);
		if (prevSleepState == LOW && sleepState == HIGH) {
			if (mode == MODE_NONE) {
				mode = MODE_SLEEP;
				digitalWrite(PIN_LED_SLEEP, HIGH);
				logData("LOG: Sleep button was pressed, sleep mode enabled");
			} else if (mode == MODE_SLEEP) {
				mode = MODE_NONE;
				digitalWrite(PIN_LED_SLEEP, LOW);
				logData("LOG: Sleep button was pressed, sleep mode disabled");
			}
			// Wait for button to come to rest again
			delay (REST);
		}
	}

}

// Perform sleep processing
void idle() {
	processSyncButton();
	processSleepButton();
}

// Perform sleep processing
void sync() {
	processSyncButton();
}

// Perform sleep processing
void sleep() {
	processSleepButton();
}

// Main loop
void loop() {
	switch (mode) {
		case MODE_NONE:
			idle();
			break;
		case MODE_SYNC:
			sync();
			break;
		case MODE_SLEEP:
			sleep();
			break;
	}
}
