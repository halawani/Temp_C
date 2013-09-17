#include <Time.h>
#include <SD.h>
#include <Wire.h>

int sentDat;
// Definitions for the SD card
File myFile;
// Definitions for the Temperature Reading
int refrigerator = 9;
volatile float temp0;
volatile float temp1;
volatile float temp2;
volatile float temp3;
volatile float temp4;
//float average;
// Definitions for the Energy Reading
volatile float energy_consumed;
float energy_consumed1 = 0;
float conversion_kWH = 1000;
int t = 0;
volatile int pulse_number = 0;

void setup() 
{
  Serial.begin(9600);                                          // Start the Serial Monitor with 9600 bps
  Wire.begin();                                                // Open library for the sensor
//  pinMode(9, OUTPUT);                                         // Pin 10 (The refrigerator) is stablished as output (10 due to the microSD)
  Serial.println("Welcome, send the command:");                // Start of the program in the Serial Monitor
  Serial.println("'s' to turn on the program");                // Press "s" to start the loop without saving
  Serial.println("'v' to turn on and save the results");       // Press "v" to start the loop saving the results in the SD
  Serial.println("'h' to check the conection with the board"); // Press "h" to light the LED in the 13 pin
  Serial.println("'t' to close the file and save");            // Press "t" to stop the loop saving the results in the SD
  Serial.println("'c' to check the sensors in the string");    // Press "c" to check if the sensors in the string are working
  attachInterrupt (5, energy, RISING);                         // Subrutine to read the data that varies in the Saia Counter
  energy_consumed1 = energy_consumed;
}

void loop() 
{
  if (Serial.available() > 0)                        // Check if there has been an input in the Serial Monitor
  {                                                
    sentDat = Serial.read();                         // If true, read the input, Start the if loops                                          
    if(sentDat == 's')                               // Start the program without saving the values in the SD
    {                            
      Serial.println ("Start reading");
      Serial.print ("The date and hour: ");
      digitalClockDisplay();
      Serial.println();
      Serial.println ("   t      T0    T1    T2    T3    T4  ON/OFF  n   E");   // Header of the results
      while (Serial.available() <= 0)                // Make a stop of the loops, while there is no input in the serial monitor
      {
        temp0 = get_temp(0);                         // Measure the T in order to stablish a baseline for the loop
        while (temp0 >= 30 && Serial.available() <= 0)// While the T is higher or equal to 30°C & there is no input
        {
          digitalWrite(refrigerator, LOW);          // The refrigerator must work to cool it down, pin 13 is activated
          Temp_Reading();
          Serial.print ("  ON");                     // The label
          if (energy_consumed != energy_consumed1)   // Establish a change in the energy meter
          {
            EnergyMeter();
            energy_consumed1 = energy_consumed;      // Renew the condition of the loop to start next time
          }
          Serial.println();
          delay(2000); 
        }
        while (temp0 < 30 && Serial.available() <= 0)// While the T is lower to 30°C & there is no input
        {
          digitalWrite(refrigerator, HIGH);           // The refrigerator could stop, pin 13 is deactivated
          Temp_Reading();
          Serial.print ("  OFF");                    // The label
          Serial.print (pulse_number);
          if (energy_consumed != energy_consumed1)   // Establish a change in the energy meter
          {
            EnergyMeter();
            energy_consumed1 = energy_consumed;      // Renew the condition of the loop to start next time
          }
          Serial.println();
          delay(2000);
        }
      }
      Serial.println ("Stop");
      Serial.println ();
    }
    if(sentDat == 'v')                               //Start the program saving the results in the SD
    {
      Serial.print("Initializing SD card...");       // Open Communication with the SD. On the Ethernet Shield, CS is pin 4. It's set as an output by default.
      pinMode(10, OUTPUT);                           // Note that even if it's not used as the CS pin, the hardware SS pin (10 on most Arduino boards, 53 on the Mega) must be left as an output or the SD library functions will not work.
      if (!SD.begin(4))
      {  
        Serial.println("initialization failed!");    // Error message if the SD could not be reached.
        return;
      }
//      Serial.println("initialization done.");        // Correct message if the SD was reached.
      myFile = SD.open("test.txt", FILE_WRITE);      // open the file. note that only one file can be open at a time, so you have to close this one before opening another. 
      while (Serial.available() <= 0)              // Make a stop of the loops, while there is no input in the serial monitor
      {
        Serial.println ();                       // Start printing the results in the Serial Monitor
        myFile.println ();
        Serial.println ("   t      T0    T1    T2    T3    T4  ON/OFF  n   E");// Header of the results
        myFile.println ("t,T0,T1,T2,T3,T4,ON/OFF,n,E");
        temp0 = get_temp(0);
        while (temp0 >= 30 && Serial.available() <= 0)// While the T is higher or equal to 30°C & there is no input
        {
          digitalWrite(refrigerator, LOW);          // The refrigerator must work to cool it down, pin 13 is activated
          Temp_Reading();
          Serial.print ("  ON");                     // The label
          myFile=SD.open("test.txt", FILE_WRITE);
          if (myFile)                                    // if the file opened okay, write to it:
          {
            Temp_Saving();
            myFile.print ("ON,");
          }
          if (energy_consumed != energy_consumed1)   // Establish a change in the energy meter
          {
            EnergyMeter();
            if (myFile)                                    // if the file opened okay, write to it:
            {
              EnergySaver();
            }
            energy_consumed1 = energy_consumed;      // Renew the condition of the loop to start next time
          }
          Serial.println();
          if (myFile)                                    // if the file opened okay, write to it:
          {
            myFile.println();
            myFile.close();
          }
          delay(2000); 
        }
          while (temp0 < 30 && Serial.available() <= 0)// While the T is lower to 30°C & there is no input
          {
            digitalWrite(refrigerator, HIGH);           // The refrigerator could stop, pin 13 is deactivated
            myFile=SD.open("test.txt", FILE_WRITE);
            Temp_Reading();
            Serial.print ("  OFF");                    // The label
            Temp_Saving();
            myFile.print ("OFF,");
            if (energy_consumed != energy_consumed1)   // Establish a change in the energy meter
            {
              EnergyMeter();
              EnergySaver();
              energy_consumed1 = energy_consumed;      // Renew the condition of the loop to start next time
          }
            Serial.println();
            myFile.println();
            myFile.close();
            delay(2000);
          }
        }
      }
    if (sentDat == 't')                                // Activate the LED for check purposes.
    {
//      myFile.close();                                  // close the file:
//      Serial.println ();                             // Start printing the results in the Serial Monitor
//      Serial.println("File saved and closed: done.");      // Show in the Serial Monitor that the file had closed
      SD.begin(4);
      File myFile = SD.open("test.txt");

      // if the file is available, write to it:
      if (myFile)
      {
        while (myFile.available()) 
        {
        Serial.write(myFile.read());
        }
      myFile.close();
      }  
  // if the file didn't open, pop up an error:
      else 
      {
        Serial.println("error opening datalog.txt");
      } 
    }
    
    if(sentDat == 'h')                                // Activate the LED for check purposes.
    {      
      while (Serial.available() <= 0)
      {
        digitalWrite(9, HIGH);
        delay(1000);
        digitalWrite(9, LOW);
        delay(1000);
      }
    }
    if(sentDat == 'c')                               // Start the program without saving the values in the SD
    {                            
      Serial.println ();
      Serial.print ("Start reading");
      Serial.println ();
      while (Serial.available() <= 0)                // Make a stop of the loops, while there is no input in the serial monitor
      {
        ClockDisplay();
        Serial.println ();
        temp0 = get_temp(0);
        temp1 = get_temp(1);
        temp2 = get_temp(2);
        temp3 = get_temp(3);
        temp4 = get_temp(4);
        Serial.print ("Sensor-0");
        Serial.print ("  ");
        Serial.print ("Sensor-1");
        Serial.print ("  ");
        Serial.println ("Sensor-2");
        Serial.print (temp0,2);                    // Temperature with 2 decimal places (float)
        Serial.print ("     ");
        Serial.print (temp1,2);                    // Temperature with 2 decimal places (float)
        Serial.print ("     ");
        Serial.print (temp2,2);                    // Temperature with 2 decimal places (float)
        Serial.println ();
        Serial.print ("Sensor-3");
        Serial.print ("  ");
        Serial.println ("Sensor 4");
        Serial.print (temp3,2);        // Temperature with 2 decimal places (float)
        Serial.print ("     ");
        Serial.print (temp4,2);        // Temperature with 2 decimal places (float)
        Serial.println ();
        delay(2000);        
        Serial.println ();
//        average = (temp0 + temp1 + temp2 + temp3 + temp4)/5;
//        Serial.println ("Average");
//        Serial.println (average);
//        delay(2000);
      }
    }
  }
}

