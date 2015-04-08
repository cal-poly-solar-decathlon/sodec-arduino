/* Name: Arduino HTTP Client
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
#include <avr/wdt.h>
#include "TimerOne.h"

//void postToServer(float humidity, float temperature, char *room);
void postToServer(float humidity, float temperature);
void setupNetworkConnection();

//static port number outside the range of most reserved port numbers
#define SERVER_PORT 3000

// MAC address for Ethernet Shield.
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x47, 0x26 };

// IP for the server:
//IPAddress server(192,168,1,2);
//IPAddress server(192,168,2,5);
//IPAddress server(169,254,176,70);
IPAddress server(192,168,2,3);
//char server[] = "192.168.2.5";

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,2,4);

// Initialize the Ethernet client library
// with the IP address and port of the server
EthernetClient piServer;

char type,
     checksum;
char temperature[2],
     humidity[2];

char *uptime() // Function made to millis() be an optional parameter
{
  return (char *)uptime(millis()); // call original uptime function with unsigned long millis() value
}

char *uptime(unsigned long milli) {
  static char _return[32];
  unsigned long secs=milli/1000, mins=secs/60;
  unsigned int hours=mins/60, days=hours/24;
  milli-=secs*1000;
  secs-=mins*60;
  mins-=hours*60;
  hours-=days*24;
  sprintf(_return,"Uptime %d days %2.2d:%2.2d:%2.2d.%3.3d", (byte)days, (byte)hours, (byte)mins, (byte)secs, (int)milli);
  return _return;
}

void timer1Event() {
   static int count = 1;
   
   if (count++ >= 4) {
      Serial.print(" ");
      Serial.println(uptime());
      count = 1;
   }
}

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
   char *room = "s-temp-bed";
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
      if (checksumCheck(*(int16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {          //send ASCII data to server
            postToServer(room, *(int16_t *)temperature);
            room = "s-hum-bed";
            postToServer(room, *(int16_t *)humidity);
         }
         else {                       //send ASCII data to terminal
            Serial.print("Temperature1 (Bed): ");
            Serial.print((*(uint16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity1 (Bed): ");
            Serial.print((*(uint16_t *)humidity) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {            //checksum wrong, print error to terminal
         Serial.println("BAD checksum1");      
      }
   }
}

/* ProcessUART2
 * read from UART2 (same as ProcessUART1)
 */
