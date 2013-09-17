/* Definition of Libraries*/
#include <Time.h>                          // Libray in charge of the time count of the system
#include <Wire.h>                          // Libray in charge of the I2C communication
#include <SD.h>                            // Libray in charge of the SD
#include <WiFi.h>                          // Libray in charge of WIFI shield
#include <WiFiUdp.h>                       // Libraries in charge of Upload of data to the server
#include <SPI.h>
#include <HttpClient.h>
#include <Xively.h>                        // Library of the Cloud

/* Definition of variables constants*/
/* Definitions for the case*/
int LED0 = 30;                            // LED to determine if the fridge is ON or OFF
int LED1 = 24;                            // LED showing that the WIFI shield is working
int LED2 = 22;                            // LED showing that the WIFI shield isn't working
int LED3 = 26;                            // LED showing that the data is stored in the SD
int LED4 = 28;                            // LED showing that the the SD isn't working
int LED5 = 32;                            // LED showing that the data was sent to the server
int LED6 = 34;                            // LED showing that the data wasn't sent to the server

/* Definitions for the SD card */
File myFile;                              // Define a variable to save in the SD

/* Definitions for the Temperature Reading */
int refrigerator = 8;                     // The refrigerator will be conected in the pin 8
float temp0;                              // Define a variable for the "calling" sensor
volatile int pulse_number = 0;            // Define a variable pulse number
int var = 1;

/* Definitions for the Wifi connection */
int status = WL_IDLE_STATUS;              // Stablish a WAP connection of the network
char ssid[] = "RenQi WiFi 2";             // The network SSID (name)
char pass[] = "z1yd-htb6-9ufj";           // The network password
unsigned int localPort = 2390;            // local port to listen for UDP packets for time server
IPAddress timeServer(129, 6, 15, 28);     // Opening NTP server time.nist.gov
const int NTP_PACKET_SIZE = 48;           // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];      // Buffer to hold incoming and outgoing packets 
WiFiUDP Udp;                              // A UDP instance to let us send and receive packets over UDP
const long timeZoneOffset = 7200L;        // Define time zone offset constant

/* Definitions for the Xively Upload (Cloud)*/
int ret;                                  // Define the variables for server response
int getret;
float tempON;
float tempOFF;
char xivelyKey[] =                        // The Xively key to let you upload data
"1ZKzgBO8YU6NFbxRwyAz8N7dPSE7A4w2wkBqxkNEy1xPTCRz";
#define xivelyFeed 358590698              // The xively feed ID
char sensor0ID[] = "Temp0";               // Define the datastreams' IDs
char sensor1ID[] = "Temp1";
char sensor2ID[] = "Temp2";
char sensor3ID[] = "Temp3";
char sensor4ID[] = "Temp4";
char sensor5ID[] = "Pulses";
char TempON[] = "TempON";
char TempOFF[] = "TempOFF";
XivelyDatastream datastreams[] = {        /* Define the strings for our datastream IDs */
  XivelyDatastream(sensor0ID, strlen(sensor0ID), DATASTREAM_FLOAT),
  XivelyDatastream(sensor1ID, strlen(sensor1ID), DATASTREAM_FLOAT),
  XivelyDatastream(sensor2ID, strlen(sensor2ID), DATASTREAM_FLOAT),
  XivelyDatastream(sensor3ID, strlen(sensor3ID), DATASTREAM_FLOAT),
  XivelyDatastream(sensor4ID, strlen(sensor4ID), DATASTREAM_FLOAT),
  XivelyDatastream(sensor5ID, strlen(sensor5ID), DATASTREAM_FLOAT),
  XivelyDatastream(TempON, strlen(TempON), DATASTREAM_FLOAT),
  XivelyDatastream(TempOFF, strlen(TempOFF), DATASTREAM_FLOAT),
};
XivelyFeed feed(xivelyFeed, datastreams, 8); // Finally, wrap the datastreams into a feed (the last number is the # of streams)
WiFiClient client;                        // Necessary definitions for the Xively library
XivelyClient xivelyclient(client);        // Define the communication with Xively as a Client

