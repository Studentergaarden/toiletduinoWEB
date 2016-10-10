/* -*- coding: utf-8 -*- */

/*
  Testing:
  Setup netcat to listen to the relevant (TCP) port on the server.
  In this case:
  $ nc -l -vv -p 555

  In the case of a gateway
  $ nc -l -vv -p 5555 -g 172.16.0.1
  $ nc -l -k --broker 5555

  Connect to the server with:
  $ nc loki 5555

  Client sending:
  id=t1&type=state&state=1
  id=t2&type=log&ms=29561

  Server sending:
  log id=t1 time=123 avg=987 visits=30

  See more netcat examples here:
  http://www.g-loaded.eu/2006/11/06/netcat-a-couple-of-useful-examples/

  Useful commands to see which ports are open:
  $ netstat -lnptu | grep LISTEN  (viser hvilke services der kører)
  $ nmap localhost
  $ lsof | grep TCP | grep lem
  Get additional info with
  $ lsof -p #pid

  program arduino over ethernet shield tftp
  http://forum.arduino.cc/index.php?topic=267003.0
  https://github.com/codebendercc/Ariadne-Bootloader
  http://www.freetronics.com/pages/how-to-upload-a-sketch-to-your-arduino-via-a-network

*/

/* macro for bool busy/free */
#define FREE (1 != 1)
#define BUSY (!FREE)

#define __LCD
/* #define __SERIAL */
/* #define DEBUG */

#include <SPI.h>
#include <Ethernet.h>

#ifdef __LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#endif

// For debugging
//#include <MemoryFree.h>
//#include <pgmStrToRAM.h>

typedef struct {
	unsigned long start, duration;
	bool busy, ledState;
	int sensorVal; // analog readings
	float val; // scaled vals
	byte pin;
	byte ledPin;
	unsigned long previousMillis, currentMillis;
	const char* id;
} sensor_t;

sensor_t t1, t2, b1, b2, b3;
void setupSensor(sensor_t *t, const char *id, byte SensorPin, byte ledPin);
void checkSensor(sensor_t *t);
void blink(byte ledPin, int times);
/* http://arduino.stackexchange.com/a/1478 */
void(* resetFunc) (void) = 0; //declare reset function @ address 0
/* Overloading or template */
/* http://stackoverflow.com/questions/8627625/is-it-possible-to-make-function-that-will-accept-multiple-data-types-for-given-a */
void sendServer(const char *id, bool state);
void sendServer(const char *id,  unsigned long val);

#ifdef __LCD
void readServer();
void formatTime(unsigned long time,char *timeStr);
#endif



const unsigned long BLINKINTERVAL = 1000; // ms
const unsigned long MIN_DURATION = 15000; // ms

/* The MAC-adress have to be unique for the network */
/* Just increment the MAC-address from the end, eg: the next device could be
 * 0xDE 0xAD 0xBE 0xEF 0xFE 0xEE, for example. Then your third would end in
 * 0xEF, the fourth in 0xF0, and so on. */

// deviceID is used for creating mac-address. Must be unique.
const byte deviceID     = 10;
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, deviceID};
const int port = 5555;
// Enter the IP address of the server you're connecting to:
IPAddress server(10,42,0,1);
const char deviceName[] = "pharisæer_toilet";
EthernetClient client;

#ifdef __LCD
// create an lcd instance with correct constructor for how the lcd is wired to the I2C chip
// addr, EN, RW, RS, D4, D5, D6, D7, Backlight, POLARITY
// Find the addr from the address scanner in the README.org file.
LiquidCrystal_I2C  lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
#endif



#ifdef __SERIAL
Print *myPrint = &Serial; // Or assign software serial port address to it
Stream *myStream = &Serial;
#else
Print *myPrint = &client;
Stream *myStream = &client;
#endif

