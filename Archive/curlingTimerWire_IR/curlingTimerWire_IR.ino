#include <TimerOne.h>
#include <TripDetector_IR.h>
#include <RunningAverage.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

TripDetector_IR detector;
RF24 radio(6,10);

const int BatLedPin = 5;
const int BatStatusPin = A3;

//Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xE8E8F0F0E1LL, 0xF0F0F0F0D2LL };

void setup() { 
  Timer1.initialize(400);         // initialize timer1, and set 100 us period
  Timer1.attachInterrupt(detectTrip);  // attaches  detectTrip() as a timer overflow interrupt
  detector.intervalLen = 400;
  
  Serial.begin(9600);
  pinMode(BatLedPin, OUTPUT);
  digitalWrite(BatLedPin, LOW);

  // set up the ADC
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library

  // you can choose a prescaler from above.
  // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_16;    // set our own prescaler to 16  
}

void setLaserAccuracy(long interval){
  Timer1.setPeriod(interval);
  detector.intervalLen = interval;  
}

void loop() {   
 
}

int interruptCount;

//called from the Timer1 interrupt
void detectTrip(){ 
  detector.DetectTrip(); 
}


