
// Controlling various RGB LEDs for a PC
// Including PWM RGB LED strips and NeoPixel serial RGB LEDs
// Nic Chester 2017

#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//A command packet consists of 8 bytes:
//[CMD_START] [CMD] [STRIP_ID] [LED_ID] [RED] [GREEN] [BLUE] [TIME]
// Positions:
#define CMD_START   0
#define CMD         1
#define STRIP_ID    2
#define LED_ID      3
#define RED         4
#define GREEN       5
#define BLUE        6
#define TIME        7

// Start of "valid command" marker
#define START    0xA
//Commands:
#define SET      0xB        // Set RGB value immediately. 5 arguments: Strip ID, LED ID, RGB.
#define FADE     0xC        // Fade to value. 4 arguments: Strip ID, RGB. 
#define SETMODE  0xD        // Set mode
#define SETDEF   0xE        // set default values 

// Strip 1 pins
#define ST1_BLUE   10
#define ST1_GREEN  9
#define ST1_RED    11

// Strip 2 pins
#define ST2_BLUE   6
#define ST2_GREEN  5
#define ST2_RED    3

// pin to which fan 1 LEDs are connected
#define FAN1 8

// pin to which fan 2 LEDs are connected
#define FAN2 7

// Fade speeds
int slowFade = 1000; 
int medFade  = 500;
int fastFade = 200;

// Modes
#define CONTINUOUS 0
#define CYCLE      1

int mode = 0; // default mode is 0/Continuous colour

// EEPROM Memory locations

#define R1 0x0
#define G1 0x1
#define B1 0x2

#define R2 0x3
#define G2 0x4
#define B2 0x5

#define R3 0x6
#define G3 0x7
#define B3 0x8

class Fade
{
public:
  int interval = 30;
  int currentInterval; 
  int timeStep;
  int deltaRed, deltaGreen, deltaBlue;
  boolean fading;
  unsigned long lastTick;
  
  boolean redUp, greenUp, blueUp;
  int redStep, greenStep, blueStep;
  
  byte tmpRed, tmpGreen, tmpBlue;
  byte red, green, blue; 
  
};

class RGB
{
  int redPin, greenPin, bluePin;
  int curRed   = 0;
  int curGreen = 0;
  int curBlue  = 0;
  
  Fade fade; 
  
public:
  RGB(int r, int g, int b)
  {
    redPin   = r;
    greenPin = g;
    bluePin  = b;
  }
  void init()
  {  
    pinMode(redPin,   OUTPUT); 
    pinMode(greenPin, OUTPUT); 
    pinMode(bluePin,  OUTPUT);
  }
  
  void set(byte red, byte green, byte blue)
  {
    curRed   = red;
    curGreen = green; 
    curBlue  = blue; 
  }
  
  void fadeTo(byte red, byte green, byte blue, int ms)
  {  
    fade.timeStep = ms / fade.interval;
  
    fade.deltaRed   = abs(curRed   - red);
    fade.deltaGreen = abs(curGreen - green);
    fade.deltaBlue  = abs(curBlue  - blue);
    
    fade.red   = red;
    fade.green = green; 
    fade.blue  = blue; 
  
    fade.fading = true;
  
    fade.redUp   = (red   > curRed)   ? 1 : 0; 
    fade.greenUp = (green > curGreen) ? 1 : 0; 
    fade.blueUp  = (blue  > curBlue)  ? 1 : 0; 
    
    fade.redStep   = fade.deltaRed   / fade.interval;
    fade.greenStep = fade.deltaGreen / fade.interval;
    fade.blueStep  = fade.deltaBlue  / fade.interval;

    fade.currentInterval = 0; 
    
    fade.tmpRed   = curRed; 
    fade.tmpGreen = curGreen;
    fade.tmpBlue  = curBlue; 
   }
    
