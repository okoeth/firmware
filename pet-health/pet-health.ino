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

  For the MPU 6050, the following PINs are used
  - SDA  - pin A4
  - SCL  - pin A5

  For input / output the following PINs are used
  - Button Sync   - pin 7
  - Button Sleep  - pin 8
  - LED Sync      - pin 5
  - LED Sleep     - pin 6
  - LED Active    - pin 3
 
 */

//#include <SD.h>
#include <Wire.h>

// Pin layout 
const int PIN_BUTTON_SYNC  = 7;
const int PIN_BUTTON_SLEEP = 8;
const int PIN_LED_SYNC = 5;
const int PIN_LED_SLEEP = 6;
const int PIN_LED_ACTIVE = 3;

const int PIN_CHIPSELECT = 4;

// Modes
const int MODE_NONE = 0;
const int MODE_SYNC = 1;
const int MODE_SLEEP = 2;

// I2C addresses
const int MPU_ADDRESS=0x68;

// Other constants
const int REST = 500;

char * LOG_FILE="LOG.TXT";

int mode = MODE_NONE;
int eventID = 0;

boolean statusSerial = false;
boolean statusSD = false;

// Initialize the digital pins
void initializePins() {
	pinMode(PIN_BUTTON_SYNC, INPUT);     
	pinMode(PIN_BUTTON_SLEEP, INPUT);     
	pinMode(PIN_LED_SYNC, OUTPUT);     
	pinMode(PIN_LED_SLEEP, OUTPUT);
	pinMode(PIN_LED_ACTIVE, OUTPUT);	

	analogWrite(PIN_LED_ACTIVE, 0);
}

// Initialize the serial communication
void initializeSerial() {
	// Initialise serial communication
	Serial.begin(9600);
	statusSerial = true;
}

/*
// Initialize the SD card
void initializeSDcard() {
	// Required for SD card
	pinMode(10, OUTPUT);
	Serial.println("<log message='Initializing SD card...'/>");

	// Initialise SD Card
	if (!SD.begin(PIN_CHIPSELECT)) {
		Serial.println("<log message='SD card failes or not present...'/>");
	} else {
		Serial.println("<log message='SD card present...'/>");
		statusSD = true;
		logData("LOG: SD card initialised");
	}
}
*/

// Initialize the MPU
void initializeMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

// Setup
void setup() {                
	initializePins();
	initializeSerial();
	//initializeSDcard();
	initializeMPU();
}

/*
// Log data record USE_SD = true
void logData (String message) {	
	File dataFile = SD.open(LOG_FILE, FILE_WRITE);

	if (dataFile) {
		dataFile.println(message);
		Serial.println("<log message='Writing message to SD card'/>");
		dataFile.close();
	}  
	// if the file isn't open, pop up an error:
	else {
		Serial.println("<log message='Error opening LOG.TXT...'/>");
		Serial.println(message);
	}
} 
*/

// Log data record USE_SD = fales
void logData (String message) {	
	String line = "<log message='";
	line += message + "'/>";
	Serial.println(line);
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
int AcStep = 0;
int AcCount = 0;
long AcTotal = 0;
void processSleepButton() {
	// Only one state at a time!
	if (mode == MODE_NONE || mode == MODE_SLEEP) {		
		int sleepState = digitalRead(PIN_BUTTON_SLEEP);
		if (prevSleepState == LOW && sleepState == HIGH) {
			if (mode == MODE_NONE) {
				mode = MODE_SLEEP;
				AcStep = 0;
				AcCount = 0;
				AcTotal = 0;
				digitalWrite(PIN_LED_SLEEP, HIGH);
				logData("LOG: Sleep button was pressed, sleep mode enabled");
			} else if (mode == MODE_SLEEP) {
				mode = MODE_NONE;
				digitalWrite(PIN_LED_SLEEP, LOW);
				analogWrite(PIN_LED_ACTIVE, 0);
				logData("LOG: Sleep button was pressed, sleep mode disabled");
				logData(summariseSleepData());
			}
			// Wait for button to come to rest again
			delay (REST);
		}
	}

}

// Summarise sleep data
String summariseSleepData() {
	String sleepData = "SLEEP: duration=";
	sleepData+=AcCount;
	sleepData+=", activity=";
	sleepData+=AcTotal;
	return sleepData;
}

// Process sleep data
void processSleepData() {
	int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

	// Initialise read from MPU
	Wire.beginTransmission(MPU_ADDRESS);
	Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
	Wire.endTransmission(false);
	Wire.requestFrom(MPU_ADDRESS,14,true);  // request a total of 14 registers

	// Perform read from MPU
	AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
	AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
	AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
	Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
	GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
	GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
	GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

	// Normalise data
	AcX = AcX / 100;
	AcY = AcY / 100;
	if (AcX < 0) AcX = AcX * -1;
	if (AcY < 0) AcY = AcY * -1;
	AcCount+=1;
	AcStep=(AcX+AcY)/2;
	AcTotal+=AcStep;

	// Set activity LED
	analogWrite(PIN_LED_ACTIVE, AcStep);
	delay(REST);
}

/*
// Read SD Card data and send line by line: USE_SD=true
void sendData() {
	Serial.println("<log message='Sending SD card data'/>");
	File dataFile = SD.open(LOG_FILE, FILE_READ);

	if (dataFile) {
		Serial.println("<cmd name='data'>");
		int data;
		while ((data = dataFile.read())>0) {
			Serial.write(data);
		}
		Serial.println("</cmd>");
		dataFile.close();
	}  
	// if the file isn't open, pop up an error:
	else {
		Serial.println("<log message='Error opening LOG.TXT...'/>");
	}
}
*/

// Read SD Card data and send line by line: USE_SD=true
void sendData() {
	Serial.println("<log message='Sending sleep data'/>");
	Serial.println("<cmd name='data'>");
	Serial.println(summariseSleepData());
	Serial.println("</cmd>");
}

/*
// Reset SD Card data
void resetDevice() {
	Serial.println("<log message='Reset SD card data'/>");
	if (!SD.remove (LOG_FILE)) {
		Serial.println("<log message='Error deleting LOG.TXT...'/>");
	}
}
*/

// Reset SD Card data - dummy
void resetDevice() {
}

// Perform sleep processing
void idle() {
	processSyncButton();
	processSleepButton();
}

// Perform sleep processing
void sync() {
	processSyncButton();
  	if (Serial.available() > 0) {
	    int character = Serial.read();
	    switch (character) {
	      // Read data
	      case 'D':
	        Serial.println("<log message='Received: Read data'>");
	        sendData();
	        break;
	      // Reset device
	      case 'R':
	        Serial.println("<log message='Received: Reset device'>");
	        resetDevice();
	        break;
	    }
	}
}

// Perform sleep processing
void sleep() {
	processSleepButton();
	processSleepData();
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