/* The following code is the setup part of the program and it will just execute itself once (at the start of the program) 
In this part we define some characteristics of the pins, we start some of the libraries, start the wifi, 
the SD: create the file (or open it), sync the time with the server, write the headers and other stuff like that */
void setup() 
{
  pinMode(LED0, OUTPUT);                  // Define the LEDs as output pins
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(LED6, OUTPUT);
  Serial.begin(9600);                     // Start the Serial Monitor with 9600 bps
  if (WiFi.status() == WL_NO_SHIELD)      // Checking for the presence of the shield
  {
    digitalWrite(LED2, HIGH);             // Turn on LED to establish status
    Serial.println ("Wifi not present");  // Message for know that there is a problem of the WIFI shield
    while(true);                          // While this status is true, keep in the loop
  } 
  while ( status != WL_CONNECTED)         // Attempt to connect to Wifi network:
  { 
    status = WiFi.begin(ssid, pass);      // Try to begin the connection with the WIFI network
    Serial.println ("No Connection");     // Message to know status
    digitalWrite(LED2, HIGH);             // Turn on LED to establish status
    delay(5000);                          // Wait 5 seconds for connection, otherwise try again
  }
  if (!SD.begin(4))
  {
    digitalWrite(LED4, HIGH);                 // Turn on LED to establish status
    Serial.println("initialization failed!"); // Error message if the SD could not be reached.
  }
  myFile = SD.open("DATA.txt", FILE_WRITE);   // open the file. note that only one file can be open at a time, so you have to close this one before opening another.
  digitalWrite(LED2, LOW);                    // turn off the error alert LED
  digitalWrite(LED1, HIGH);                   // turn on the LED to establish status
  printWifiStatus();                          // Stablish a loop for wifi status
  Udp.begin(localPort);                       // Begin the Udp
  setSyncProvider(getNtpTime);                // Sync the hour with the Ntp Server
  Wire.begin();                               // Open library for the sensor
  pinMode(8, OUTPUT);                         // Define the pin 8 as the OUTPUT for the compressor of the fridge
  attachInterrupt (0, energy, RISING);        // subroutine to read the data that varies in the Saia Counter. Channel 5 means PIN 18 in Arduino MEGA
  Serial.println ("Start reading");           // Print the labels
  Serial.println ("The date and hour: ");
  digitalClockDisplay();                      // subroutine to display the clock in a digital form
  Serial.println();
  Serial.println();
  Serial.println ("           t             T0    T1    T2    T3    T4  ON/OFF  n");   // Header of the results
  if (myFile)                                 // if the file opens okay, write to it:
  {
    myFile.println ("Start reading");         // Save the labels in the SD
    myFile.println ("The date and hour: ");
    digitalClockSave();                       // subroutine to save the clock in the SD
    myFile.println ();
    myFile.println ("t,T0,T1,T2,T3,T4,ON/OFF,n");
  }  else  {
    digitalWrite(LED3, LOW);                  // if the SD failed to open, then turn off the status LED
    digitalWrite(LED4, HIGH);                 // if the SD failed to open, then turn on the alert LED
    Serial.println("error opening test.txt"); // if the file didn't open, print an error on the Serial Monitor
  }
//  Serial.println("trying to retrieve data from xively");
  XivelyReturn();
}

