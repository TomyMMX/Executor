#ifndef TripDetector_h
#define TripDetector_h

#include "Arduino.h"

class TripDetector 
{
	public:
		TripDetector(char* name, int laserOutPin, int laserInPin, int calibrationLeds);
		void DetectTrip();
		void StopDetect();
		void TurnLaserOff();
		boolean isTripped = false;
		boolean tripEnded = true;
                		
		unsigned long TripStartTime = 0;		
		unsigned long TripStartMicros = 0;        

	private:
		void blinkLasers();		
		int LaserOutPins = 2;
		int LaserInPins = A1;
	
		char* Name = "LASER";
		//const int Laser2OutPins = 9;
		//const int Laser2InPins = A2;

		int CalibrationLeds = 3;	
		boolean waitForTripEnd=false;
		int CalibrationLedState = HIGH;
		
        //some designs of the trip wire have the polarities reversed
		int CalibrationLedHIGH = HIGH;
		int CalibrationLedLOW = LOW;

		int LaserState = LOW;
		
		//sensor sensitivity (the lower the number, the more sensitive)
		const int diff = 50;		

        int failCount=0;
		int okCount=0;
		int signalBackCount=0;
						
		unsigned long LedOnStartTime = 0;			
		
		int prevLaserValue = 500;			
};

#endif
// END OF FILE
