#include <RunningAverage.h>


int calibrationLeds[] = {2,3}

int numReadings = 50;
RunningAverage myRA(20);

void getLaserAverage(){  
  Serial.print("Callibrating...");
  
  myRA.clear();
  for(int i=0;i<numReadings;i++)
  {
    int sensorValue = analogRead(A0);
    myRA.addValue(sensorValue * 0.001);
    delay(50);
    digitalWrite(TRIPLED, LOW);
    delay(50);
    digitalWrite(TRIPLED, HIGH);    
  }  
  
  Serial.print("Average is: ");
  Serial.println(myRA.getAverage(), 3);
}

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(TRIPLED, OUTPUT);
  getLaserAverage();
}

boolean tripped=false;
int count = 0;
// the loop routine runs over and over again forever:
void loop() {
  delay(1000);
  
  Serial.println(A0);
  int reading= analogRead(A0);
  float value = (float)reading* 0.001; 
 Serial.println(value); 
  if((value>myRA.getAverage()*1.1||value<myRA.getAverage()*0.85) && !tripped){
    digitalWrite(TRIPLED, HIGH);
    if(count>3)
    {
      Serial.print("Laser tripped!! ");
      Serial.println(value);
      
      tripped=true;
    }  
    count++;  
  }  
  else if(value<myRA.getAverage()*1.1&&value>myRA.getAverage()*0.85)
  {
    digitalWrite(TRIPLED, LOW);
    tripped=false;
    count = 0;
  }
  else if(tripped)
  {
    count++;
  }
  
  
  if(count>1000)
  {
    myRA.clear();
    getLaserAverage();
  }
  
  
  delay(5);
}