/* this is the main part of the program. the loops will keep runing infinitely until it is disconnected 
Here the main functions and subrutines are called.
It is divided in different loops according to the temperature readings */
void loop() 
{
  temp0 = get_temp(1);                        // Measure the T in order to stablish a baseline for the loop
  if (var == 1)
  {
//    Serial.println("Inside first temperature loop");
    while (temp0 >= tempON)                   // While the T is higher or equal to 0°C stay in this loop
    {
//      Serial.println("Temp ON loop");
      digitalWrite(refrigerator, HIGH);         // The refrigerator must work to cool it down, pin 13 is activated
      digitalWrite(LED0, HIGH);
      if (myFile)                               // if the file opens without problem,
      {
        digitalWrite(LED3, HIGH);               // Turn on the status LED
      }  else  {
        myFile = SD.open("DATA.txt", FILE_WRITE); // Otherwise, open the file to save the readings
        digitalWrite(LED3, HIGH);               // Turn on the status LED
      }
      Temp_Reading();                           // Start the loop of temperature reading of 5 sensors
      Serial.print (", ON");                    // The label
      if (myFile)                               // if the file opens without problem
      {
        myFile.print (", ON");                  // The Label
      }
      EnergyMeter();                            // Start the energy reading's subroutine
      XivelyResponse();                         // Start the Xively responses' subroutine
      Serial.println();
      if (myFile)                               // if the file opened without problem
      {
        myFile.println();
        myFile.close();                         // Close the file in order to store the information printed
        digitalWrite(LED3, LOW);                // Turn off the status LED
      }  else  {
        digitalWrite(LED3, LOW);                  // if the SD failed to open, then turn off the status LED
        digitalWrite(LED4, HIGH);               // Otherwise flash the Alert LED
        delay(1000);
        digitalWrite(LED4, LOW);
      }
//      Serial.println("trying to retrieve data from xively inside TempON Loop");
      XivelyReturn();
    }
//    Serial.println("Outside TempON loop");
    digitalWrite(refrigerator, LOW);          // The refrigerator could stop, pin 13 is deactivated
    digitalWrite(LED0, LOW);
    var = 2;
  }
  if (var == 2)
  {
//    Serial.println ("Enter the second loop");
    while (temp0 <= tempOFF)                           // While the T is lower to 0°C
    {
//      Serial.println("Temp OFF loop");
      pulse_number=0;                           // Reset the pulse number counter
      digitalWrite(refrigerator, LOW);          // The refrigerator could stop, pin 13 is deactivated
      if (myFile)                               // if the file opened without problem
      {
        digitalWrite(LED3, HIGH);               // Turn on the status LED
      }  else  {
        myFile = SD.open("DATA.txt", FILE_WRITE); // Otherwise, open the file to save the readings
        digitalWrite(LED3, HIGH);               // Turn on the status LED
      }
      Temp_Reading();                           // Start the loop of temperature reading of 5 sensors
      Serial.print (", OFF");                   // The label
      digitalWrite(LED0, LOW);
      if (myFile)                               // if the file opens without problem
      {
        myFile.print (", OFF");                 // The Label
      }
      EnergyMeter();                            // Start the energy reading's subroutine
      XivelyResponse();                         // Start the Xively responses' subroutine
      Serial.println();
      if (myFile)                               // if the file opened without problem
      {
        myFile.println();
        myFile.close();                         // Close the file in order to store the information printed
        digitalWrite(LED3, LOW);                // Turn off the status LED
      }  else  {
        digitalWrite(LED3, LOW);                // if the SD failed to open, then turn off the status LED
        digitalWrite(LED4, HIGH);               // Otherwise flash the Alert LED
        delay(1000);
        digitalWrite(LED4, LOW);
      }
//      Serial.println("trying to retrieve data from xively inside TempOFF Loop");
      XivelyReturn();
    }
//    Serial.println("Outside TempOFF loop");
    digitalWrite(refrigerator, HIGH);          // The refrigerator could stop, pin 13 is deactivated
    digitalWrite(LED0, HIGH);
    var = 1;  
  } 
}

/* subroutine to count the change in the SAIA counter. It uses the Attach_Interrupt function of arduino to read the changes in pulses
In this case the pulse number is counted in the pin 18 (just for Arduino Mega) */
void energy()                                 // Add one to the previous pulse number (pin 18 is ised just for Arduino MEGA)
{
  pulse_number = pulse_number + 1;  
}

