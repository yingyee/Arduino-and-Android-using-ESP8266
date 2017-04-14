#include <Servo.h>
#include <ESP8266.h>

#define WIFI_CHECK_TIME 5000  // interval-time (in ms) to check WiFi module (5s)
#define WIFI_RESTART_TIMEOUT 30000  // interval-time (in ms) to restart the WiFi ESP8266 module 
                                    // in case that it become unresponsive (hardware reset required!)
#define DEBUG true

#define ir_sense0 A0  //parking 1 
#define ir_sense1 A1  //parking 2
#define ir_sense2 A2  //parking 3
#define ir_sense3 A3  //parking 4
#define ir_sense4 A4  //parking 5
#define ir_sense5 A5  //parking 6
#define ir_sense6 A6  //parking 7
#define ir_sense7 A7  //parking 8
#define ir_sense8 A8  //parking 9
#define ir_sense9 A9  //parking 10
#define ir_sense10 A10 //in/down counter
#define ir_sense11 A11 //exit/up counter
#define ServoI    9     //Connected to the servo motor entrance.
#define ServoO    10    //Connected to the servo motor exit.
#define BarLow    180   //Low position of the barrier.
#define BarUp     90    //Up position of the barrier.

Servo myservo1;  // create servo object to control a servo
Servo myservo2;
int number_of_space = 10;
int lot = 0;

ESP8266 esp(Serial);   // ESP8266 WiFi module controller

// data template for sensors
const char SENSORS_DATA_TEMPLATE[] PROGMEM =
  "{\"P0\": %d, \"P1\": %d, \"P2\": %d, \"P3\": %d, \"P4\": %d, \"P5\": %d, \"P6\": %d, \"P7\": %d, \"P8\": %d, \"P9\": %d}";

enum class Command
{
  GET_ALL_SENSORS_DATA = 65
};

void setupWiFi()
{
  Serial.begin(115200);
  Serial1.begin(115200);

  sendCommand("AT+RST\r\n", 2000, DEBUG); // reset module
  sendCommand("AT+CWMODE=3\r\n", 1000, DEBUG); // configure as access point
  sendCommand("AT+CWJAP=\"xxx\",\"yyy\"\r\n", 3000, DEBUG);
  delay(10000);
  sendCommand("AT+CIFSR\r\n", 1000, DEBUG); // get ip address
  sendCommand("AT+CIPMUX=1\r\n", 1000, DEBUG); // configure for multiple connections
  sendCommand("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80

  esp.atCipstartUdp();  // start UDP connection - wait on all ports
}

void setupHardware()
{
  pinMode(ir_sense0, INPUT);  // Parking 1
  pinMode(ir_sense1, INPUT);  // Parking 2
  pinMode(ir_sense2, INPUT);  // Parking 3
  pinMode(ir_sense3, INPUT);  // Parking 4
  pinMode(ir_sense4, INPUT);  // Parking 5
  pinMode(ir_sense5, INPUT);  // Parking 6
  pinMode(ir_sense6, INPUT);  // Parking 7
  pinMode(ir_sense7, INPUT);  // Parking 8
  pinMode(ir_sense8, INPUT);  // Parking 9
  pinMode(ir_sense9, INPUT);  // Parking 10
  pinMode(ir_sense10, INPUT); // In/down counter
  pinMode(ir_sense11, INPUT); // Exit/up counter
  pinMode(ServoI, OUTPUT);
  pinMode(ServoO, OUTPUT);
  myservo1.attach(ServoI);    // Attaches the servo.
  myservo1.write(BarLow);     // Barrier in the low position
  myservo2.attach(ServoO);    // Attaches the servo.
  myservo2.write(BarLow);     // Barrier in the low position
}

void createSensorsDataFromTemplate(char *&data)
{
  char buffLot1[7] = {0}, buffLot2[7] = {0}, buffLot3[7] = {0}, buffLot4[7] = {0}, buffLot5[7] = {0},
                                        buffLot6[7] = {0}, buffLot7[7] = {0}, buffLot8[7] = {0}, buffLot9[7] = {0}, buffLot10[7] = {0}, tmp1[140] = {0};
  char *pTmp1 = tmp1;
  uint8_t templateLen = -1;
  getPMData(SENSORS_DATA_TEMPLATE, pTmp1, templateLen); // read template from PROGMEM

  // create data string from template by replacing
  // parameters with their actual values from sensors
  //sprintf( data, pTmpl);
  buffLot1[7] == analogRead(ir_sense0);
  buffLot2[7] == analogRead(ir_sense1);
  buffLot3[7] == analogRead(ir_sense2);
  buffLot4[7] == analogRead(ir_sense3);
  buffLot5[7] == analogRead(ir_sense4);
  buffLot6[7] == analogRead(ir_sense5);
  buffLot7[7] == analogRead(ir_sense6);
  buffLot8[7] == analogRead(ir_sense7);
  buffLot9[7] == analogRead(ir_sense8);
  buffLot10[7] == analogRead(ir_sense9);

  /*dtostrf( analogRead(ir_sense0), 6, 1, buffLot1),
    dtostrf( analogRead(ir_sense1), 6, 1, buffLot2),
    dtostrf( analogRead(ir_sense2), 6, 1, buffLot3),
    dtostrf( analogRead(ir_sense3), 6, 1, buffLot4),
    dtostrf( analogRead(ir_sense4), 6, 1, buffLot5));*/
}

void processRequest( char *data)
{
  char progmemData[150] = {0};
  char *pData = progmemData;
  // first char represents the command
  char cmd = *(data);
  switch ( (Command)cmd)
  {
    case Command::GET_ALL_SENSORS_DATA:
      createSensorsDataFromTemplate(pData);
      esp.atCipsend(pData);
      break;

    default:
      // nothing to do ...
      break;
  }
}

void setup()
{
  setupWiFi();
  setupHardware();
  //Get initial data from sensors
  //Add a 2 seconds delay to prevent timeouts from sensors
  delay(2000);
}

void loop()
{
  int down_counter  = analogRead(ir_sense10);
  int up_counter    = analogRead(ir_sense11);

  char data[10] = {0}, *ipdData = data;   // Incomming real data via IPD is 1 byte (the command identifier byte).
  // The first bytes are: "+IPD,n:", followed by the real data
  if (Serial1.available() > 7)           // Incomming data from ESP8266 module
  { // Lengt must be greater than 7 bytes ("+IPD,n:")
    if (esp.ipd (ipdData) == ESP8266::Error::NONE) // process request received from UDP port: (wait 50ms for data)
    {
      processRequest(ipdData);           // process the request
    }
  }
  delay(250);                             // a small delay before checking again for WiFi incomming data
}

/*
  Name: sendCommand
  Description: Function used to send data to ESP8266.
  Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
  Returns: The response from the esp8266 (if there is a reponse)
*/
String sendCommand(String command, const int timeout, boolean debug)
{
  String response = "";

  Serial1.print(command); // send the read character to the esp8266

  long int time = millis();

  while ( (time + timeout) > millis())
  {
    while (Serial1.available())
    {

      // The esp has data so display its output to the serial window
      char c = Serial1.read(); // read the next character.
      response += c;
    }
  }

  if (debug)
  {
    Serial.print(response);
  }

  return response;
}