  void fadeTick()
  {    
    if(fade.currentInterval >= fade.interval) 
    {
      fade.fading = false;
      this -> set(fade.red, fade.green, fade.blue); 
      return;
    }
    
    unsigned long currentMillis = millis();
    if(currentMillis - fade.lastTick > fade.timeStep)
    {
      if(fade.redUp)   fade.tmpRed   += fade.redStep;   else fade.tmpRed   -= fade.redStep;
      if(fade.greenUp) fade.tmpGreen += fade.greenStep; else fade.tmpGreen -= fade.greenStep; 
      if(fade.blueUp)  fade.tmpBlue  += fade.blueStep;  else fade.tmpBlue  -= fade.blueStep;
      
      this -> set(fade.tmpRed, fade.tmpGreen, fade.tmpBlue);
      
      fade.lastTick = currentMillis;
      fade.currentInterval++; 
    }
  }
  
  void tick()
  {
    if(fade.fading) fadeTick();
    
    analogWrite(redPin,   curRed); 
    analogWrite(greenPin, curGreen);
    analogWrite(bluePin,  curBlue);
  }
  
};

class Fan
{
  Adafruit_NeoPixel* thisFan;
   
  byte curRed   = 0;
  byte curGreen = 0; 
  byte curBlue  = 0; 
  
  Fade fade; 
  
public:
  Fan(Adafruit_NeoPixel* fan)
  {
    thisFan = fan; 
  }
  void init()
  {
    thisFan -> begin(); 
    thisFan -> show(); 

     
  }
  void set(byte r, byte g, byte b)
  {
    curRed   = r; 
    curGreen = g; 
    curBlue  = b;
  }
  
  void fadeTo(byte red, byte green, byte blue, int ms)
  {  
    fade.timeStep = ms / fade.interval;
  
    fade.deltaRed   = abs(curRed   - red);
    fade.deltaGreen = abs(curGreen - green);
    fade.deltaBlue  = abs(curBlue  - blue);
    
    fade.red   = red;
    fade.green = green; 
    fade.blue  = blue; 
  
    fade.fading = true;
  
    fade.redUp   = (red   > curRed)   ? 1 : 0; 
    fade.greenUp = (green > curGreen) ? 1 : 0; 
    fade.blueUp  = (blue  > curBlue)  ? 1 : 0; 
    
    fade.redStep   = fade.deltaRed   / fade.interval;
    fade.greenStep = fade.deltaGreen / fade.interval;
    fade.blueStep  = fade.deltaBlue  / fade.interval;

    fade.currentInterval = 0; 
    
    fade.tmpRed   = curRed; 
    fade.tmpGreen = curGreen;
    fade.tmpBlue  = curBlue; 
   }
   
  void fadeTick()
  {
    if(fade.currentInterval >= fade.interval) 
    {
      fade.fading = false;
      this -> set(fade.red, fade.green, fade.blue); 
      return;
    }
    
    unsigned long currentMillis = millis();
    if(currentMillis - fade.lastTick > fade.timeStep)
    {
      if(fade.redUp)   fade.tmpRed   += fade.redStep;   else fade.tmpRed   -= fade.redStep;
      if(fade.greenUp) fade.tmpGreen += fade.greenStep; else fade.tmpGreen -= fade.greenStep; 
      if(fade.blueUp)  fade.tmpBlue  += fade.blueStep;  else fade.tmpBlue  -= fade.blueStep;
      
      this -> set(fade.tmpRed, fade.tmpGreen, fade.tmpBlue);
      
      fade.lastTick = currentMillis;
      fade.currentInterval++; 
    }
    
  }
  void tick()
  {
    if(fade.fading) fadeTick();
    
    for(int i = 0; i < 5; i++)
    {
      // Fucking GRB instead of RGB:
      thisFan -> setPixelColor(i, curGreen, curRed, curBlue);
    }
    thisFan -> show();
  }
};

// Our PWM RGB LEDs
RGB rgb(ST1_RED, ST1_GREEN, ST1_BLUE);