void setup() {
#ifdef __LCD
	lcd.begin(20,4);
	lcd.off();
	/* lcd.noBacklight(); */
#endif

	// start the Ethernet connection:
	Ethernet.begin(mac);
	//Ethernet.begin(mac, ip, gateway, gateway);
	// start the serial library:
	//Serial.begin(9600);
	// give the Ethernet shield a second to initialize:
	delay(1000);
	//Serial.println("connecting...");

	// if you get a connection, report back via serial:
	if (client.connect(server, port)) {
		myPrint->print("type=connected&id="), myPrint->println(deviceName);
		//Serial.print(deviceName), Serial.println(" connected.");
	}
	else {
		// if you didn't get a connection to the server:
		//Serial.println("connection failed");
	}

	/* Setup sensors */
	setupSensor(&t1,"t1",5,9); // struct, id, pin
	setupSensor(&t2,"t2",6,9);
	/* setupSensor(&b1,"b1",2,8); */
	/* setupSensor(&b2,"b2",3,8); */

	/*
	 * old setup(the correct, but the lcd is hardwired to the shower socket, and
	 * I dont really care enough about it, to also wire the toilet socket. Thus
	 * I just swapped them here, since the shower is not used anyway.)

	 setupSensor(&t1,"t1",2,8); // struct, id, pin, ledPin
	 setupSensor(&t2,"t2",3,8);

	 setupSensor(&b1,"b1",5,9);
	 setupSensor(&b2,"b2",6,9);
	 setupSensor(&b3,"b3",7,9);
	*/

	/* myPrint->print(F("Fre ram, setup: ")); */
	/* myPrint->println(freeMemory(), DEC); */

}

void loop()
{
	//delay(100);


	int attemps = 0;
	while (!client.connected()) {
		//Serial.print(attemps);
		//Serial.println("Creating connection...");

		// if connection is lost/disconnected, we might need to close the
		// connection and start it again!!
		if (attemps > 5){
			resetFunc(); // do a complete reset of the board
			attemps = 0;
		}
		client.connect(server, port);
		blink(t1.ledPin,6);
		attemps++;

		delay(500);
		if(client.connected())
			myPrint->print("type=connected&id="), myPrint->println(deviceName);
	}


	/* checkSensor(&b1); */
	/* checkSensor(&b2); */
	//  checkSensor(&b3);
	checkSensor(&t1);
	checkSensor(&t2);

#ifdef __LCD
	readServer();
#endif
}

void setupSensor(sensor_t *t, const char *id, byte SensorPin, byte ledPin){

	t->pin = SensorPin;
	t->ledPin = ledPin;
	t->val = 1;
	t->busy = false;
	t->previousMillis = 0;
	t->currentMillis = 0;
	t->ledState = LOW;
	t->id = id; // dynamic memory declaration
	pinMode(t->pin, INPUT_PULLUP);
	// digitalWrite(t->pin,HIGH); // not necessary with INPUT_PULLUP
	pinMode(t->ledPin, OUTPUT);
}

void checkSensor(sensor_t *t){
	/* set a flag and start timer when door is locked. door is not declared
	   'locked' until after a set interval */

	const float scaleb = 0.999;
	const float scalea = 0.001;

	t->sensorVal = digitalRead(t->pin);
	t->val = scalea*t->val + scaleb*t->sensorVal;


	/* blink if occupied */
	t->currentMillis = millis();
	if((t->busy == true) && (t->currentMillis - t->previousMillis > BLINKINTERVAL)) {
		// save the last time you blinked the LED and swap state
		t->previousMillis = t->currentMillis;
		t->ledState = !t->ledState;
		digitalWrite(t->ledPin, t->ledState);
	}

	if (t->val >= 0.999 && t->busy == true) {
		/* VACANT */
		t->busy = false;
		t->duration = millis() - t->start;

		/* ignore visits under 15 sec */
		if(t->duration > MIN_DURATION){
			/* send to server */

			sendServer(t->id,t->duration);
			blink(t->ledPin,3);
		}
		sendServer(t->id,FREE);

		/* Make sure to diable led */
		digitalWrite(t->ledPin, LOW);

#ifdef DEBUG
		Serial.print("toilet frit igen - ");
		Serial.print("Varighed[ms]: "), Serial.println(t->duration);
#endif

	} else if (t->val <= 0.0001 && t->busy == false) {
		/* OCCUPIED */
		t->busy = true;
		t->start = millis();

		sendServer(t->id,BUSY);

#ifdef DEBUG
		Serial.println("toilet optaget");
#endif
	}


	/* #ifdef DEBUG */
	/*   Serial.print("scaled: "), Serial.println(t->val); */
	/*   Serial.print("sensor: "), Serial.println(t->sensorVal); */
	/* #endif */

}


void blink(byte ledPin, int times){
	/* blink led the *stupid* way */
	for(int i=0;i<times;i++){
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
	}
}

void sendServer(const char *id, bool val){
	/* type can be state or log */
	myPrint->print("id=");
	myPrint->print(id);
	myPrint->print("&type=state&state=");
	myPrint->println(val);
}

