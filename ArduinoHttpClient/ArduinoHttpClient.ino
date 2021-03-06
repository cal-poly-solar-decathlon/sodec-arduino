/* Name: Arduino HTTP Client
 * 
 * Description: Reads from up to 4 microcontrollers sending temp and humidity data (with checksum
 *              in an established format.
 * Author: Tim Ambrose
 * Copyright: 2015
 * Project: Solar Decathlon 2015 - home automation - temp/humidity readings
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>
#include "TimerOne.h"
#include "Time.h"
#include <SD.h>

#define TIMER1_PERIOD 1000000
//static port number outside the range of most reserved port numbers
#define SERVER_PORT 3000

static const uint32_t UPTIME_LOG_INCREMENT = 20;   //seconds

//void postToServer(float humidity, float temperature, char *room);
void postToServer(char *room, int16_t value);
void setupNetworkConnection();

// MAC address for Ethernet Shield.
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x47, 0x26 };

// IP for the server:
//IPAddress server(192,168,1,2);
//IPAddress server(192,168,2,5);
//IPAddress server(169,254,176,70);
IPAddress server(192,168,1,3);
//char server[] = "192.168.2.5";

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,1,4);

// Initialize the Ethernet client library
// with the IP address and port of the server
EthernetClient piServer;

uint32_t uptimeLoopCount;
uint32_t loopCounter2;
bool sdCardReady = false;
char type0,
     type1,
     type2,
     type3,
     checksum,
     readC;
char temperature0[2],
     temperature1[2],
     temperature2[2],
     temperature3[2],
     humidity0[2],
     humidity1[2],
     humidity2[2],
     humidity3[2];
char printUptime;
uint8_t serverTime;

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

//void timer1Event() {
//   static int count = 1;
//   
//   if (count++ >= 4) {
//      printUptime = 1;
////      Serial.print("+++++++++++++++++++++++++");
////      Serial.print(uptime());
////      Serial.println("+++++++++++++++++++++++++");
//      count = 1;
//   }
//}

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
//   char *room = "s-temp-bed\0";
   
   Serial1.setTimeout(50);
//   Serial.print("U1");
   Serial1.readBytes(&type1, 1);        //read first 
   if (type1 == 'T') {                  //read temp data
      Serial1.setTimeout(50);
      Serial1.readBytes(temperature1, 2);
   }
   else if (type1 == 'H') {             //read humidity data
      Serial1.setTimeout(50);
      Serial1.readBytes(humidity1, 2);
   }
   else if (type1 == 'C') {             //read checksum byte
      Serial1.setTimeout(50);
      Serial1.readBytes(&checksum, 1);
      //verify checksum
      if (checksumCheck(*(int16_t *)temperature1, *(uint16_t *)humidity1, checksum)) {
         if (where == 's') {          //send ASCII data to server
            postToServer(1, *(int16_t *)temperature1);
            postToServer(2, *(int16_t *)humidity1);
         }
         else {                       //send ASCII data to terminal
            Serial.print("Temperature1 (Bed): ");
            Serial.print((*(uint16_t *)temperature1) / 10.0, 1);
            Serial.print(" °C\tHumidity1 (Bed): ");
            Serial.print((*(uint16_t *)humidity1) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {            //checksum wrong, print error to terminal
         Serial.println("BAD checksum1");      
      }
   }
//   Serial.print("/U1\t");
}

/* ProcessUART2
 * read from UART2 (same as ProcessUART1)
 */