// NeoPixel objects
Adafruit_NeoPixel fan1_pixel = Adafruit_NeoPixel(4, FAN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel fan2_pixel = Adafruit_NeoPixel(4, FAN2, NEO_GRB + NEO_KHZ800);

// RGB Fan objects
Fan fan1(&fan1_pixel);
Fan fan2(&fan2_pixel);

void setup()
{
  rgb.init(); 
  fan1.init();
  fan2.init();
  
  Serial.begin(9600);
  delay(50);
  
  setStrip(1, EEPROM.read(R1), EEPROM.read(G1), EEPROM.read(B1)); 
  setStrip(2, EEPROM.read(R2), EEPROM.read(G2), EEPROM.read(B2)); 
  setStrip(3, EEPROM.read(R3), EEPROM.read(G3), EEPROM.read(B3)); 
  
  rgb.tick();
  fan1.tick(); 
  fan2.tick(); 
}

void setStrip(byte strip, byte r, byte g, byte b)
{
  if(strip == 1)
  {
    rgb.set(r, g, b);
  }
  else if(strip == 2)
  {
    fan1.set(r, g, b);
  }
  else if(strip == 3)
  {
    fan2.set(r, g, b);
  }
  else
  {
    rgb.set(r, g, b);
    fan1.set(r, g, b);
    fan2.set(r, g, b);
  }
}

void fadeStrip(byte strip, byte r, byte g, byte b, int time)
{
  if(strip == 1)
  {
    rgb.fadeTo(r, g, b, time);
  }
  else if(strip == 2)
  {
    fan1.fadeTo(r, g, b, time);
  }
  else if(strip == 3)
  {
    fan2.fadeTo(r, g, b, time);
  }
  else
  {
    rgb.fadeTo(r, g, b, time);
    fan1.fadeTo(r, g, b, time);
    fan2.fadeTo(r, g, b, time);
  }
}

void checkWrite(byte address, byte value)
{
  if(value != EEPROM.read(address))
  {
    EEPROM.write(address, value);
  }
}

void eepromSave(byte strip, byte r, byte g, byte b) 
{
  if (strip == 1)
  {
    checkWrite(R1, r);
    checkWrite(G1, g);
    checkWrite(B1, b); 
  }
  else if(strip == 2)
  {
    checkWrite(R2, r);
    checkWrite(G2, g);
    checkWrite(B2, b);
  }
  else if(strip == 3)
  {
    checkWrite(R3, r); 
    checkWrite(G3, g); 
    checkWrite(B3, b); 
  }
  else
  {
    checkWrite(R1, r);
    checkWrite(G1, g);
    checkWrite(B1, b); 
    
    checkWrite(R2, r);
    checkWrite(G2, g);
    checkWrite(B2, b);
    
    checkWrite(R3, r); 
    checkWrite(G3, g); 
    checkWrite(B3, b);
  }
}

byte command[8];
boolean newCommand  = false; 
unsigned long lastCommandByte; 
int commandPos = 0;
int timeout = 1000;
int cmdFadeTime; 

void loop()
{
  if(Serial.available() > 0)
  {
    lastCommandByte = millis();
    command[commandPos] = Serial.read();
    commandPos++;
    
    if(commandPos > 7)
    {
      commandPos = 0;
      newCommand = true;
    }
  }
  if(millis() - lastCommandByte > timeout)
  {
    commandPos = 0;
  }
  
  if(newCommand)
  {
    if(command[CMD_START] == START)
    {
      switch(command[TIME])
      {
        case 'f' :
          cmdFadeTime = fastFade;
          break;
        case 'm' :
          cmdFadeTime = medFade;
          break;
        case 's' :
          cmdFadeTime = slowFade;
          break;
        default:
          cmdFadeTime = medFade;
          break;
      }
      switch(command[CMD])
      {
        case SET:
          setStrip(command[STRIP_ID], command[RED], command[GREEN], command[BLUE]);
          eepromSave(command[STRIP_ID], command[RED], command[GREEN], command[BLUE]);
          break;
        case FADE:
          fadeStrip(command[STRIP_ID], command[RED], command[GREEN], command[BLUE], cmdFadeTime);
          eepromSave(command[STRIP_ID], command[RED], command[GREEN], command[BLUE]);
          break; 
      }
    }
        
    newCommand = false; 
  }
  
  
  rgb.tick();
  fan1.tick(); 
  fan2.tick();
  
  Serial.flush();
}
