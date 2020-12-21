//////////// debug macros

#define PRINT_DEBUG 1

#if PRINT_DEBUG
#define PRINT(a)   { Serial.print(a); }
#define PRINTLN(a)  { Serial.println(a); }
#else
#define PRINT(s,v)
#define PRINTLN(s,x)
#endif


//////////// config

#define GARAGE_DOOR_DISTANCE 30  //(centimeters) distance of the door of the garage from the sensor
#define CHECK_RATE 10  //(seconds) how often does the chip check for the door
#define DOOR_CLOSE_TIME 60 //(seconds) after what time should the bot send an SMS
#define GSM_NUMBER "+420603453610"
#define WARNING_MSG "The door is open!"

//////////// gsm system

#include <SoftwareSerial.h>
//SIM800 TX is connected to Arduino D8
#define SIM800_TX_PIN 8
//SIM800 RX is connected to Arduino D7
#define SIM800_RX_PIN 7
SoftwareSerial serialSIM800(SIM800_TX_PIN,SIM800_RX_PIN);

void sendSMS()
{
  serialSIM800.write("AT+CMGF=1\r\n");
  delay(1000);
  serialSIM800.write("AT+CMGS=\"" GSM_NUMBER "\"\r\n");
  delay(1000);
  serialSIM800.write(WARNING_MSG);
  delay(1000);
  serialSIM800.write((char)26);
  delay(1000);
}


//////////// USS ultradsounder

#define TRIGGER 3
#define ECHO 4
long distance;

long ms2cm(long microseconds){ return microseconds / 29 / 2; }

long getUssTime()
{
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);
  return (long) pulseIn(ECHO, HIGH);
}

long getDistance() {
  return ms2cm( getUssTime() );
}

//////////// Application

// application states

#define STATE_CLOSED      1
#define STATE_OPEN        2
#define STATE_ALARM_SENT  3

int app_state = STATE_CLOSED;

long timer_start, timer_end = 0;

void setup()
{
  Serial.begin(9600);
  // while(!Serial);
  PRINTLN("Connecting...");

  serialSIM800.begin(9600);
  delay(1000);
  
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);

  PRINTLN("Connected!");
}

void loop()
{
  // check door status
  distance = getDistance();

  PRINT("Distance measurement: ");
  PRINT(distance);
  PRINTLN(" cm");

  if (distance > GARAGE_DOOR_DISTANCE) {
    // door is CLOSED
    
    app_state = STATE_CLOSED;
    PRINTLN("In STATE_CLOSED");
  }
  else {
    // door is OPEN

    switch (app_state) {
      case STATE_CLOSED:
        // change state to OPEN
        app_state = STATE_OPEN;
        // start timer
        timer_start = millis();
        PRINTLN("Entered STATE_OPEN");
      break;
      case STATE_OPEN:
        // check timer
        timer_end = millis();
        if (timer_end <= timer_start) {
          // timer rolled over (happens approx. every 50 days)
          // we just restart the timer and check again next check interval
          timer_start = timer_end;
        }
        else {
          if ((timer_end - timer_start) / 1000 >= DOOR_CLOSE_TIME) {
            // send message
            sendSMS();
            PRINTLN("Alarm triggered!");
            // change state to ALARM_SENT
            app_state = STATE_ALARM_SENT;
          }
        }
      break;
      case STATE_ALARM_SENT:
        // do nothing, just wait for someone to close the bloody door!
        PRINTLN("In STATE_ALARM_SENT");
      break;    
    }
  }

  // wait till next check
  delay(CHECK_RATE * 1000);
}
