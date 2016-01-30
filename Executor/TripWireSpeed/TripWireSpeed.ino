#include <TripDetector.h>
#include <TimerOne.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

TripDetector detector("MAIN", 2, A1, 3);
TripDetector detector2("SLAVE", 9, A2, 5);

RF24 radio(6,10);

const long accuracy = 250;

const int BatLedPin = 5;
const int BatStatusPin = A3;

//Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xE8E8F0F0E101, 0xF0F0F0F0D201 };

void setup() { 
  detector2.StopDetect(); 

  Serial.begin(9600);
  pinMode(BatLedPin, OUTPUT);
  digitalWrite(BatLedPin, LOW);

  // set up the ADC
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library

  // you can choose a prescaler from above.
  // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_16;    // set our own prescaler to 16  
  
  Timer1.initialize(accuracy);         // initialize timer1, and set 100 us period
  Timer1.attachInterrupt(detectTrip);  // attaches  detectTrip() as a timer overflow interrupt

  radio.begin();
  radio.setChannel(125);
  radio.setAutoAck(false);
  radio.setRetries(15,15);
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(8);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_8);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);  
  radio.startListening();
  
  Serial.println("STARTUP!");
}

void checkBattery(){  
  int reading = analogRead(BatStatusPin);  
  //Serial.println(reading);
  if(reading<400){
    digitalWrite(BatLedPin, HIGH);  
  }
  else{
    digitalWrite(BatLedPin, LOW);  
  }  
}

void setLaserAccuracy(long interval){
  Timer1.setPeriod(interval);
}

unsigned long started_waiting_at=0;
unsigned long prevTripStart = 0;
unsigned long prevTripSpeed = 0;

unsigned long prevTripStart1 = 0;
unsigned long prevTripStart2 = 0;

unsigned long lastBatCheck = 0;

boolean waitForResponse = false;
int timeoutCount = 0;
boolean newTrip = false;
boolean newSpeed = false;

void loop() {   
  
  //check battery state
  if(millis() - lastBatCheck > 4000){
    lastBatCheck = millis();
    checkBattery();  
  }
  
  
  if(newSpeed){
    Serial.println("TRIP TO SEND DETECTED!");
    newSpeed = false;
    
    Serial.print("T1: ");
    Serial.println(prevTripStart1); 
    
    Serial.print("T2: ");
    Serial.println(prevTripStart2); 
    
    Serial.print("Speed: ");
    prevTripSpeed = 34000000 / (prevTripStart2 - prevTripStart1);
    Serial.println(prevTripSpeed); 
  }  
  
  if(newTrip)  { 
    newTrip = false; 

    unsigned long times[3];
    times[0] = 0;         
    times[1] = prevTripSpeed; //the 1 compensates for wirelles delay
    times[2] = 0;

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
      setLaserAccuracy(accuracy);
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

      if(got_time[0]!=0){
        //some other data, not for us
      }     
      else{
        radio.stopListening(); 

        waitForResponse=false;        

        setLaserAccuracy(accuracy);

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

        setLaserAccuracy(accuracy);
      }
    }          
  } 
}

boolean doMain = true;
boolean doCheck = true;

//called from the Timer1 interrupt
void detectTrip(){
  if(doMain){
    detector.DetectTrip();    
 
    if(detector.isTripped && doCheck){    
      if(doMain){        
        prevTripStart1 = detector.TripStartMicros;
        detector.TurnLaserOff();
      }
      doMain = false;            
    } 
    else if(!detector.isTripped && !doCheck){
      //trip went away
      doCheck = true;
    }
  }else{
    detector2.DetectTrip();
    
    if(detector2.isTripped && doCheck){
      prevTripStart2 = detector2.TripStartMicros;
      
      doCheck = false;              
      doMain = true;
      newSpeed = true;
                  
      detector2.StopDetect(); 
    }   
  }
  
  if(newSpeed)  {
      setLaserAccuracy(5000);    
      newTrip=true;
      timeoutCount = 0; 
  }  
}



