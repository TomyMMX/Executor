#include <TripDetector.h>
#include <TimerOne.h>
#include <RunningAverage.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

TripDetector detector;
RF24 radio(6,10);

const int BatLedPin = 4;
const int BatStatusPin = A3;

//Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xE8E8F0F0E101, 0xF0F0F0F0D201 };

void setup() { 
  Timer1.initialize(400);         // initialize timer1, and set 100 us period
  Timer1.attachInterrupt(detectTrip);  // attaches  detectTrip() as a timer overflow interrupt
  detector.intervalLen=400;
  
  Serial.begin(9600);
  pinMode(BatLedPin, OUTPUT);
  digitalWrite(BatLedPin, LOW);

  // set up the ADC
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library

  // you can choose a prescaler from above.
  // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_16;    // set our own prescaler to 16  

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
}

void checkBattery(){  
    int reading = analogRead(BatStatusPin);  
    //Serial.println(reading);
    if(reading<400){
        digitalWrite(BatLedPin, LOW);  
    }
    else{
        digitalWrite(BatLedPin, HIGH);  
    }  
}

void setLaserAccuracy(long interval){
 Timer1.setPeriod(interval);
 detector.intervalLen = interval;  
  
 // Timer1.setPeriod(40000);
 // detector.intervalLen = 40000;  
}

unsigned long started_waiting_at=0;
unsigned long prevTripStart = 0;
unsigned long prevTripSpeed = 0;
boolean waitForResponse = false;
int timeoutCount = 0;
boolean newTrip = false;
boolean trippedBefore = false;

void loop() {   
  if(newTrip)  { 
    newTrip = false; 
    
    unsigned long times[3];
    times[0] = 5;         
    times[1] = millis()-prevTripStart + 1; //the 1 compensates for wirelles delay
    times[2] = prevTripSpeed;
  
    //Serial.println(times[1]);
  
    radio.stopListening(); 
    boolean ok = radio.write(times, sizeof(times));
    radio.startListening();
  
    if(ok==true){
      waitForResponse = true;
      started_waiting_at = millis();       
    }
    else{
      Serial.println("ERROR!");
      setLaserAccuracy(400);
    }      
  } 
  
  bool timeout = false;
  bool doRead = false;
  
  if(waitForResponse){
    if(radio.available()){
      doRead=true;
    }
    else if (millis() - started_waiting_at > 20 ){
      timeout = true;
    }
  }      
    
  if ( timeout || doRead ){  
    if(doRead){        
      unsigned long got_time[3];
      radio.read( got_time, sizeof(got_time) );   
        
      if(got_time[0]!=5){
        //some other data, not for us
      }     
      else{
        radio.stopListening(); 
        
        waitForResponse=false;        
        
        setLaserAccuracy(400);
        
        Serial.println("TRIP REPLY!");
        /*Serial.print("Data: ");               
        Serial.println(got_time[1]);
        Serial.print("round-trip delay: ");          
        unsigned long roundtripDelay = millis()-prevTripStart-got_time[1];          
        Serial.println(roundtripDelay);*/
      }
    }
    if(timeout){
      timeoutCount++;  
        
      if(timeoutCount<100){
        newTrip = true;          
      }        
      else{
        Serial.println("TRIP SEND TIMEOUT! -- GAVE UP");
        waitForResponse=false;
        
        setLaserAccuracy(400);
      }
    }          
  } 
}

int interruptCount;

//called from the Timer1 interrupt
void detectTrip(){
  interruptCount++;
   
  detector.DetectTrip(); 
  
  if(detector.tripEnded)  {
    
    if(detector.isTripped && !trippedBefore){
      //new trip
      setLaserAccuracy(4000);    
      Serial.println("TRIP TO SEND DETECTED!");
      newTrip=true;
      timeoutCount = 0;
      prevTripStart = detector.TripStartTime;     
      prevTripSpeed = detector.StoneSpeed; 
    }  

    trippedBefore=detector.isTripped; 
  }
    
  if(interruptCount>4000){
    interruptCount=0;
    checkBattery();  
  }
}


