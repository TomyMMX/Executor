#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(6,10);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xE8E8F0F0E101, 0xF0F0F0F0D201 };

boolean running = false;
unsigned long SlideTime = 0;
unsigned long Speed = 0;
unsigned long StartTime = 0;
unsigned long LastChange = 0;
unsigned long LastSpeedShow = 0;
int no = 0;

//Pin Assignments (You should change these)
const int CLK       = 9;           //Connected to TPIC pin 13: SRCLK (aka Clock)
const int LATCH     = 4;          //Connected to TPIC pin 12: RCLK (aka Latch/load/CS/SS...)
const int OE        = 5;          //Connected to TPIC pin 9: OE (Output Enable)
const int DOUT      = 3;          //Connected to TPIC pin 3: SER (aka MOSI)

//Number Patterns (0-9)
//***Drains 0-7 must be connected to segments A-DP respectively***
const byte numTable[] =
{
  B01110111,
  B01000001,
  B00111011,
  B01101011,
  B01001101,
  B01101110,
  B01111110,
  B01000011,
  B01111111,
  B01101111,
  B11110111,
  B11000001,
  B10111011,
  B11101011,
  B11001101,
  B11101110,
  B11111110,
  B11000011,
  B11111111,
  B11101111,
  B01111000,
};

byte number[3];

void setup()
{
  Serial.begin(9600);  
  //Serial1.begin(9600);

  //Set pin modes
  pinMode(CLK,OUTPUT);
  pinMode(LATCH,OUTPUT);
  pinMode(DOUT, OUTPUT);
  pinMode(OE, OUTPUT);

  //7-Segment Display Init
  digitalWrite(OE,LOW);        //Enables SR Operation
  
  StartTime=millis(); 
    
  Serial.println("STARTING RADIO!");
  radio.begin();
  radio.setChannel(125);
  radio.setAutoAck(false);
  radio.setRetries(15,15);
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(8);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_8);  
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);  
  radio.startListening();    
 
  clearDisplay(); 
}

void initializeSRData()
{
  //Display Scanner (Iterates through each display module)
  digitalWrite(LATCH,LOW);      //Tells all SRs that uController is sending data

    //Digit Scanner (Iterates through each SR (digit) in a display module)
    for(int digit = 0; digit < 4; digit++)
    {
      //Clears any garbage on the serial line
      shiftOut(DOUT,CLK,LSBFIRST,0);          //Shift out 0s to all displays
      //number[digit] = 0;              //Stores a 0 for each digit so its completely off
    }
  
  digitalWrite(LATCH,HIGH);      //Tells all SRs that uController is done sending data
}

void clearDisplay()
{
  initializeSRData();
  refreshDisplay();
}

int digit = 2;
void refreshDisplay()
{   
  //for(int digit = 2; digit > -1; digit--)
  {  
    digitalWrite(LATCH,LOW);
    //Pre-Digit blanker (shifts out 0s to correct digits before sending segment data to desired digit)
    for(int blanks = (3 - 1 - digit); blanks > 0; blanks--)
      shiftOut(DOUT,CLK,LSBFIRST,0);

    shiftOut(DOUT,CLK,LSBFIRST,number[digit]);

    //Post-Digit blanker (shifts out 0s to remaining digits)
    for(int blanks = digit; blanks > 0; blanks--)
      shiftOut(DOUT,CLK,LSBFIRST,0);
      
    digitalWrite(LATCH,HIGH);
  } 
  
  initializeSRData();
  
  if(digit==2) {
    digit = 1;
  }else if(digit==1){
    digit=0;
  }else if(digit==0){
    digit = 2;
  }  
}

unsigned long LastSerial = 0;
unsigned long timeOfStop = 0;
unsigned long lastDisplayShow = 0;

void recalculateDisplayNumbers()
{  
  if(running)
  {
    SlideTime=millis()-StartTime;
    LastSpeedShow=millis();
  }
  
  //
  
  if((!running || SlideTime<1000 || SlideTime>5000) && millis()-timeOfStop<20000)   
     {     
      /* if(millis()-LastSpeedShow>2000){
         if(millis()-LastSpeedShow>3500){
           LastSpeedShow=millis();
         }
         
         
         if(Speed<1000){
           number[2] = numTable[(Speed/100)%10];
           number[1] = numTable[(Speed/10)%10];
           number[1] = numTable[Speed%10];
         }
         else{
           number[2] = numTable[20];
           number[1] = numTable[20];
           number[0] = numTable[20];
         }         
       }else{*/
         long sec1 = SlideTime/10000;  
         if(sec1<1)
         {   
           number[2] = numTable[(SlideTime/1000)%10+10];
           number[1] = numTable[(SlideTime/100)%10];
           if(running){
             number[0]= 0;
           }
           else{
             number[0] = numTable[(SlideTime/10)%10];
           }
         }
         else{
           number[2] = numTable[sec1%10];
           number[1] = numTable[(SlideTime/1000)%10+10];
           number[0] = numTable[(SlideTime/100)%10];
         }
      // }
    }
    else
    {
      number[0] = 0;
      number[1] = 0;
      number[2] = 0;
    }
}

unsigned long lastDisplayPause = 0;
void loop()
{ 
  //
 
  if(radio.available())
  {    
    Serial.println("Data on RADIO!");
    
    //Serial.println("GOT DATA!");
    unsigned long got_time1[3];
    
    bool done = false;
    while (!done)
    {
      done = radio.read(got_time1, sizeof(got_time1) );   
    }
    
    Serial.println("Data READ!");
    
    if(got_time1[0]==1||got_time1[0]==0)
    {
      unsigned long send_time[3];
      send_time[0]=got_time1[0];
      send_time[1]=got_time1[1];
      send_time[2]=got_time1[2];
    
      radio.stopListening();  
      bool ok = radio.write(send_time, sizeof(send_time) );  
      radio.startListening();  
      
      if(ok){
        Serial.println("SENT REPLY!");
      }    
      
      if(got_time1[0]==1)
      {
        Serial.println("BT - START");
        Serial.println("START!");
        running=true;
        timeOfStop = millis();
        StartTime=millis()-got_time1[1];
      }
      else if(got_time1[0]==0)
      {
        running = false;
        timeOfStop = millis();
        SlideTime=got_time1[1];
        Speed = got_time1[2];
        Serial.println("BT - STOP");
        Serial.println("STOP!");
        
        //"BT" so we know this is important for BlueTooth devices
        Serial.print("BT - TIME: ");
        Serial.println(SlideTime);        
      }   
    }
  }
  
  if(millis()-LastChange>100)
  {
    LastChange=millis();    
    recalculateDisplayNumbers();    
  }
  
  
  
  if(millis()-lastDisplayPause>1) {
    lastDisplayPause=millis();
    refreshDisplay();
  }
}

