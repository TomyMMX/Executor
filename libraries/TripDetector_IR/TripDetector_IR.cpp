#include "TripDetector_IR.h"
#include "Arduino.h"
#include <RunningAverage.h>

TripDetector_IR::TripDetector_IR(){
	pinMode(CalibrationLeds, OUTPUT);
	pinMode(LaserOutPins, OUTPUT);
	digitalWrite(CalibrationLeds, HIGH);	
}

void TripDetector_IR::DetectTrip(){ 
    ReadTimeTime = micros();   
    int reading = analogRead(LaserInPins);
    
    blinkLasers();  

    boolean change = false;
    int diff=10;

    if(LaserState == HIGH){      
      if(prevLaserValue < reading-diff)
	change=true;	
    }else if(LaserState == LOW){     
      if(prevLaserValue > reading+diff)
	change=true;	
    }   
       
    if(!change){
      //No change	
      failCount++;	
      signalBackCount=0;      
    }else if(!isTripped){
      TripStartTime = millis();
      TripStartMicros = micros();
      if(failCount!=0){	
        okCount++; 
      }
      if(okCount>3){
        failCount=0;
        okCount=0;  
      }	
    }

    if(change && !tripEnded){
      signalBackCount++;
      if(signalBackCount==1){            	
        TripEndMicros = micros();
      }
      if(signalBackCount>5){
        Serial.println("TRIPP END detected");
        tripEnded=true;             
        unsigned long tt = TripEndMicros-TripStartMicros;
	StoneSpeed = 28900000/tt;
        Serial.println(StoneSpeed);
      }
    }	
     
        //trip when we didn't see the signal
        if(failCount > (20000/intervalLen) ){     
          digitalWrite(CalibrationLeds, HIGH);
	  
          if(!isTripped){          
            Serial.println("TRIPPED");	    
	    tripEnded=false;             
          }
          isTripped=true;
          LedOnStartTime=millis(); 
	  failCount=0;
	  okCount=0;
        }
	//wire stays tripped for 500ms
        else if(millis()-LedOnStartTime>=200){  
	  digitalWrite(CalibrationLeds, LOW);     
          if(isTripped){                 
            Serial.println("UN-TRIPPED");
          }    
          isTripped=false;                       
        } 
	
	//is the light to bright??
	/*if(millis()-TripStartTime>2000 && millis()-startCalibration>3000 
           && isTripped){
	  Serial.println("CALLIBRATION");	  
	  startCalibration=millis();
	  LedOnStartTime=millis();
	}*/
            
      prevLaserValue=reading;      
}

void TripDetector_IR::blinkLasers()
{  
  if(LaserState==LOW){
    LaserState = HIGH;
  }
  else{
    LaserState = LOW;
  }
  
  digitalWrite(LaserOutPins, LaserState);  
}
// END OF FILE
