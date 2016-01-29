/*
  Code for interfacing with 7 segment displays
using the multiplexing method
and the TPIC6B595 Shift Register (1 per digit)
By K.O.
*/

//Pin Assignments (You should change these)
const int CLK       = 9;           //Connected to TPIC pin 13: SRCLK (aka Clock)
const int LATCH     = 7;          //Connected to TPIC pin 12: RCLK (aka Latch/load/CS/SS...)
const int OE        = 6;          //Connected to TPIC pin 9: OE (Output Enable)
const int DOUT      = 8;          //Connected to TPIC pin 3: SER (aka MOSI)

//Number Patterns (0-9)
//***Drains 0-7 must be connected to segments A-DP respectively***
const byte numTable[] =
{
  B01111110,
  B00011000,
  B10110110,
  B10111100,
  B11011000,
  B11101100,
  B11101110,
  B00111000,
  B11111110,
  B11111100,
};

//Global Variables
int numDevices = 1;                       //The number of x-digit display modules you plan to use
int maxDisplays = 3;                      //The maximum displays that could be accommodated (see note 1)
int maxDigits = 3;                        //The maximum digits you plan on displaying per display module (each SR can handle a max of 8 digits)
int SRData[3][3];                         //The storage location for the digit information. We must specify a fixed array at compile time (see note 2)
boolean debug = true;                     //Change to true to print messages
int delayTime = 1000;                     //Optional (just for demonstrating multiplexing)

/*
  Notes
1. It is recommended to use an external power supply to avoid oversource/sinking the microcontroller
    or if you need to power high voltage, high current displays. This code will turn on/off all segments in a digit for ***each*** display.
    So, if using 2x 3-digit displays all displaying an 8 + DP, the max consumption will be:
       20mA (desired forward current) * 8 (segments that are on) * 2 (displays showing identical info) = 320mA
2. The first dimension should equal maxDisplays. The second dimension should equal the number of digits
*/

void setup()
{
  Serial.begin(9600);

  //Set pin modes
  pinMode(CLK,OUTPUT);
  pinMode(LATCH,OUTPUT);
  pinMode(DOUT, OUTPUT);
  pinMode(OE, OUTPUT);

  //7-Segment Display Init
  digitalWrite(OE,LOW);        //Enables SR Operation
  initializeSRData();          //Prepares SR and clears data on serial line

  //Test
  setDigit(0,0,4,true);
  setDigit(0,1,5,true);
  setDigit(0,2,6,true);
}

void loop()
{
  refreshDisplay();            //Cycles through all displays and digits
}

//==========BEGIN SR Functions==========
void initializeSRData()
{
  //Display Scanner (Iterates through each display module)
  digitalWrite(LATCH,LOW);      //Tells all SRs that uController is sending data

  for(int dispID = 0; dispID < maxDisplays; dispID++)
  {   
    //Digit Scanner (Iterates through each SR (digit) in a display module)
    for(int digit = 0; digit < maxDigits; digit++)
    {
      //Clears any garbage on the serial line
      shiftOut(DOUT,CLK,LSBFIRST,0);          //Shift out 0s to all displays
      SRData[dispID][digit] = 0;              //Stores a 0 for each digit so its completely off
    }
  }
  digitalWrite(LATCH,HIGH);      //Tells all SRs that uController is done sending data
}

void printSRData()
{
  if(!debug)
    return;

  Serial.println("Printing SR Data...");

  //Display Scanner
  for(int dispID = 0; dispID < maxDisplays; dispID++)
  {   
    Serial.print("Display # ");
    Serial.println(dispID);

    //Digit Scanner
    for(int digit = 0; digit < maxDigits; digit++)
    {
      Serial.print("Digit ");
      Serial.print(digit);
      Serial.print(": ");
      Serial.println(SRData[dispID][digit],BIN);
    }
    Serial.println();
  }
}

void setDigit(int dispID, int digit, int value, boolean dp)
{
  //Parameter checker
  if(dispID < 0 || dispID >= numDevices)
  {
    Serial.println("dispID OoB!");         //OoB = Out of bounds
    return;
  }

  if(digit < 0 || digit > maxDigits)
  {
    Serial.println("digit OoB!");
    return;
  }

  if(value < 0 || value > 9)
  {
    Serial.println("Invalid value!");
    return;
  }

  value = numTable[value];

  //Toggle dp if needed
  if(dp)
    value |= B00000001;          //Turns on the first binary digit (segment) using an OR bitmask

  //Store the digit
  SRData[dispID][digit] = value;

  if(debug)
    printSRData();
}

void setSegments(int dispID, int digit, byte value)
{
  //Parameter checker
  if(dispID < 0 || dispID >= numDevices)
  {
    Serial.println("dispID OoB!");
    return;
  }

  if(digit < 0 || digit > maxDigits)
  {
    Serial.println("digit OoB!");
    return;
  }

  if(value < 0 || value > 255)
  {
    Serial.println("Invalid byte!");
    return;
  }

  //Store the digit
  SRData[dispID][digit] = value;

  if(debug)
    printSRData();
}

void clearDisplay(int dispID)
{
  initializeSRData();
  refreshDisplay();
}

void refreshDisplay()
{
  //Digit Scanner
  for(int digit = 0; digit < maxDigits; digit++)
  {  
    //Display Scanner
    digitalWrite(LATCH,LOW);
    for(int dispID = numDevices -  1; dispID >= 0; dispID--)
    {
      //Pre-Digit blanker (shifts out 0s to correct digits before sending segment data to desired digit)
      for(int blanks = (maxDigits - 1 - digit); blanks > 0; blanks--)
        shiftOut(DOUT,CLK,LSBFIRST,0);

      shiftOut(DOUT,CLK,LSBFIRST,SRData[dispID][digit]);

      //Post-Digit blanker (shifts out 0s to remaining digits)
      for(int blanks = digit; blanks > 0; blanks--)
        shiftOut(DOUT,CLK,LSBFIRST,0);
    }
    digitalWrite(LATCH,HIGH);

    //Demonstrates multiplexing operation
    delay(delayTime);
    delayTime -= 10;
    if(delayTime <= 0)
      delayTime = 0;
  }
}
