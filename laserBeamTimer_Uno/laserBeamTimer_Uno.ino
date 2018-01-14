/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

// include the library code:
#include <LiquidCrystal.h>
#include <HC_BouncyButton.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define LASER_LOCK_INDICATOR_PIN 7
#define LAP_RESET_PIN 6
#define LASER_EYE_PIN A0

#define LASER_LOCK_THRESHOLD 500

#define LASER_LOCK_FLASH_INTERVAL 500

#define MAX_LAP_HISTORY 10

unsigned long pinTimers[17];
unsigned long resetMillis=0;

BouncyButton lapResetButton=BouncyButton(LAP_RESET_PIN);

int eyeBrightness=0;

int lapCount=0;
unsigned long lapTimes[MAX_LAP_HISTORY];
unsigned long lapStartTime=0;

#define MODE_IDLE 0
#define MODE_ARMED 1
#define MODE_RUNNING 2

#define MINIMUM_LAP_DURATION 2000

int sysMode = MODE_IDLE;
char modeDisplayStrings[] = {'I', 'A','X'};

unsigned long modeDisplayStringRunningCtrl=0;


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  //lcd.print("hello, world!");
  
  pinMode(LAP_RESET_PIN, INPUT_PULLUP);
  pinMode(LASER_LOCK_INDICATOR_PIN, OUTPUT);
  
  lapResetButton.init();
 
 
  updateDisplay(); 
}

void loop() {
  
  if (lapResetButton.update() && !lapResetButton.getState()) {
    // button was just pressed down
    resetMillis=millis();
    
    // if we are IDLE, then we can become ARMED if we have laser lock
    // otherwise, we become IDLE and clear out the history
    
    if (sysMode==MODE_IDLE) {
      if (haveLaserLock()) {
        sysMode=MODE_ARMED;

        lapStartTime=0;

        lapCount=0;
        for (int i=0; i<MAX_LAP_HISTORY; i++) {
          lapTimes[i]=0;
        }
      }
      
    } else {
      
      sysMode=MODE_IDLE;
       
      // don't clear out the timers, but DO zero out the current lap running timer
      lapStartTime=0;
    }
    updateDisplay();    
  }
  
  // detect if we have laser lock or not
/*  if ((millis()-pinTimers[LASER_LOCK_INDICATOR_PIN])>LASER_LOCK_FLASH_INTERVAL) {
    pinTimers[LASER_LOCK_INDICATOR_PIN]=millis();
    digitalWrite(LASER_LOCK_INDICATOR_PIN, !digitalRead(LASER_LOCK_INDICATOR_PIN));
  }  
*/

  eyeBrightness = analogRead(LASER_EYE_PIN);

  if (!haveLaserLock()) {
    // we DO NOT have laser lock, something is obstructing
    digitalWrite(LASER_LOCK_INDICATOR_PIN, HIGH);

    // need to allow enough time for a robot to actually pass through the laser before we consider this a new beam break event
    // essentially a meta-debouncer for the whole robot
    if ((millis()-lapStartTime)>MINIMUM_LAP_DURATION) {
      
      // TODO implement a checker to see if lock is broken for too long, go IDLE

      if (sysMode==MODE_ARMED) {
        // begin a new lap
        sysMode=MODE_RUNNING;
        beginLap();
      } else if (sysMode==MODE_RUNNING) {

        // end the current lap and start a new one
        endLap();
        beginLap();
      }
    }    
    
    
  } else {
    digitalWrite(LASER_LOCK_INDICATOR_PIN, LOW);
  }

  if ((millis()-pinTimers[LASER_EYE_PIN])>500) {
    // periodically display the value being read
    pinTimers[LASER_EYE_PIN]=millis();
    displayChrome();
  }

  
  if (sysMode==MODE_RUNNING) {
    // we are in a lap, update the current time display
    displayCurrent();
  }
  
  
  
  if ((millis()-modeDisplayStringRunningCtrl)>500) {
    modeDisplayStringRunningCtrl=millis();
    if (modeDisplayStrings[MODE_RUNNING]=='X') {
      modeDisplayStrings[MODE_RUNNING]='+';
    } else {
      modeDisplayStrings[MODE_RUNNING]='X';
    }
  }
  
  
}

boolean haveLaserLock() {
  return eyeBrightness>=LASER_LOCK_THRESHOLD;
}

void beginLap() {
  // record the start time
  lapStartTime=millis();
  
  // update the chrome to show the lap is in progress
  displayChrome();
}

void endLap() {
  // record the lap time
  unsigned long lapTime = millis()-lapStartTime;
  
  // update the "lap in progress" flag as complete
  lapStartTime=0;
  
  // bump out the history and save the current lap
  for (int i=lapCount; i>0; i--) {
    if (lapCount<MAX_LAP_HISTORY) {
      // oldest one drops off the radar
      lapTimes[i]=lapTimes[i-1];
    }
  }
  lapTimes[0]=lapTime;
  if (lapCount<MAX_LAP_HISTORY) {
    lapCount++;
  }
  
  
  // need to update the entire display now
  updateDisplay();
}

void displayClear() {
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}

void displayAverage() {
  unsigned long totalMillis = 0;
  for (int i=0;i<lapCount; i++) {
    totalMillis+=lapTimes[i];
  }
  unsigned long avgMillis = 0;
  if (lapCount>0) {
    avgMillis = (unsigned long)(totalMillis/lapCount);
  }
  showTime(0,0,avgMillis);
}

void displayRecent() {
  unsigned long recentTime=0;
  if (lapCount>0) {
    recentTime=lapTimes[0];
  }
  showTime(0,1,recentTime);
}

void displayCurrent() {
  unsigned long runningTime=0;
  if (lapStartTime>0) {
    runningTime=millis()-lapStartTime;
  }
  showTime(9,1,runningTime);
}
void displayChrome() {
  
  lcd.setCursor(8,0);
  char buffer[8]="";
  sprintf(buffer, "%4d %2d%c", eyeBrightness,lapCount,modeDisplayStrings[sysMode]);
  lcd.print(buffer);

  
  // show inprogress flag on upper right
//  lcd.setCursor(15,0);
//  lcd.print(lapStartTime==0?0:1);

}

void updateDisplay() {
  displayClear();
    
  // show average of all history on upper left
  displayAverage();
  
  // show inprogress flag on upper right
  displayChrome();
  
  // show most recent split time in lower left
  displayRecent();
  
  // show running time of current lap in lower right
  displayCurrent();
}

void showTime(int col, int row, unsigned long time) {

  char buffer[7]="";
  int minutes=(int)(time/60000);
  int seconds=(int)((time-minutes*60000)/1000);
  int hundredths=(int)((time%1000)/10);
  if (minutes>9) {
    // we don't support times 10 minutes or longer
    minutes=minutes%10;
    // this breaks if it is 100 minutes, but that means i walked away and need a reset anyways
  }
  
  sprintf(buffer, "%01d:%02d:%02d", minutes, seconds, hundredths);

  lcd.setCursor(col,row);  
  lcd.print(buffer);
  // m:ss:XX
}

