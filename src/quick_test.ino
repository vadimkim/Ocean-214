// SI473x Quick Test
// Simple test program to accompany the article
// Build an Arduino-Controlled AM/FM/SW Radio
// DrG 2016
//
//*****************************************************
//** This software is offered strictly as is with no **
//** warranties whatsoever. Use it at your own risk. **
//*****************************************************
//
#include <Wire.h>
// chip I2C address is 10001b if !SEN = 0
#define SI_ADDRESS 0x11

// Si Commands
#define POWER_UP        0x01
#define GET_REV         0x10
#define POWER_DOWN      0x11
#define SET_PROPERTY    0x12
#define GET_INT_STATUS  0x14
#define FM_TUNE_FREQ    0x20
#define FM_SEEK_START   0x21
#define FM_TUNE_STATUS  0x22

// Command args
#define POWER_UP_ARG1   0xC0
#define POWER_UP_ARG2   0x05


// Globals
// Arduino Pins D2=IRQ D12=reset
const byte INTpin=2;
const byte RESETpin=12;
byte power=0;
byte error=0;
byte status_rsp[16];
volatile byte IRQflag;

void setup() {
  // set up pins and IRQ
  Serial.begin(115200); 
  // interrupt active HIGH
  pinMode(INTpin, INPUT);
  pinMode(RESETpin,OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(0,SiISR,FALLING); 
}

void loop() {
  if (power == 0 && error == 0) {
    delay(1000);  // wait for serial monitor click
    Serial.println("*** Si473x Quick Test **");
    Serial.println("...resetting Si4735");
    SiReset();
    Serial.println("Si4735 reset complete");      
    
    // send POWER_UP
    Serial.println("...sending POWER_UP to boot from device memory and setting IRQ");
    Wire.beginTransmission(SI_ADDRESS);
    Wire.write(POWER_UP);  
    Wire.write(POWER_UP_ARG1);
    Wire.write(POWER_UP_ARG2);
    byte result = Wire.endTransmission();
    
    // wait for crystal to stabilize at least 500ms, but it is probably better to use IRQ to check CTS
    // delay(700);
    IRQflag=0; 
    while(!IRQflag);
    
    if (result == 0) {
      Serial.println("Si4735 POWER_UP completed");
    } else {
      Serial.print("Error during Si4735 POWER_UP : ");
      Serial.print(result);
      Serial.println();
      error = 1;
    }
    power = 1;

    setProperties();
    getRevision();
    // fmTune100();
    fmSeekStart();
  
  } else if (error == 1) {
    // Error, blink the led
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);                      
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);  
  } else {
    // After successful Power UP lit the LED first
    digitalWrite(LED_BUILTIN, HIGH);
  }
  /*
  // uncomment if you want to see the response bytes
  for(int x=0;x<4;x++)
  {
    Serial.print("Response byte ");
    Serial.print(x);
    Serial.print("= ");
    Serial.println(status_rsp[x],BIN);
  }
  */
//  Serial.print("Band Mode is ");
//  bandmode=status_rsp[1]>>6;
//  Serial.print(bandmode);
//  Serial.println(" [0=FM 1=AM 2=SW]");
//  Serial.print("Band Index is ");
//  band=status_rsp[1]&B00011111;
//  Serial.print(band);
//  Serial.println(" [0-19 FM / 20-24 AM / 25-40 SW]");
//  // print the frequency digits
//  Serial.print("Tuner Frequency (mHz)= ");
//  Serial.print(status_rsp[2]>>4);
//  Serial.print(status_rsp[2]%16);
//  Serial.print(status_rsp[3]>>4);
//  Serial.print(".");
//  Serial.println(status_rsp[3]%16);
//  Serial.println();
//  Serial.println("Chip information...");
//  Wire.beginTransmission(SI_ADDRESS);
//  Wire.write(GET_REV);  
//  Wire.endTransmission();  
//  delay(5);  // should really check for CTS but this works
//  Wire.requestFrom(SI_ADDRESS, 0x09);
//  for(int x=0;x<9;x++)
//  {
//    status_rsp[x]=Wire.read();
//  }
//  IRQflag=0;  
//  Serial.print("Final two digits of part number: ");
//  Serial.println(status_rsp[1]);
//  Serial.print("Firmware revision: ");
//  Serial.print((char)status_rsp[2]);
//  Serial.print(".");
//  Serial.println((char)status_rsp[3]);
//  Serial.print("Component revision: ");
//  Serial.print((char)status_rsp[6]);
//  Serial.print(".");
//  Serial.println((char)status_rsp[7]);
//  Serial.print("Chip revision: ");
//  Serial.println((char)status_rsp[8]);
//  Serial.println();
//  
//  Serial.println("Entering tuning loop...");
//  IRQflag=0;
//  tuneloop:
//  if(IRQflag)
//   {
//     Wire.beginTransmission(SI_ADDRESS);
//     Wire.write(ATDD_GET_STATUS);  
//     Wire.endTransmission();  
//     Wire.requestFrom(SI_ADDRESS, 0x04);
//     status_rsp[0]=Wire.read();
//     status_rsp[1]=Wire.read();
//     status_rsp[2]=Wire.read();
//     status_rsp[3]=Wire.read();
//     // print the frequency digits
//     Serial.print(status_rsp[2]>>4);
//     Serial.print(status_rsp[2]%16);
//     Serial.print(status_rsp[3]>>4);
//     Serial.print(".");
//     Serial.println(status_rsp[3]%16);
//     IRQflag=0;
//   }
//   goto tuneloop;
}