void ProcessUART2(char where) {
//   char *room = "s-temp-out\0";
   int roomH, roomT;
   
   Serial2.setTimeout(50);
   Serial2.readBytes(&type2, 1);
   if (type2 == 'T') {
      Serial2.setTimeout(50);
      Serial2.readBytes(&type2, 1);
      if (type2 == 'O' || type2 == 'K') {
         Serial2.setTimeout(50);
         Serial2.readBytes(temperature2, 2);
      }
//      else if (type0 == 'K') {
//         Serial2.setTimeout(50);
//         Serial2.readBytes(temperature2, 2);
//      }
   }
   else if (type2 == 'H') {
      Serial2.setTimeout(50);
      Serial2.readBytes(&type2, 1);
      if (type2 == 'O' || type2 == 'K') {
         Serial2.setTimeout(50);
         Serial2.readBytes(humidity2, 2);
      }
//      else if (type0 == 'K') {
//         Serial2.setTimeout(50);
//         Serial2.readBytes(temperature0, 2);
//      }
   }
   else if (type2 == 'C') {
      Serial2.setTimeout(50);
      Serial2.readBytes(&type2, 1);
      if (type2 == 'O' || type2 == 'K') {
         if (type2 == 'O') {
            roomT = 9;
            roomH = 0;
         }
         else if (type2 == 'K') {
            roomT = 7;
            roomH = 8;
         }
         else
            return;
         Serial2.setTimeout(50);
         Serial2.readBytes(&checksum, 1);
         if (checksumCheck(*(int16_t *)temperature2, *(uint16_t *)humidity2, checksum)) {
            if (where == 's') {
               postToServer(roomT, *(int16_t *)temperature2);
               postToServer(roomH, *(int16_t *)humidity2);
            }
            else {
               if (type2 == 'O')
                  Serial.print("Temperature2 (Outside): ");
               else if (type2 == 'K')
                  Serial.print("Temperature2 (Kitchen): ");
               else
                  Serial.print("Temperature2 (Unknown): ");
               Serial.print((*(int16_t *)temperature2) / 10.0, 1);
               if (type2 == 'O')
                  Serial.print(" C\t\tHumidity2 (Outside): ");
               else if (type2 == 'K')
                  Serial.print(" C\t\tHumidity2 (Kitchen): ");
               else
                  Serial.print(" C\t\tHumidity2 (Unknown): ");
               Serial.print((*(uint16_t *)humidity2) / 10.0, 1);
               Serial.print("%\n\r");
            }
         }
         else if (where != 's'){
            Serial.println("BAD Checksum2");
         }
      }
   }
}

/* ProcessUART3
 * read from UART3 (same as ProcessUART1)
 */
