#include <TinyWireM.h>
#include <LiquidCrystal_I2C.h>

#include <HC_BouncyButton.h>


LiquidCrystal_I2C lcd(0x27, 20, 4);


// LASER_LOCK_INDICATOR_PIN is PB1 = digital 1
#define LASER_LOCK_INDICATOR_PIN 1

// LAP_RESET_PIN is PB3 = digital 3
#define LAP_RESET_PIN 3

// LASER_EYE_PIN is PB4 = analog 2
#define LASER_EYE_PIN 2


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
  // set up the LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.home();
/*
  lcd.print("Hello, world!");
  
  //         1234567890123456
  lcd.setCursor(16,0);
  lcd.print("|");
  lcd.setCursor(16,1);
  lcd.print("|");
  lcd.setCursor(0,2);
  lcd.print("----------------+");
*/

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
  
/*
  // detect if we have laser lock or not
  if ((millis()-pinTimers[LASER_LOCK_INDICATOR_PIN])>LASER_LOCK_FLASH_INTERVAL) {
    pinTimers[LASER_LOCK_INDICATOR_PIN]=millis();
    digitalWrite(LASER_LOCK_INDICATOR_PIN, !digitalRead(LASER_LOCK_INDICATOR_PIN));
  }  
*/

  eyeBrightness = analogRead(LASER_EYE_PIN);

  if (!haveLaserLock()) {
    // we DO NOT have laser lock, something is obstructing
    digitalWrite(LASER_LOCK_INDICATOR_PIN, LOW);

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
    // we have laser lock, illuminate the indicator
    digitalWrite(LASER_LOCK_INDICATOR_PIN, HIGH);
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
  
  
/*  
  if ((millis()-modeDisplayStringRunningCtrl)>500) {
    modeDisplayStringRunningCtrl=millis();
    if (modeDisplayStrings[MODE_RUNNING]=='X') {
      modeDisplayStrings[MODE_RUNNING]='+';
    } else {
      modeDisplayStrings[MODE_RUNNING]='X';
    }
  }
*/  
  
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
  //lcd.clear();

  lcd.setCursor(0,0);
  //         1234567890123456
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
  lcd.setCursor(0,0);
  lcd.print("AVG ");
  showTime(4,0,avgMillis);
}

void displayRecent() {
  unsigned long recentTime=0;
  if (lapCount>0) {
    recentTime=lapTimes[0];
  }
  lcd.setCursor(0,1);
  lcd.print("LAP ");
  showTime(4,1,recentTime);
}

void displayCurrent() {
  unsigned long runningTime=0;
  if (lapStartTime>0) {
    runningTime=millis()-lapStartTime;
  }
  lcd.setCursor(0,2);
  lcd.print("RUN ");
  showTime(4,2,runningTime);
}


void displayChrome() {
  
  lcd.setCursor(0,3);
  //             01234567890123456789
  //             LAPS:xx LASER:xxxx x 
  char buffer[]="LAPS:xx LASER:xxxx x";

  buffer[5]='0'+(lapCount/10)%10;
  buffer[6]='0'+lapCount%10;

  buffer[14]='0'+(eyeBrightness/1000)%10;
  buffer[15]='0'+(eyeBrightness/100)%10;

  buffer[16]='0'+(eyeBrightness/10)%10;
  buffer[17]='0'+eyeBrightness%10;
  
  buffer[19]=modeDisplayStrings[sysMode];


//  sprintf(buffer, "%4d %2d%c", eyeBrightness,lapCount,modeDisplayStrings[sysMode]);
  lcd.print(buffer);
  
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
  //               01234567
  char buffer[] = "mm:ss.HH";
  int minutes=(int)(time/60000);
  int seconds=(int)((time-minutes*60000)/1000);
  int hundredths=(int)((time%1000)/10);
/*
  if (minutes>9) {
    // we don't support times 10 minutes or longer
    minutes=minutes%10;
    // this breaks if it is 100 minutes, but that means i walked away and need a reset anyways
  }
*/
  buffer[0] = '0' + (minutes/10)%10;  
  buffer[1] = '0' + minutes%10;
  
  buffer[3]='0' + (seconds/10)%10;
  buffer[4]='0' + seconds%10;
  
  buffer[6]='0' + (hundredths/10)%10;
  buffer[7]='0' + hundredths%10;
  
  
//  sprintf(buffer, "%01d:%02d.%02d", minutes, seconds, hundredths);

  lcd.setCursor(col,row);  
  lcd.print(buffer);
  // m:ss.XX
}