void SiReset()
{
  // reset Si4735 and back up
  digitalWrite(RESETpin,LOW);
  delayMicroseconds(200);
  digitalWrite(RESETpin,HIGH);
  delayMicroseconds(200);
}

void getRevision() {
    Serial.println("...Getting chip info");
    IRQflag=0; 
    Wire.beginTransmission(SI_ADDRESS);
    Wire.write(GET_REV);
    Wire.endTransmission();
    while(!IRQflag);
    
    Wire.requestFrom(SI_ADDRESS, 0x09);
    int x=0;
    while(Wire.available())    // slave may send less than requested
    {
      status_rsp[x] = Wire.read();    // receive a byte
      x++;
    }
    Serial.println();
    Serial.print("Final two digits of part number: ");
    Serial.println(status_rsp[1]);
    Serial.print("Firmware revision: ");
    Serial.print((char)status_rsp[2]);
    Serial.print(".");
    Serial.println((char)status_rsp[3]);
    Serial.print("Component revision: ");
    Serial.print((char)status_rsp[6]);
    Serial.print(".");
    Serial.println((char)status_rsp[7]);
    Serial.print("Chip revision: ");
    Serial.println((char)status_rsp[8]);
    Serial.println(); 
}

void getIntStatus() {
  Serial.println("...Reading interrupt status");
  waitstat:
  Wire.beginTransmission(SI_ADDRESS);
  Wire.write(GET_INT_STATUS);  
  Wire.endTransmission();  
  delay(1000);
  Wire.requestFrom(SI_ADDRESS, 0x01);
  
  int status = Wire.read();    // receive a byte
  Serial.println(status);
 
  if (status != 0x81) {
    goto waitstat;
  }
  Serial.println("STC is set");
}

// The interrupt service routine
void SiISR()
{
    IRQflag=1;
}

void setProperties() {
    // Set propertie  GPO_IEN (0x0001)
    // 2 bytes map is
    // 0 0 0 0 RSQREP RDSREP 0 STCREP | CTSIEN ERRIEN 0 0 RSQIEN RDSIEN 0 STCIEN
    // 0x00 0x81
    Wire.beginTransmission(SI_ADDRESS);
    Wire.write(SET_PROPERTY);  
    Wire.write(0x00);  // ARG1 always = 0
    Wire.write(0x00);   // ARG2 
    Wire.write(0x01);   // ARG3
    Wire.write(0x00);   // ARG4
    Wire.write(0x81);   // ARG5 -- enable CTS and STC
    Wire.endTransmission();    
}

void fmTune100() {
    Serial.println("...Tune to 100Mhz");

    Wire.beginTransmission(SI_ADDRESS);
    Wire.write(FM_TUNE_FREQ);
    Wire.write(0x00); // ARG1
    Wire.write(0x27); // Hi byte
    Wire.write(0x10); // Lo byte 
    Wire.write(0x00); // Antenna cap automatic selection
    Wire.endTransmission();

 //   getIntStatus(); 
     getTuneStatus();
}

void getTuneStatus() {
    IRQflag=0; 
    Wire.beginTransmission(SI_ADDRESS);
    Wire.write(FM_TUNE_STATUS);
    Wire.write(0x00); // ARG1
    Wire.endTransmission();
    while(!IRQflag);
    
    Wire.requestFrom(SI_ADDRESS, 0x06);
    int x=0;
    while(Wire.available())    // slave may send less than requested
    {
      status_rsp[x] = Wire.read();    // receive a byte
      x++;
    }
      
    Serial.print("Seek status is: ");
    for (int i=0; i<6; i++) {
      Serial.print(status_rsp[i]);
      Serial.print(" ");
    }
    Serial.println();
    delay(1000);
}

void fmSeekStart() {
    Serial.println("...Start seeking.... ");
    Wire.beginTransmission(SI_ADDRESS);
    Wire.write(FM_SEEK_START);
    Wire.write(0x0C); // ARG1
    Wire.endTransmission();
    //getIntStatus();
}
    