void sendServer(const char *id,  unsigned long val){
	/* type can be state or log */
	myPrint->print("id=");
	myPrint->print(id);
	myPrint->print("&type=log&ms=");
	myPrint->println(val);
}



#ifdef __LCD
typedef struct {
	unsigned long time, avg, visits;
	char id[10];
	bool on;
	unsigned long previousMillis;
	/* char *id; */
} lcdData_t;


void readServer(){

	static char readString[101];
	static const int READ_STRING_SIZE = sizeof(readString) - 1;
	static int readIndex = 0;
	static char cmd[10], arg[10], par[10];
	const unsigned long LCDBACKLIGHT = 30000;// ms
	// initialize everything to zero
	static lcdData_t lcdData = {0};


	if((lcdData.on == true) && (millis() - lcdData.previousMillis > LCDBACKLIGHT)) {
		lcdData.on = false;
		lcd.off();
		/* lcd.noBacklight(); */
	}

	if (myStream->available()) {
		char c = myStream->read();
		if (readIndex < READ_STRING_SIZE) {
			readString[readIndex++] = c;
		}else{
			// reset the buffer if the incoming is too long
			memset(&readString[0], 0, sizeof(readString));
			readIndex = 0;
		}

		if (c == '\n') {
			//myPrint->print(F("Fre ram, newline: "));
			//myPrint->println(freeMemory(), DEC);
			// Terminate the read string
			readString[readIndex-1] = '\0';
			//myPrint->println(readString);

			// Prepare for next reading
			readIndex = 0;

			// convert readString to all uppercase
			for (uint16_t i = 0; i < strlen(readString); i++){
				readString[i] = toupper(readString[i]);
			}

			// search for first space
			char* pch = strchr(readString, ' ');
			if (pch != NULL){
				// copy first word until space
				memcpy(cmd,readString,pch-readString+1);
				cmd[strlen(cmd)-1] = '\0';
			}

			// handle command
			if (strcmp(cmd,"LOG")==0 ){
				while (pch!=NULL){
					// poor mans reqex. Match everything between ' '...=, that
					// is (space)...(=). The parameter
					char* ppar = strchr(pch, '=');
					if (ppar == NULL)
						break;
					// increment pointer to avoid ' '(space)
					*pch++;
					memcpy(par,pch,ppar-pch);
					par[strlen(par)] = '\0';

					// Match between =...' '. That is the argument of the parameter
					// normally it should be (pch+1,..), but we already incremented pch
					pch=strchr(pch,' ');
					*ppar++;

					// for the last set of par=arg, pch==NULL
					if (pch == NULL) {
						memcpy(arg,ppar,strlen(ppar));
					}else{
						memcpy(arg,ppar,pch-ppar);
					}
					arg[strlen(arg)] = '\0';
					// log id=t1 time=9999 avg=1234 

					if (strcmp(par,"ID")==0 ){
						/*
						  +1 is to make room for the NULL char that terminates C
						  strings in C++ we need to cast the return of
						  malloc. This is not the case in C
						 lcdData.id = (char *)malloc(strlen(arg) + 1);

						 Now id is allocated as a fixed char array in the struct.
						 strcpy adds '\0'
						*/
						strcpy(lcdData.id, arg);
					}else if (strcmp(par,"TIME")==0 ){
						lcdData.time = atol(arg);
					}else if (strcmp(par,"AVG")==0 ){
						lcdData.avg = atol(arg);
					}else if (strcmp(par,"VISITS")==0 ){
						lcdData.visits = atol(arg);
					}else{
						myStream->println("unknown log arguments - toiletduinoWEB"), myStream->print(arg);
					}


					// DEBUG
					/*
					Serial.print("pch: "),Serial.println(pch);
					Serial.print("ppar: "),Serial.println(ppar);
					Serial.print("afstand: ");
					Serial.println(ppar-pch);
					Serial.print("par is: ");
					Serial.println(par);
					Serial.print("arg is: ");
					Serial.println(arg);
					*/

					memset(&arg[0], 0, sizeof(arg));
					memset(&par[0], 0, sizeof(par));
				}

				/* I stedet for lange lcd.print, brug sprint()! */
				/* sprintf(buf, "0x%02x,%d,%d,%d,%d,", */
				/*         address, */
				/*         i2cparam[guess].en, */
				/*         i2cparam[guess].rw, */
				/*         i2cparam[guess].rs, */
				/*         i2cparam[guess].d4); */
				/* lcd.print(buf); */

				char buf[20];
				char timeStr[10];
				char sted[10];
				char buf2[4];
				bool bad = false;
				lcd.clear();
				lcd.on();
				/* lcd.backlight(); */
				lcd.home();
				if (strcmp(lcdData.id,"B1") == 0){
					sprintf(buf2,"tv."), bad = true;
				}else if (strcmp(lcdData.id,"B2") == 0){
					sprintf(buf2,"i mt"), bad = true;
				}else if (strcmp(lcdData.id,"B3") == 0){
					sprintf(buf2,"th."), bad = true;
				}else if (strcmp(lcdData.id,"T1") == 0){
					sprintf(buf2,"tv."), bad = false;
				}else if (strcmp(lcdData.id,"T2") == 0){
					sprintf(buf2,"th."), bad = false;
				}
				if(bad)
					sprintf(sted,"badet");
				else
					sprintf(sted,"toilet");
				sprintf(buf,"Du brugte %s %s", sted, buf2);
				lcd.print(buf);

				lcd.setCursor(0,1);
				formatTime(lcdData.time, timeStr);
				lcd.print("Det tog "), lcd.print(timeStr);

				lcd.setCursor(0,2);
				memset(buf, 0, sizeof(buf));
				sprintf(buf,"Besog i dag: %lu", lcdData.visits);
				lcd.print(buf);

				lcd.setCursor(0,3);
				formatTime(lcdData.avg, timeStr);
				memset(buf, 0, sizeof(buf));
				sprintf(buf,"Gns:    %s", timeStr);
				lcd.print(buf);

				// start timer
				lcdData.previousMillis = millis();
				lcdData.on = true;

			}else if (strcmp(cmd,"CMD")==0 ){
				// copy argument - without the leading space
				strcpy(par,&pch[1]);
				par[strlen(par)] = '\0';

				// handle parameters
				if (strcmp(par,"NETWORK")==0 ){
					myStream->print("Device name    : "), myStream->println(deviceName);
					myStream->print("mac address    : "), myStream->print(mac[0],HEX), myStream->print(":"), myStream->print(mac[1],HEX),
						myStream->print(":"), myStream->print(mac[2],HEX), myStream->print(":"), myStream->print(mac[3],HEX),
						myStream->print(":"), myStream->print(mac[4],HEX), myStream->print(":"), myStream->println(mac[5],HEX);
					myStream->print("ip address     : "), myStream->println(Ethernet.localIP());
					myStream->print("server address : "), myStream->println(server);
					myStream->print("port           : "), myStream->println(port);
					//myStream->print("gateway        : "), myStream->println(gateway);
					//}else if (strcmp(par,"UPTIME")==0 ){
					// http://forum.arduino.cc/index.php?topic=71212.0
				}else{
					myStream->println("unknown parameter - toiletduinoWEB"), myStream->print(par);
				}
			}else if (strcmp(cmd,"SET")==0 ){
				// copy argument - without the leading space
				strcpy(par,&pch[1]);
				arg[strlen(arg)] = '\0';
			}else{
				myStream->print("Unknown command - toiletduinoWEB: "), myStream->print(cmd);
			}

			// set everything to zero
			// metset sets the whole array to zero. Remember it is not enough just to
			// set the first byte to zero.
			// Because the first byte is always overwritten, so if the new string is
			// shorter than the old one, the array will consists of the new + rest of
			// the old.
			memset(&arg[0], 0, sizeof(arg));
			memset(&par[0], 0, sizeof(par));
			memset(&cmd[0], 0, sizeof(cmd));
			memset(&readString[0], 0, sizeof(readString));
			/* free(lcdData.id); */
			// set whole struct to zero
			// lcdData = (const lcdData_t){ 0 };
			memset(&lcdData.id[0], 0, sizeof(lcdData.id));
			lcdData.time = 0;
			lcdData.avg = 0;
			//myPrint->print(F("Fre ram, end og readserver: "));
			//myPrint->println(freeMemory(), DEC);
		}
	}
}


void formatTime(unsigned long time,char *timeStr){
	/* format the received time(in milliseconds) */

	int secs = (time/1000)%60;
	int mins = (time/(60000))%60; // 1000L*60L
	int hrs = (time/3600000L)%24; // 1000L*60L*60L = 3.6e6

	if (hrs > 0)
		sprintf(timeStr, "%d.%02d t", hrs, mins);
	else if (mins > 0)
		sprintf(timeStr, "%d.%d min", mins, secs/10 ); // show only leading second.
	else
		sprintf(timeStr, "%d sek", secs);
}
#endif
