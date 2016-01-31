#include <TimerOne.h>
#include <TripDetector.h>
#include <RunningAverage.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

const int BatLedPin = 4;
const int BatStatusPin = A3;

boolean IsTripped[] = {false, false};
unsigned long TripStartTime[] = {0, 0};
unsigned long TripSpeed[] = {0,0};

boolean doSendTime = false;

TripDetector detector("MAIN", 5, A2, 3);
RF24 radio(6,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xE8E8F0F0E102, 0xF0F0F0F0D202 };

void setup() {
  
  Timer1.initialize(2000);         // initialize timer1, and set 100 us period
  Timer1.attachInterrupt(detectTrip);  // attaches  detectTrip() as a timer overflow interrupt
  //detector.intervalLen=2000;
  
  Serial.begin(9600);

  pinMode(BatLedPin, OUTPUT);
  digitalWrite(BatLedPin, HIGH);
  
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
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);  
  radio.startListening();
}
boolean canDetectTrip = false;

const long minTime = 5000;
const long maxTime = 99990;
unsigned long SlideTime = 0;
unsigned long TeeLineTime = 0;
boolean isTimeRunning = true;
unsigned long lastTeeBrake=0;

int first = 1;
int second = 0;

unsigned long lastStopTime=0;

void calculateCurrentTime()
{
  if((TeeLineTime==0 || SlideTime>minTime) && millis()-lastTeeBrake>1000)
  {
    if(!isTimeRunning){
      if(millis()-lastStopTime>5000){
        if(IsTripped[1]&&TripStartTime[1]!=lastStopTime){
          Serial.println("WIRE ORDER: 1 -> 0");
          first=1;
          second=0;
        }
        else if(IsTripped[0]&&TripStartTime[0]!=lastStopTime){
          Serial.println("WIRE ORDER: 0 -> 1");
          first=0;
          second=1;
        }
      }
    }
    
    if(IsTripped[first]){
      Serial.print("START: ");
      Serial.println(first);
      TeeLineTime=TripStartTime[first];
      lastTeeBrake=millis();
      isTimeRunning=true;
      doSendTime=true;
    }
  }
  
  if(TeeLineTime!=0)
  {
    SlideTime=millis()-TeeLineTime;
    if(IsTripped[second]||SlideTime>maxTime){
      if(IsTripped[second]){
        SlideTime=TripStartTime[second]-TeeLineTime;
      }
      if(SlideTime>maxTime){
        SlideTime=0;
      }
      
      lastStopTime=TripStartTime[second];
      
      Serial.print("STOP: ");
      Serial.println(second);
       Serial.print("SlideTime: ");
      Serial.println(SlideTime);
      
      TeeLineTime=0;
      isTimeRunning=false;
      Timer1.setPeriod(2000);
      //detector.intervalLen=2000;
      canDetectTrip = false;
      doSendTime=true;
    }    
  }   
  
  if(SlideTime>minTime){
    Timer1.setPeriod(2000);
    //detector.intervalLen=2000;
    canDetectTrip = false;
  }
}
unsigned long sendCount=0;
unsigned long lastRemoteSend = 0;
boolean waitForResponse = false;
void sendTimeToRemoteDisplay()
{     
  unsigned long times[3];
  if(isTimeRunning)
    times[0] = 1;
  else
    times[0] = 0;
     
  times[1] = SlideTime;   
  times[2] = TripSpeed[second];  
  
  radio.stopListening();
  bool ok = radio.write(times, sizeof(times)); 
  radio.startListening();
  
  if(ok){
    lastRemoteSend = millis();
    waitForResponse=true;
    doSendTime=false;
    Serial.println("SENT TIME!");
  }
}

unsigned long remoteBlinkStart= 0;

void readDataOnWireless(){
 if(radio.available()){
    unsigned long got_time[3];
    radio.read(got_time, sizeof(got_time) );   
   
     //Serial.println("DATA!!!!!!!!!");
   
    //Remote TRIP
    if(got_time[0]==5){
      Serial.println("DATA: Remote TRIP!");
      digitalWrite(BatLedPin, LOW);         
      remoteBlinkStart=millis();
      
      IsTripped[1]=true;
      TripStartTime[1] = millis()-got_time[1];
      TripSpeed[1] = got_time[2];
      Serial.print("TRIP SPEED 1: ");
      Serial.println(TripSpeed[1]);
      radio.stopListening();
      bool ok = radio.write(got_time, sizeof(got_time) );  
      radio.startListening(); 
      
      if(ok){
        Serial.println("SENT Trip Reply!");        
      }          
    }  
   
    //Display reply
    if(got_time[0]==1 || got_time[0]==0){
      Serial.println("DATA: Clock Reply!"); 
      waitForResponse = false;
      if( got_time[0]==1&&!isTimeRunning){
        Timer1.setPeriod(300);
        //detector.intervalLen=300;
        canDetectTrip = true;
      }        
    }  
  }
}



void checkBattery(){ 
    int reading = analogRead(BatStatusPin);  
    if(reading<400){
       digitalWrite(BatLedPin, LOW);  
    }
    else{
      digitalWrite(BatLedPin, HIGH);  
    }
        
    /*Serial.print("BAT cap: ");
    Serial.println(reading);    */
}

void loop() { 
  if(!canDetectTrip){
     readDataOnWireless();
  }     
  
  calculateCurrentTime(); 
  
  long timeSinceRemote = millis()-remoteBlinkStart;
  
  if(timeSinceRemote<1000){
    if(timeSinceRemote<250){
      digitalWrite(BatLedPin, LOW);
    }
    else if(timeSinceRemote<500){
      digitalWrite(BatLedPin, HIGH);
    }
    else if(timeSinceRemote<750){
      digitalWrite(BatLedPin, LOW);
    }
    else{
      digitalWrite(BatLedPin, HIGH);
    }  
  }
  
  
  if(!canDetectTrip){
     //so the romote does not have to send the trip end signal
     IsTripped[1]=false;   
  
    int retryTime=50;
    if(isTimeRunning)
      retryTime=200;
  
    //if timeout while sending to remote display
    if(millis()-lastRemoteSend>retryTime && waitForResponse){
      Serial.println("SEND TIME AGAIN!");
      doSendTime=true;     
    }
  } 
  
  if(doSendTime){ 
      sendTimeToRemoteDisplay();
      sendCount++;    
  }  
}


int interruptCount=0;

//called from the Timer1 interrupt
void detectTrip(){
   interruptCount++;   
   
   detector.DetectTrip(); 
   if(detector.tripEnded){
     IsTripped[0]=detector.isTripped;
     TripStartTime[0]=detector.TripStartTime;  
     TripSpeed[0]=0;
   }
     
  if(interruptCount>4000){
    interruptCount=0;
    checkBattery();  
  }
  
}


