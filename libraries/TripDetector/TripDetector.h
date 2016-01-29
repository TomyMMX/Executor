#ifndef TripDetector_h
#define TripDetector_h

#include "Arduino.h"
#include <RunningAverage.h>

class TripDetector 
{
	public:
		TripDetector();
		void DetectTrip();
		boolean isTripped = false;
		boolean tripEnded = true;
                int intervalLen = 100;
		unsigned long TripStartTime = 0;
                unsigned long StoneSpeed = 0;

	private:
		void blinkLasers();		
		const int LaserOutPins = 2;
		const int LaserInPins = A1;

		const int Laser2OutPins = 9;
		const int Laser2InPins = A2;

		const int CalibrationLeds = 3;	
		boolean waitForTripEnd=false;
		int CalibrationLedState = HIGH;
		
                //some designs of the trip wire have the polarities reversed
		int CalibrationLedHIGH = HIGH;
		int CalibrationLedLOW = LOW;

		int LaserState = LOW;
		
		//sensor sensitivity (the lower the number, the more sensitive)
		const int diff = 1;		

                int failCount=0;
		int okCount=0;
		int signalBackCount=0;
		unsigned long TripStartMicros = 0;
		unsigned long TripEndMicros = 0;
		unsigned long LastIntervalStartTime = 0;		
		unsigned long LedOnStartTime = 0;		
		unsigned long ReadTimeTime = 0;
		unsigned long LastDisplay=0;

		int prevLaserValue = 500;
		RunningAverage ReadAverage = RunningAverage(50);
		unsigned long startCalibration = 0;		
};

#endif
// END OF FILE
