#include "TripDetector.h"
#include "Arduino.h"

TripDetector::TripDetector(char* name, int laserOutPin, int laserInPin, int calibrationLeds){	
	LaserOutPins = laserOutPin;
	LaserInPins = laserInPin;
	CalibrationLeds = calibrationLeds;
	Name = name;

	pinMode(CalibrationLeds, OUTPUT);
	pinMode(LaserOutPins, OUTPUT);

	digitalWrite(CalibrationLeds, HIGH);
}

void TripDetector::TurnLaserOff(){ 
	LaserState = HIGH;
	digitalWrite(LaserOutPins, LaserState);  	
}

void TripDetector::StopDetect(){ 
	TurnLaserOff(); 

	digitalWrite(CalibrationLeds, LOW);
	isTripped = false;
	tripEnded = true;
	failCount=0;
	okCount=0;	
}

//int count=0;
void TripDetector::DetectTrip(){   	
	int reading = analogRead(LaserInPins);
	blinkLasers();  

    //Serial.println(reading);
   
	boolean change = false;
       
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
		signalBackCount = 0;		  
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
	    if(signalBackCount>5){
		//Serial.print(Name);
		//Serial.print(" - ");
		//Serial.println("TRIPP END detected");
        	tripEnded=true;       
	    }
	}	
     
	//trip when we didn't see the signal
	if(failCount > 1){     
		digitalWrite(CalibrationLeds, CalibrationLedHIGH);
		if(!isTripped){   
			//Serial.print(Name);
			//Serial.print(" - ");       
            //Serial.println("TRIPPED");	    
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
			//Serial.print(Name);
			//Serial.print(" - ");           
            //Serial.println("UN-TRIPPED");
		}    
		isTripped=false;                       
	} 
	prevLaserValue=reading;  
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
