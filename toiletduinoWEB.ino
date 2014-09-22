/* -*- coding: utf-8 -*- */

/*
 Testing:
 Setup netcat to listen to the relevant (TCP) port on the server.
 In this case:
 $ nc -lvp 9999

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

#include <SPI.h>
#include <Ethernet.h>
#define DEBUG


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
/* Overloading or template */
/* http://stackoverflow.com/questions/8627625/is-it-possible-to-make-function-that-will-accept-multiple-data-types-for-given-a */
void sendServer(const char *id, bool state);
void sendServer(const char *id,  unsigned long val);

const unsigned long BLINKINTERVAL = 1000; // ms
const unsigned long MIN_DURATION = 15000; // ms

/* The MAC-adress have to be unique for the network */
/* Just increment the MAC-address from the end, eg: the next device could be
 * 0xDE 0xAD 0xBE 0xEF 0xFE 0xEE, for example. Then your third would end in
 * 0xEF, the fourth in 0xF0, and so on. */
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };
IPAddress ip(172, 16, 10, 2);
IPAddress gateway(172,16,0,1);

// Enter the IP address of the server you're connecting to:
IPAddress server(130,226,169,164);
EthernetClient client;


void setup() {
  // start the Ethernet connection:
  Ethernet.begin(mac, ip, gateway, gateway);
  // start the serial library:
  Serial.begin(9600);
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(server, 5555)) {
    Serial.println("connected");
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

  /* Setup sensors */
  setupSensor(&t1,"t1",5,9); // struct, id, pin, ledPin
  setupSensor(&t2,"t2",6,9);

  setupSensor(&b1,"b1",2,8);
  setupSensor(&b2,"b2",3,8);
  /* setupSensor(&b3,"b3",4,8); */

}

void loop()
{
  delay(100);

  /* Test for connection */
  int attemps = 0;
  while (!client.connected()) {
    Serial.println("Creating connection...");

    /* if connection is lost/disconnected, we might need to close the connection and start it again!! */
    if (attemps > 5){
      client.stop();
      attemps = 0;
    }
    client.connect(server, 5555);
    blink(b1.ledPin,6);
    attemps++;
    
    delay(1000);        
  }

  
  checkSensor(&b1);
  checkSensor(&b2);
  /* checkSensor(&b3); */
  checkSensor(&t1);
  checkSensor(&t2);
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
  digitalWrite(t->pin,HIGH);
  pinMode(t->ledPin, OUTPUT);  
}

void checkSensor(sensor_t *t){
  /* set a flag and start timer when door is locked. door is not declared 'locked' until after a set interval */

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
  /* client.print("&type="); */
  /* client.print(type); */
  /* client.print("&id="); */
  client.print("id=");
  client.print(id);
  client.print("&type=state & state=");
  client.println(val);
}

void sendServer(const char *id,  unsigned long val){
  /* type can be state or log */
  /* client.print("&type="); */
  /* client.print(type); */
  /* client.print("&id="); */
  client.print("&id=");
  client.print(id);
  client.print("&type=log&ms=");
  client.println(val);
}