void energy()
{
  pulse_number = pulse_number + 1;
  energy_consumed = float(pulse_number)/conversion_kWH;
}

void Temp_Reading()
{
  String dataString = "";
  for (int PIN = 0; PIN < 5; PIN++)
  {
    float temp = get_temp(PIN);
    dataString += String(temp,2);
    if (PIN < 4)
    {
      dataString += " ";
    }
  }
  ClockDisplay();
  Serial.print (" ");
  Serial.print(dataString);
}

void Temp_Saving()
{
//  temp0 = get_temp(0);                       // T is measured again.
//  temp1 = get_temp(1);
//  temp2 = get_temp(2);
//  temp3 = get_temp(3);
//  temp4 = get_temp(4);
  String dataString = "";
  for (int PIN = 0; PIN < 5; PIN++)
  {
    float temp = get_temp(PIN);
    dataString += String(temp,2);
    if (PIN < 4)
    {
      dataString += ",";
    }
  }
  SaveClock();
  myFile.print (",");
  myFile.print(dataString);
//  myFile.print (temp0,2);
//  myFile.print (",");
//  myFile.print (temp1,2);
//  myFile.print (",");
//  myFile.print (temp2,2);
//  myFile.print (",");
//  myFile.print (temp3,2);
//  myFile.print (",");
//  myFile.print (temp4,2);
//  myFile.print (",");
}

void EnergyMeter()
{
  Serial.print ("     ");                  
  Serial.print (pulse_number);             // Print the number of pulses so far
  Serial.print ("     "); 
  Serial.print (energy_consumed,3);        // Print the energy consumed with 3 decimal places (float)
}

void EnergySaver()
{
  myFile.print (pulse_number);
  myFile.print (",");
  myFile.print (energy_consumed,3);
  myFile.print (",");
}

void digitalClockDisplay()
{
  Serial.print (hour());
  printDigits (minute());
  printDigits (second());
  Serial.print (" ");
  printDate (day ());
  printDate (month ());
  Serial.print (year ());
  Serial.println();
}

void ClockDisplay()
{
  Serial.print (hour());
  printDigits (minute());
  printDigits (second());
}

void SaveClock()
{
  myFile.print (hour());
  saveDigits (minute());
  saveDigits (second());
}

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