/* subroutine to determine the Server Status. In this case Xively. 
Everytime the system tries to read or send a feed from the server there is an answer */
void XivelyReturn()
{
  getret = xivelyclient.get(feed, xivelyKey);
  if (getret == 200)                             // If the response es higher than 0 it means the feed couldn't be feched from the server
  {
//    Serial.println("Response acquired");
    tempON = feed[6].getFloat();
    tempOFF = feed[7].getFloat();
    Serial.print ("TempON = ");
    Serial.println(tempON);
    Serial.print ("TempOFF = ");
    Serial.println(tempOFF);
    digitalWrite(LED5, HIGH);                 // Flash the status LED
    delay(1000);
    digitalWrite(LED5, LOW);
  }  else  {
//    Serial.println("No response");
    tempON = -1.00;
    tempOFF = 10.00;
//    Serial.print ("TempON = ");
//    Serial.println(tempON);
//   Serial.print ("TempOFF = ");
//    Serial.println(tempOFF);
    digitalWrite(LED6, HIGH);                 // Flash the alert LED
    delay(1000);
    digitalWrite(LED6, LOW);
    delay(10000);                             // Wait 10 seconds between readings
  }
}
void XivelyResponse()                         // subroutine to determine the Server Status
{
  ret = xivelyclient.put(feed, xivelyKey);    // Send the feed to the server and wait for an answer
  if (ret == 200)                             // If the response es different than 200 it means the feed didn't reach the server
  {
//    Serial.println("Response acquired");
    digitalWrite(LED5, HIGH);                 // Flash the status LED
    delay(1000);
    digitalWrite(LED5, LOW);
  }  else  {
//    Serial.println("No response");
    digitalWrite(LED6, HIGH);                 // Flash the alert LED
    delay(1000);
    digitalWrite(LED6, LOW);
  }
}

/* subroutine to create a string with the temperature readings. It displays the results in the Serial Monitor and Save them in the SD */
void Temp_Reading()                           // Loop for reading the sensors
{
  String dataString = "";                     // Create a String with the values
  for (int PIN = 0; PIN < 5; PIN++)           // For loop to read from the sensor 0 to the 4 adding 1 each time
  {
    float temp = get_temp(PIN);               // Define an calculate the temperature variable 
    dataString += String(temp,2);             // Add the reading to the String
    datastreams[PIN].setFloat(temp);          
    if (PIN < 4)                              // If the sensor is between 0 and 3 then it has to separate them from the rest with a comma
    {
      dataString += ", ";
    }
  }
  digitalClockDisplay();                      // Display the clock in the Serial Monitor
  Serial.print (", ");                        // Separate this value with comma
  Serial.print(dataString);                   // Display the string of the readings
  if (myFile)                                 // If the SD file opens without problem
  {
    digitalClockSave();                       // Save the hour of the reading
    myFile.print (", ");                      // Separate the value with comma to make it easy to separate the variables in an Excel sheet
    myFile.print(dataString);                 // Save the readings in the file
  }
  temp0 = get_temp(1);                        // Renew the condition in order to activate the required loop
}

/* subroutine to include in the string and feed the number of pulses. */
void EnergyMeter()
{
  Serial.print (", ");                  
  Serial.print (pulse_number);                // Print the number of pulses so far
  datastreams[5].setFloat(pulse_number);      // Include this value in the String to be sent to the server
  if (myFile)                                 // If the SD file opens without problem
  {
    myFile.print (",");                       // Separate this value with its predecesor with comma
    myFile.print (pulse_number);              // Save the value in the SD
  }
}

/* The subroutine for displaying the date and hour in the Serial Monitor */
void digitalClockDisplay()
{
  Serial.print (hour());                      // Print the hour calling the hour function of the time library
  printDigits (minute());                     // Show the minutes in the format required calling the minute function of the time library
  printDigits (second());                     // Show the seconds in the format required calling the minute function of the time library
  Serial.print (" ");                         // Format the display
  printDate (day ());                         // Show the day in the format required calling the day function of the time library
  printDate (month ());                       // Show the minutes in the format required calling the minute funciont of the time library
  Serial.print (year ());                     // Print the year calling the hour function of the time library
}

/* The variation of the subroutine of above but for saving the date and hour in the SD */ 
void digitalClockSave()
{
  myFile.print (hour());                      // The same logic as above with the difference that now it must be saved in the file of the SD
  saveDigits (minute());
  saveDigits (second());
  myFile.print (" ");
  savePrintDate (day ());
  savePrintDate (month ());
  myFile.print (year ());
}

