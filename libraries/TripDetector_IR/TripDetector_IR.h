#ifndef TripDetector_IR_h
#define TripDetector_IR_h

#include "Arduino.h"
#include <RunningAverage.h>

class TripDetector_IR 
{
	public:
		TripDetector_IR();
		void DetectTrip();
		boolean isTripped = false;
		boolean tripEnded = true;
                int intervalLen = 100;
		unsigned long TripStartTime = 0;
                unsigned long StoneSpeed = 0;

	private:
		void blinkLasers();		
		const int LaserOutPins = 9;
		const int LaserInPins = A1;
		const int CalibrationLeds = 3;	
		boolean waitForTripEnd=false;
		int CalibrationLedState = HIGH;
		int LaserState = LOW;

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
		RunningAverage ReadAverage = RunningAverage(10);
		unsigned long startCalibration = 0;		
};

#endif
// END OF FILE
