#include "TripDetector.h"
#include "Arduino.h"
#include <RunningAverage.h>

TripDetector::TripDetector(){
	pinMode(CalibrationLeds, OUTPUT);
	pinMode(LaserOutPins, OUTPUT);

	pinMode(Laser2OutPins, OUTPUT);
	digitalWrite(Laser2OutPins, LOW);

	digitalWrite(CalibrationLeds, HIGH);
}
//int count=0;
void TripDetector::DetectTrip(){ 
    //count++; 
    ReadTimeTime = micros();   
    int reading = analogRead(LaserInPins);
    blinkLasers();  

    //Serial.println(reading);
    float margin = 0;      
     
    /*if(!tripEnded){ 
	Serial.println(reading);
    }*/
        
    //Don't detect trips as long as initial callibration is running
    if(millis()-startCalibration<100){
	//Add value for running average 
    	ReadAverage.addValue(reading*1.0);
    }    
    else{ 
      margin = ReadAverage.getAverage(); 
      
      //Too bright to operate... just blink trip LED
      if(margin>700){
        if(millis()-LastIntervalStartTime>=50){
	  ReadAverage.addValue(reading*1.0);
	  LastIntervalStartTime=millis();	
	  if(CalibrationLedState==LOW){
	    CalibrationLedState=HIGH;            
          }else{
            CalibrationLedState=LOW;
	  }
	  digitalWrite(CalibrationLeds, CalibrationLedState);
        }
      }
      else{  
        boolean change = false;
       
	if(LaserState == HIGH){      
	  if(prevLaserValue < reading-diff){
	    change=true;		
	}
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
          digitalWrite(CalibrationLeds, CalibrationLedHIGH);
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
        else if(millis()-LedOnStartTime>=500){  
	  digitalWrite(CalibrationLeds, CalibrationLedLOW);     
          if(isTripped){                 
            Serial.println("UN-TRIPPED");
          }    
          isTripped=false;                       
        } 
	
	//is the light to bright??
	if(millis()-TripStartTime>2000 && millis()-startCalibration>3000 
	   && isTripped){
	  Serial.println("CALLIBRATION");	  
	  startCalibration=millis();
	  LedOnStartTime=millis();
	}
      }      
      prevLaserValue=reading;      
    }   
}

void TripDetector::blinkLasers()
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