/* The following 4 subroutines are just to give the required format to the date and hour in the Sreial Monitor and in the SD 
The formats only define the addition of ':', the separation between hour and date and the necessity of putting a '0' before lower than 10 values*/
void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
  {
    Serial.print('0');
  }
  Serial.print(digits);
}
void saveDigits(int digits)
{
  myFile.print(":");
  if (digits < 10)
  {
    myFile.print('0');
  }
  myFile.print(digits);
}
void printDate(int digits)
{
  if (digits < 10)
  {
    Serial.print('0');
  }
  Serial.print(digits);
  Serial.print("/");
}
void savePrintDate(int digits)
{
  if (digits < 10)
  {
    myFile.print('0');
  }
  myFile.print(digits);
  myFile.print("/");
}

/* Subroutine to display the Wifi Status once it managed to connect the specified wireless network */
void printWifiStatus() 
{
  Serial.print("SSID: ");                       // print the SSID of the network you're attached to:
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();                // print your WiFi shield's IP address:
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();                      // print the received signal strength:
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

/* subroutine to read the values of the I2C sensors and change them into a celsius degree format */
float get_temp(int MCP9804_ADDRESS)             // MCP9804_ADDRESS is only a correct address if it contains the following bits 0 0 1 1 A2 A1 A0 r/w b
{         
  byte byte_1 = 0;                              // where A2, A1 and A0 are the address bits for the 8 MCP9804 sensors and r/w is the read/write bit
  byte byte_2 = 0;                              // for the data. In order to construct a correct address from the input parameter MCP9804_ADDRESS
  boolean sign_bit_set = false;
  MCP9804_ADDRESS ^= 24;                        // it has to be xor'ed with 24

  Wire.beginTransmission(MCP9804_ADDRESS);      // Begin transmission to the slave device (the digital temperature sensor with address MCP9804_ADDRESS)
  Wire.write(0x05);                             // Queue bits 0101 b for the register pointer. 0101 points to the temperature register TA
  Wire.endTransmission();                       // Transmit the pointer to the register

  Wire.requestFrom(MCP9804_ADDRESS, 2);         // Read two bytes from temperature register TA. The lower 13 bits contain the digital temperature in two's complement format
  byte_1 = Wire.read()&0x1f;                    // Read the lower 5 bits from the high byte (bit 8 up to and including bit 12)
  if (byte_1 & 0x10) 
  {
    sign_bit_set = true;
  }
  byte_2 = Wire.read();                         // Read the low byte
  if (!sign_bit_set) 
  {
    return ((byte_1 << 8) + float(byte_2))/16;  // implicit conversion to float
    Serial.print(byte_2);
  }
  else 
  {
    return float(8192 - ((byte_1 << 8) + byte_2))/-16;
  }
}

/* definitions in order to sync with the Ntp time server */
unsigned long getNtpTime()
{
  sendNTPpacket(timeServer);                    // Send an NTP packet to a time server
  delay(1000);                                  // wait to see if a reply is available
  if ( Udp.parsePacket() ) 
  { 
    Udp.read(packetBuffer,NTP_PACKET_SIZE);     // read the packet into the buffer
    unsigned long highWord = 
    word(packetBuffer[40], packetBuffer[41]);   // the timestamp starts at byte 40 of the received packet and is four bytes,
    unsigned long lowWord = 
    word(packetBuffer[42], packetBuffer[43]);   // or two words, long. First, esxtract the two words:
    unsigned long secsSince1900 = 
    highWord << 16 | lowWord;                   // combine the four bytes (two words) into a long integer
    const unsigned long seventyYears = 
    2208988800UL - timeZoneOffset;              // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    unsigned long epoch = 
    secsSince1900 - seventyYears;               // subtract seventy years
    return epoch;
  }
  return 0;
}
unsigned long sendNTPpacket(IPAddress& address) // send an NTP request to the time server at the given address
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);     // set all bytes in the buffer to 0 and initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;                 // LI, Version, Mode
  packetBuffer[1] = 0;                          // Stratum, or type of clock
  packetBuffer[2] = 6;                          // Polling Interval
  packetBuffer[3] = 0xEC;                       // Peer Clock Precision
  packetBuffer[12]  = 49;                       // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;        
  Udp.beginPacket(address, 123);                // With NTP fields given values, requests a timestamp packet NTP to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}