void ProcessUART3(char where) {
//   char *room = "s-temp-lr\0";
   
   Serial3.setTimeout(50);
//   Serial.print("U3");
   Serial3.readBytes(&type3, 1);
   if (type3 == 'T') {
      Serial3.setTimeout(50);
      Serial3.readBytes(temperature3, 2);
   }
   else if (type3 == 'H') {
      Serial3.setTimeout(50);
      Serial3.readBytes(humidity3, 2);
   }
   else if (type3 == 'C') {
      Serial3.setTimeout(50);
      Serial3.readBytes(&checksum, 1);
      if (checksumCheck(*(int16_t *)temperature3, *(uint16_t *)humidity3, checksum)) {
         if (where == 's') {
            postToServer(5, *(int16_t *)temperature3);
            postToServer(6, *(int16_t *)humidity3);
         }
         else {
            Serial.print("Temperature3 (LR): ");
            Serial.print((*(int16_t *)temperature3) / 10.0, 1);
            Serial.print(" °C\tHumidity3 (LR): ");
            Serial.print((*(uint16_t *)humidity3) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {
         Serial.println("BAD Checksum3");
      }
   }
//   Serial.print("/U3\t");
}

/* ProcessUART0
 * read from UART0 (same as ProcessUART1)
 */
void ProcessUART0(char where) {
//   char *room = "s-temp-bath\0";
   
   Serial.setTimeout(50);
//   Serial.print("U2");
   Serial.readBytes(&type0, 1);
   if (type0 == 'T') {
      Serial.setTimeout(50);
      Serial.readBytes(temperature0, 2);
   }
   else if (type0 == 'H') {
      Serial.setTimeout(50);
      Serial.readBytes(humidity0, 2);
   }
   else if (type0 == 'C') {
      Serial.setTimeout(50);
      Serial.readBytes(&checksum, 1);
      if (checksumCheck(*(int16_t *)temperature0, *(uint16_t *)humidity0, checksum)) {
         if (where == 's') {
            postToServer(3, *(int16_t *)temperature0);
            postToServer(4, *(int16_t *)humidity0);
          }
         else {
            Serial.print("Temperature0 (Bathroom): ");
            Serial.print((*(int16_t *)temperature0) / 10.0, 1);
            Serial.print(" C\t\tHumidity0 (Bathroom): ");
            Serial.print((*(uint16_t *)humidity0) / 10.0, 1);
            Serial.print("%\n\r");
         }
      }
      else {
         Serial.println("BAD Checksum0");
      }
   }
//   Serial.print("/U2\t");
}

void postToServer(int dev, int16_t value) {
   char postString[100] = {0};
   char device[100] = {0};
   String data;
   String postFirstString;
   char room[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
   
   switch(dev) {
   case 1:
      strcpy(room, "s-temp-bed\0");
      break;
   case 2:
      strcpy(room, "s-hum-bed\0");
      break;
   case 3:
      strcpy(room, "s-temp-bath\0");
      break;
   case 4:
      strcpy(room, "s-hum-bath\0");
      break;
   case 5:
      strcpy(room, "s-temp-lr\0");
      break;
   case 6:
      strcpy(room, "s-hum-lr\0");
      break;
   case 7:
      strcpy(room, "s-temp-kit\0");
      break;   
   case 8:
      strcpy(room, "s-hum-kit\0");
      break;
   case 9:
      strcpy(room, "s-temp-out\0");
      break;
   default:
      strcpy(room, "s-hum-out\0");
      break;
   }

   Serial.print("#post  ");
//   noInterrupts();
   sprintf(postString, "status=%d&secret=$a8Es#crB469", value);
   data = String(postString);
   sprintf(device, "POST /srv/record-reading?device=%s HTTP/1.1", room);
//   postFirstString = String(device);

   if (piServer.connected()) {
     // Make a HTTP request:
     wdt_reset();
     piServer.println(device);
     piServer.println("Host: 192.168.2.5:3000");
     piServer.println("Content-Type: application/x-www-form-urlencoded");
     piServer.print("Content-Length: ");
     piServer.println(data.length());
     piServer.println();
     piServer.print(data);
     piServer.println();
     
     Serial.print("Posted: ");
     Serial.print(room);
     Serial.print(":\t");
     Serial.println(value / 10.0);
//     Serial.print("\t");
   }
   else
      Serial.println("Failed to Post");
      
   if (!piServer.connected()) {
      piServer.stop();
      setupNetworkConnection();
   }
   delay(1000);
   if (!piServer.connected()) {
      piServer.stop();
      setupNetworkConnection();
   }

   if (piServer.available()) {
      Serial.println("--RESP--");
      wdt_reset();
      while (piServer.available()) {
         readC = piServer.read();
         Serial.print(readC);
      }
      Serial.println();
   }
   else {
      Serial.println("))) NO RESP (((");
   }
   if (!piServer.connected()) {
      piServer.stop();
      setupNetworkConnection();
   }
//   interrupts();
}

void setupNetworkConnection()
{
   Serial.println("#setup");
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
   
//   Serial.print("#/setup\t");
}

void sendReceiveTime() {
   String timestampStr;
   time_t epoch;
   if (piServer.connected()) {
      piServer.println("GET /srv/timestamp HTTP/1.1");
      piServer.println("Host: 192.168.1.3:3000");
      piServer.println();
   }
   if (!piServer.connected()) {
      piServer.stop();
      setupNetworkConnection();
   }
   delay(1000);
   if (!piServer.connected()) {
      piServer.stop();
      setupNetworkConnection();
   }
   if (piServer.available()) {
      Serial.println("--TIMESTAMP--");
      wdt_reset();
      while (piServer.available() && readC != '{')    //skip to json start
         readC = piServer.read();
      while (piServer.available() && readC != ':')    //skip to payload
         readC = piServer.read();
      while (piServer.available() && readC != '}') {
         readC = piServer.read();
         if (readC != '}')
            timestampStr += readC;
      }
      
      epoch = timestampStr.toInt();
      Serial.println(epoch);
      setTime(epoch);
      adjustTime(-7 * 3600);
      Serial.print(hour()); Serial.print(" "); Serial.println(minute());
      uint8_t miniTime = hour() * 10 + minute() / 10;
      Serial.print(miniTime);
      Serial2.write(miniTime);
      while (piServer.available())
         readC = piServer.read();
   }
   else {
      Serial.println("))) NO TIMESTAMP RESP (((");
   }
}

void setup() {
   // Open serial communications and wait for port to open:
   Serial.begin(9600);
   Serial1.begin(9600);
   Serial2.begin(9600);
   Serial3.begin(9600);
   Serial.setTimeout(50);
   Serial1.setTimeout(50);
   Serial2.setTimeout(50);
   Serial3.setTimeout(50);
   
   uptimeLoopCount = 0;
   
   Timer1.initialize(8000000);
   Timer1.start();
//   Timer1.attachInterrupt(timer1Event);
   wdt_enable(WDTO_8S);
  
   delay(1000);
   Serial.println("----------------------------------RESET------------------------------------");
   setupNetworkConnection();
}

void loop()
{
   if (Timer1.read() > TIMER1_PERIOD) {
      uptimeLoopCount++;
      loopCounter2++;
      Timer1.restart();
      wdt_reset();
   }

   if (loopCounter2 > UPTIME_LOG_INCREMENT / 2) {
//      sendReceiveTime();
      loopCounter2 = 0;
   }
      
   if (uptimeLoopCount > UPTIME_LOG_INCREMENT) {
      sendReceiveTime();
      Serial.print("+++++++++++++++");
      Serial.print(uptime());
      Serial.println("+++++++++++++++");
      //if pin A0 is high, we have theoretically inserted an SD card
//      if (PINA & (1<<PA0)) {
//         // if we have already initialized the card
//         if (sdCardReady) {
//            File dataFile = SD.open("/Uptime.log", FILE_WRITE);
//            // if the file is available, write to it:
//            if (dataFile) {
//               dataFile.println(uptime());
//               dataFile.close();
//               Serial.println("---Logged to SD card---");
//            }
//            // if the file isn't open, pop up an error:
//            else {
//               Serial.println("error opening Uptime.log");
//            }
//         } else { //initialize the card
//            if (!SD.begin(4)) {
//               Serial.println("Card failed, or not present");
//            } else {
//               sdCardReady = true;
//            }
//         }
//      } else { //we have theoretically removed the card
//         sdCardReady = false;
//         Serial.println("No Card");
//      }
         uptimeLoopCount = 0;
   }
   
   if (Serial.available()) {
      ProcessUART0('s');      //read temp0 and humidity0 and send to server
      wdt_reset();
   }   
   
   if (Serial1.available()) {
      ProcessUART1('s');      //read temp1 and humidity1 and send to server
//      Serial.print("UART1\t");
      wdt_reset();
   }
   if (Serial2.available()) {
      ProcessUART2('s');      //read temp2 and humidity2 and send to server
//      Serial.print("UART2\t");
      wdt_reset();
   }
   if (Serial3.available()) {
      ProcessUART3('s');      //read temp3 and humidity3 and send to server
//      Serial.print("UART3\t");
      wdt_reset();
   }
   
   // if there are incoming bytes available from server,
   // read them and print them:
//   if (piServer.connected() && piServer.available()) {
//      Serial.println("--RESP--");
//      while (piServer.connected() && piServer.available())
//         readC = piServer.read();
//   }
   

   // if the server's disconnected, stop the piServer:
   if (!piServer.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      piServer.stop();
      
      setupNetworkConnection();
      wdt_reset();
      
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