void ProcessUART2(char where) {
   char *room = "s-temp-bath";
   Serial2.readBytes(&type, 1);
   if (type == 'T') {
      Serial2.readBytes(temperature, 2);
   }
   else if (type == 'H') {
      Serial2.readBytes(humidity, 2);
   }
   else if (type == 'C') {
      Serial2.readBytes(&checksum, 1);
      if (checksumCheck(*(int16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {
            postToServer(room, *(int16_t *)temperature);
            room = "s-hum-bath";
            postToServer(room, *(int16_t *)humidity);
          }
         else {
            Serial.print("Temperature2 (Bath): ");
            Serial.print((*(int16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity2 (Bath): ");
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
 * read from UART3 (same as ProcessUART1)
 */
void ProcessUART3(char where) {
   char *room = "s-temp-lr";
   Serial3.readBytes(&type, 1);
   if (type == 'T') {
      Serial3.readBytes(temperature, 2);
   }
   else if (type == 'H') {
      Serial3.readBytes(humidity, 2);
   }
   else if (type == 'C') {
      Serial3.readBytes(&checksum, 1);
      if (checksumCheck(*(int16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {
            postToServer(room, *(int16_t *)temperature);
            room = "s-hum-lr";
            postToServer(room, *(int16_t *)humidity);
         }
         else {
            Serial.print("Temperature3 (LR): ");
            Serial.print((*(int16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity3 (LR): ");
            Serial.print((*(uint16_t *)humidity) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {
         Serial.println("BAD Checksum3");
      }
   }
}

/* ProcessUART0
 * read from UART0 (same as ProcessUART1)
 */
void ProcessUART0(char where) {
   char *room = "s-temp-out";
   Serial.readBytes(&type, 1);
   if (type == 'T') {
      Serial.readBytes(temperature, 2);
   }
   else if (type == 'H') {
      Serial.readBytes(humidity, 2);
   }
   else if (type == 'C') {
      Serial.readBytes(&checksum, 1);
      if (checksumCheck(*(int16_t *)temperature, *(uint16_t *)humidity, checksum)) {
         if (where == 's') {
            postToServer(room, *(int16_t *)temperature);
            room = "s-hum-out";
            postToServer(room, *(int16_t *)humidity);
         }
         else {
            Serial.print("Temperature0 (Out): ");
            Serial.print((*(int16_t *)temperature) / 10.0, 1);
            Serial.print(" °C\tHumidity0 (Out): ");
            Serial.print((*(uint16_t *)humidity) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else if (where != 's'){
         Serial.println("BAD Checksum3");
      }
   }
}

void postToServer(char *room, int16_t value)
{
   char readC;
   
   char postString[30];
   char device[30];
   String data;
   String postFirstString;

   sprintf(postString, "status=%d&secret=$a8Es#crB469", value);
   data = String(postString);
   sprintf(device, "POST /srv/record-reading?device=%s HTTP/1.1", room);
   postFirstString = String(device);

   // Make a HTTP request:
   piServer.println(postFirstString);
   piServer.println("Host: 192.168.2.3:3000");
   piServer.println("Content-Type: application/x-www-form-urlencoded");
   piServer.print("Content-Length: ");
   piServer.println(data.length());
   piServer.println();
   piServer.print(data);
   piServer.println();
  
   Serial.print("Posted Data from Sensor: ");
   Serial.println(room);

   if (piServer.connected() && piServer.available()) {
      Serial.println("HTTP Response");
      while (piServer.connected() && piServer.available())
         readC = piServer.read();
   }
}

void setupNetworkConnection()
{
//   delay(1000);
   // start the Ethernet connection:
   //GET RID OF DHCP AND USE STATIC IP
   if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");
      // no point in carrying on, so do nothing forevermore:
      // try to congifure using IP address instead of DHCP:
      Ethernet.begin(mac, ip);
   }
   
   if(piServer.connect(server, 3000))
   {
      Serial.println("Connected");
   }
   else{
      piServer.stop();
      Serial.println("Cannot connect to server!");
   } 
}

void setup() {
   // Open serial communications and wait for port to open:
   Serial.begin(9600);
   Serial1.begin(9600);
   Serial2.begin(9600);
   Serial3.begin(9600);
   Timer1.initialize(5000000);
   Timer1.attachInterrupt(timer1Event);
   wdt_enable(WDTO_8S);
  
   delay(1000);
   setupNetworkConnection();
}

void loop()
{
   char readC;
   
//   if (Serial.available()) {
//      ProcessUART('s');      //read temp3 and humidity3 and send to server
//      wdt_reset();
//   }
   
   if (Serial1.available()) {
      ProcessUART1('s');      //read temp1 and humidity1 and send to server
      wdt_reset();
   }
   if (Serial2.available()) {
      ProcessUART2('s');      //read temp2 and humidity2 and send to server
      wdt_reset();
   }
   if (Serial3.available()) {
      ProcessUART3('s');      //read temp3 and humidity3 and send to server
      wdt_reset();
   }
   
   // if there are incoming bytes available from server,
   // read them and print them:
   if (piServer.connected() && piServer.available()) {
      Serial.println("HTTP Response");
      while (piServer.connected() && piServer.available())
         readC = piServer.read();
   }
   

   // if the server's disconnected, stop the piServer:
   if (!piServer.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      piServer.stop();
      
      setupNetworkConnection();
      
//      if (++disconnectCount > 9) {
//         // Output incoming temp and humidity data to the terminal forevermore:
//         while(true) {
//            if (Serial1.available()) {
//               ProcessUART1('t');      //read temp1 and humidity1 and send to terminal
//            }
//            if (Serial2.available()) {
//               ProcessUART2('t');      //read temp2 and humidity2 and send to terminal
//            }
//            if (Serial3.available()) {
//               ProcessUART3('t');      //read temp3 and humidity3 and send to terminal
//            }
//         }
//      }
   }
//   else
//      disconnectCount = 0;
}
