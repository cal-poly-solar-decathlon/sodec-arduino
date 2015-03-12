/* Name: Arduino Mega Multi-Mini Client
 * 
 * Description: Reads from up to 4 microcontrollers sending temp and humidity data (with checksum
 *              in an established format.
 * Author: Tim Ambrose
 * Copyright: 2015
 * Project: Solar Decathlon 2015 - home automation - temp/humidity readings
 */

#include <stdio.h>
#include <stdint.h>
#include <SPI.h>
#include <Ethernet.h>

//static port number outside the range of most reserved port numbers
#define SERVER_PORT 54998

// MAC address for Ethernet Shield.
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x47, 0x26 };

// IP for the server:
//IPAddress server(192,168,1,2);
IPAddress server(192,168,2,4);
//IPAddress server(169,254,176,70);

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,2,4);

// Initialize the Ethernet client library
// with the IP address and port of the server
EthernetClient piServer;

char type,
     checksum;
char temperature[2],
     humidity[2];

/* checksumCheck
 * return equality of received checksum value and calculated checksum
 */
int checksumCheck(uint16_t temp, uint16_t humid, char checksum) {
   char sum = 0;
   int i;
   
   for (i = 0; i < 2; ++i)
      sum += (temp >> i * 8) & 0xFF;
   for (i = 0; i < 2; ++i)
      sum += (humid >> i * 8) & 0xFF;
   
   return checksum == sum;
}

/* ProcessUART1
 * read from UART1 the temperature and humidity data in the format (1 byte at a time):
 * 'T'<temp><temp>'H'<humidity><humidity>'C'<checksum>
 * sends to ('s')server or ('t')terminal connected by default to serial0
 */
void ProcessUART1(char where) {
   Serial1.readBytes(&type, 1);        //read first 
   if (type == 'T') {                  //read temp data
      Serial1.readBytes(temperature, 2);
   }
   else if (type == 'H') {             //read humidity data
      Serial1.readBytes(humidity, 2);
   }
   else if (type == 'C') {             //read checksum byte
      Serial1.readBytes(&checksum, 1);
      //verify checksum
      if (checksumCheck(*(uint16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {          //send ASCII data to server
            piServer.print("Temperature1: ");
            piServer.print((*(int16_t *)temperature) / 10.0, 1);
            piServer.print(" °C\tHumidity1: ");
            piServer.print((*(uint16_t *)humidity) / 10.0, 1);
            piServer.print("%\n\r");
         }
         else {                       //send ASCII data to terminal
            Serial.print("Temperature1: ");
            Serial.print((*(int16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity1: ");
            Serial.print((*(uint16_t *)humidity) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {            //checksum wrong, print error to terminal
         Serial.println("BAD Checksum1");      
      }
   }
}

/* ProcessUART2
 * read from UART2 (same as ProcessUART1
 */
void ProcessUART2(char where) {
   Serial2.readBytes(&type, 1);
   if (type == 'T') {
      Serial2.readBytes(temperature, 2);
   }
   else if (type == 'H') {
      Serial2.readBytes(humidity, 2);
   }
   else if (type == 'C') {
      Serial2.readBytes(&checksum, 1);
      if (checksumCheck(*(uint16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {
            piServer.print("Temperature2: ");
            piServer.print((*(int16_t *)temperature) / 10.0, 1);
            piServer.print(" °C\tHumidity2: ");
            piServer.print((*(uint16_t *)humidity) / 10.0, 1);
            piServer.print("%\n\r");
         }
         else {
            Serial.print("Temperature2: ");
            Serial.print((*(int16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity2: ");
            Serial.print((*(uint16_t *)humidity) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {
         Serial.println("BAD Checksum2");
      }
   }
}

/* ProcessUART3
 * read from UART3 (same as ProcessUART1
 */
void ProcessUART3(char where) {
   Serial3.readBytes(&type, 1);
   if (type == 'T') {
      Serial3.readBytes(temperature, 2);
   }
   else if (type == 'H') {
      Serial3.readBytes(humidity, 2);
   }
   else if (type == 'C') {
      Serial3.readBytes(&checksum, 1);
      if (checksumCheck(*(uint16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {
            piServer.print("Temperature3: ");
            piServer.print((*(int16_t *)temperature) / 10.0, 1);
            piServer.print(" °C\tHumidity3: ");
            piServer.print((*(uint16_t *)humidity) / 10.0, 1);
            piServer.print("%\n\r");
         }
         else {
            Serial.print("Temperature3: ");
            Serial.print((*(int16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity3: ");
            Serial.print((*(uint16_t *)humidity) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {
         Serial.println("BAD Checksum3");
      }
   }
}

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);

//  // start the Ethernet connection:
//  if (!Ethernet.begin(mac)) {
//    Serial.println("Failed to configure Ethernet using DHCP");
//    // no point in carrying on, so do nothing forevermore:
//    // try to congifure using IP address instead of DHCP:
//    Ethernet.begin(mac, ip);
//  }
//  // give the Ethernet shield a second to initialize:
//  delay(1000);
//  Serial.println("connecting...");
//
//  // if you get a connection, report back via serial:
//  if (piServer.connect(server, SERVER_PORT)) {
//    Serial.println("connected");
////    piServer.print("Temperature: 20.9 °C\tHumidity: 60%\0");
//  } 
//  else {
//    // if you didn't get a connection to the server:
//    Serial.println("connection failed");
//  }
}

void loop()
{
   if (Serial1.available()) {
      ProcessUART1('s');      //read temp1 and humidity1 and send to server
   }
   if (Serial2.available()) {
      ProcessUART2('s');      //read temp2 and humidity2 and send to server
   }
   if (Serial3.available()) {
      ProcessUART3('s');      //read temp3 and humidity3 and send to server
   }
   // if there are incoming bytes available from server,
   // read them and print them:
//   if (piServer.available()) {
//     char c = piServer.read();
//     Serial.print(c);
//   }

   // if the server's disconnected, stop the piServer:
//   if (!piServer.connected()) {
//      Serial.println();
//      Serial.println("disconnecting.");
//      piServer.stop();

      // Output incoming temp and humidity data to the terminal forevermore:
      while(true) {
         if (Serial1.available()) {
            ProcessUART1('t');      //read temp1 and humidity1 and send to terminal
         }
         if (Serial2.available()) {
            ProcessUART2('t');      //read temp2 and humidity2 and send to terminal
         }
         if (Serial3.available()) {
            ProcessUART3('t');      //read temp3 and humidity3 and send to terminal
         }
      }
//   }
}

