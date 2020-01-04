#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROMex.h>

#include <HC_BouncyButton.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);

byte bar_left[4][8] = {
  {0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111},
  {0b11111, 0b10000, 0b10100, 0b10100, 0b10100, 0b10000, 0b11111},
  {0b11111, 0b10000, 0b10110, 0b10110, 0b10110, 0b10000, 0b11111},
  {0b11111, 0b10000, 0b10111, 0b10111, 0b10111, 0b10000, 0b11111}
};

byte bar_mid[6][8] = {
  {0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111},
  {0b11111, 0b00000, 0b10000, 0b10000, 0b10000, 0b00000, 0b11111},
  {0b11111, 0b00000, 0b11000, 0b11000, 0b11000, 0b00000, 0b11111},
  {0b11111, 0b00000, 0b11100, 0b11100, 0b11100, 0b00000, 0b11111},
  {0b11111, 0b00000, 0b11110, 0b11110, 0b11110, 0b00000, 0b11111},
  {0b11111, 0b00000, 0b11111, 0b11111, 0b11111, 0b00000, 0b11111},
};

byte bar_right[4][8] = {
  {0b11111, 0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b11111},
  {0b11111, 0b00001, 0b10001, 0b10001, 0b10001, 0b00001, 0b11111},
  {0b11111, 0b00001, 0b11001, 0b11001, 0b11001, 0b00001, 0b11111},
  {0b11111, 0b00001, 0b11101, 0b11101, 0b11101, 0b00001, 0b11111},
};


const int BAR_COUNT = 10;
const int MID_COUNT = BAR_COUNT - 2;

const int CHAR_LEFT = 0;
const int CHAR_MID_FULL = 1;
const int CHAR_MID_PROG = 2;
const int CHAR_MID_BLANK = 3;
const int CHAR_RIGHT = 4;

byte barStates[BAR_COUNT] = {127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };

const int PIN_BAT_ADC=A0;

const int PIN_BTN_UP=4;
const int PIN_BTN_DN=3;
const int PIN_LED_SIP=5;
const int PIN_LED_OK=6;

BouncyButton btnUp = BouncyButton(PIN_BTN_UP);
BouncyButton btnDn = BouncyButton(PIN_BTN_DN);

/*
 * EEPROM:
 * addr 0 - byte - number of sips to target
 * addr 1 - byte - number of minutes for reminder timer
 * addr 2-3 - int - min battery ADC measured
 * addr 4-5 - int - max battery ADC measured
 */

const int EEPROM_SIP_TARGET=0;
const int EEPROM_REMINDER_MINUTES=1;
const int EEPROM_BATTERY_MIN=2;
const int EEPROM_BATTERY_MAX=4;

uint8_t sipTarget=0;
uint8_t sipCount=0;

unsigned long reminderMillis=0;

unsigned long lastSipMillis=0;


const int REMINDER_STATE_QUIET=0;
const int REMINDER_STATE_ALERT=1;

uint8_t reminderState=REMINDER_STATE_QUIET;

const unsigned long LCD_BACKLIGHT_DURATION = 10000;
unsigned long lcdBacklightTimer=0;

boolean downPressRegistered=false;

const unsigned long OK_LED_DURATION_MILLIS=2000;
unsigned long okLedShutoffTime=0;

unsigned long lastBatteryMillis;
const unsigned long BATTERY_CHECK_MILLIS=1000;
int batteryLevel=0;
int batteryMax=0;
int batteryMin=1024;

unsigned long lastReportMillis;
const int REPORT_DURATION_MILLIS=1000;
int reportCnt=0;


// headers for functions with default values
void backlightOff();
void backlightOn(boolean wantTimer=true);
void okOff();
void okOn(unsigned long duration=0);

void setup() {

  // i want millis() to at least be > 0 before anything runs
  delay(2);

  Serial.begin(9600);
  Serial.println("hc-sippy");

  // Battery ADC is essentially in the range from 550-660, so
  // would never expect more than 300 writes in a single execution
  // i.e. the battery dies and we reboot, now have a good set of min/max and
  // it'll pretty much never write again except for maybe once or twice.
  EEPROM.setMaxAllowedWrites(300);

  Serial.println(F("EEPROM:"));
  
  //EEPROM.write(EEPROM_SIP_TARGET, 48);
  sipTarget = EEPROM.read(EEPROM_SIP_TARGET);
  Serial.print(F("  sipTarget: "));
  Serial.println(sipTarget);

  //EEPROM.write(EEPROM_REMINDER_MINUTES, 10);
  reminderMillis = ((unsigned long)EEPROM.read(EEPROM_REMINDER_MINUTES)) * 60UL * 1000UL;
  Serial.print(F("  reminderMinutes: "));
  Serial.println(EEPROM.read(EEPROM_REMINDER_MINUTES));
  //reminderMillis=30UL*1000UL;

  //EEPROM.writeInt(EEPROM_BATTERY_MIN, 1024);
  //EEPROM.writeInt(EEPROM_BATTERY_MAX, 0);
  batteryMin = EEPROM.readInt(EEPROM_BATTERY_MIN);
  batteryMax = EEPROM.readInt(EEPROM_BATTERY_MAX);

  Serial.print(F("  batteryMin: "));
  Serial.println(batteryMin);
  Serial.print(F("  batteryMax: "));
  Serial.println(batteryMax);

  lcd.init();

  // turn the backlight on for 10 seconds on boot
  backlightOn();
  
  lcd.clear();
  lcd.home();

  // char0 is the left endcap (starts empty, idx=0)
  lcd.createChar(CHAR_LEFT, bar_left[0]);
  
  // char1 is always a full mid (idx=5)
  lcd.createChar(CHAR_MID_FULL, bar_mid[5]);
  
  // char2 is the currently progressing mid (starts empty, idx=0)
  lcd.createChar(CHAR_MID_PROG, bar_mid[0]);

  // char3 is always a blank mid
  lcd.createChar(CHAR_MID_BLANK, bar_mid[0]);

  // char4 is the right endcap (starts empty, idx=0)
  lcd.createChar(CHAR_RIGHT, bar_right[0]);
  

  btnUp.init();
  btnDn.init();

  pinMode(PIN_LED_SIP, OUTPUT);
  pinMode(PIN_LED_OK, OUTPUT);

  // wired so HIGH is ON
  digitalWrite(PIN_LED_SIP, LOW);
  digitalWrite(PIN_LED_OK, LOW);

  countChange();
}

void loop() {

  if (btnUp.update() && btnUp.getState()) {
    // released (after a press)
    sipCount++;
    countChange();

    okOn(OK_LED_DURATION_MILLIS);
    //digitalWrite(PIN_LED_SIP, !digitalRead(PIN_LED_SIP));
  }

  if (btnDn.update()) {
    if (btnDn.getState()) {
      // released (after a press)
      if (sipCount>0) {
        sipCount--;              
        countChange();
      }
      downPressRegistered=false;
      okOn(OK_LED_DURATION_MILLIS);
    } else {
      // pressed down
      downPressRegistered=true;
    }
  }

  if (okLedShutoffTime>0 && millis()>okLedShutoffTime) {
    okOff();
  }

  if (downPressRegistered && (millis()-btnDn.getStateChangeMillis())>3000UL) {
    // they've been holding the button for long enough
    downPressRegistered=false;
    
    Serial.print(F("Down is down for long enough. now="));
    Serial.print(millis());
    Serial.print(F(" stateChg="));
    Serial.println(btnDn.getStateChangeMillis());
    
    sipCount=0;
    countChange();

    okOn();
  }

  if (lcdBacklightTimer>0 && (millis()-lcdBacklightTimer)>LCD_BACKLIGHT_DURATION) {
    backlightOff();
  }

  if ((millis()-lastBatteryMillis)>BATTERY_CHECK_MILLIS) {
    lastBatteryMillis = millis();

    // do it twice, to give the internal ADC capacitor a chance to charge
    int prevBat = batteryLevel;
    batteryLevel = analogRead(PIN_BAT_ADC);
    batteryLevel = analogRead(PIN_BAT_ADC);

    if (prevBat != batteryLevel || batteryLevel<batteryMin || batteryLevel>batteryMax) {
      Serial.print(F("battery - adc="));
      Serial.println(batteryLevel);
    }

    if (batteryLevel < batteryMin) {
      Serial.print(F("  new min battery, replacing: "));
      Serial.println(batteryMin);
      batteryMin = batteryLevel;
      EEPROM.writeInt(EEPROM_BATTERY_MIN, batteryLevel);
    }

    if (batteryLevel > batteryMax) {
      Serial.print(F("  new max battery, replacing: "));
      Serial.println(batteryMax);
      batteryMax = batteryLevel;
      EEPROM.writeInt(EEPROM_BATTERY_MAX, batteryLevel);
    }

    lcdBattery();
  }

  if ((millis()-lastReportMillis)>REPORT_DURATION_MILLIS) {
    lastReportMillis = millis();

    reportCnt++;

    lcdTimer();

    //if ((millis() - lastSipMillis) > (reminderMinutes*60*1000)) {
    if (reminderState!=REMINDER_STATE_ALERT && (millis()-lastSipMillis)>reminderMillis) {
      alertOn();
    }
  }
  
}

void okOn(unsigned long duration=0) {
  digitalWrite(PIN_LED_OK, HIGH);
  if (duration>0) {
    okLedShutoffTime = millis() + duration;
  }
}

void okOff() {
  okLedShutoffTime=0;
  digitalWrite(PIN_LED_OK, LOW);
}

void backlightOff() {
  // de-implementing the off function, just keep the damn thing on
  //lcd.noBacklight();
  lcdBacklightTimer = 0;
}

void backlightOn(boolean wantTimer=true) {
  lcd.backlight();
  lcdBacklightTimer = wantTimer ? millis() : 0;
}

void alertOn() {
  reminderState = REMINDER_STATE_ALERT;
  digitalWrite(PIN_LED_SIP, HIGH);  

  // turn on the backlight and keep it on (may waste battery, so time that silly thing out)
  backlightOn(false);
}

void alertOff() {
  // update the reminder state
  reminderState = REMINDER_STATE_QUIET;
  digitalWrite(PIN_LED_SIP, LOW);  

  // turn on the backlight for a period of time
  backlightOn();
}

void countChange() {

  // update last click time
  lastSipMillis = millis();

  alertOff();

  // update the display
  lcdTimer();
  lcdSipCount();
  lcdProgressBar();
}

void lcdBattery() {
  lcd.setCursor(13, 1);
  lcd.print("   ");
  lcd.setCursor(13, 1);
  lcd.print(batteryLevel);
}

void lcdProgressBar() {
  /* 
   * we have 10 chars to display a progress bar.
   * end caps have 3 bars, middles have 5.
   * Total of 46 bars across 10 chars.
   * 
   * 1) map how many sips we have to the number of bars we have.
   * 2) left
   * 3) full mids
   * 4) progress mid
   * 5) empty mids
   * 6) right
   *  
   */
  byte barTarget = 46;
  byte barCount = round(mapf(sipCount, 0, sipTarget, 0, barTarget));

  byte newStates[BAR_COUNT];
  byte barChars[BAR_COUNT] = {0, CHAR_MID_BLANK, CHAR_MID_BLANK, CHAR_MID_BLANK, CHAR_MID_BLANK, CHAR_MID_BLANK, CHAR_MID_BLANK, CHAR_MID_BLANK, CHAR_MID_BLANK, 4};

  // values 0-3 are left
  newStates[0] = (barCount >=3) ? 3 : barCount;

  // values 4-43 are mids
  byte midBarCount = barCount;
  if (midBarCount > 43) {
    midBarCount = 40;
  } else if (midBarCount < 4) {
    midBarCount = 0;
  } else {
    midBarCount -= 3;
  }

  /* walk through each middle char, and figure
   * if its blank, full, or in progress
   */
  for (byte midIdx=0; midIdx<MID_COUNT; midIdx++) {
    if (midIdx < midBarCount/5) {
      // full bar
      newStates[midIdx+1] = 5;
      barChars[midIdx+1] = CHAR_MID_FULL;
    } else if (midIdx > midBarCount/5) {
      // empty bar
      newStates[midIdx+1] = 0;
      barChars[midIdx+1] = CHAR_MID_BLANK;
    } else {
      // progress bar
      byte partialBars = midBarCount % 5;
      newStates[midIdx+1] = partialBars;
      barChars[midIdx+1] = CHAR_MID_PROG;

      // this one is special, if the state changed, need to customize a new char
      if (newStates[midIdx+1] != barStates[midIdx+1] || partialBars==0) {
        lcd.createChar(CHAR_MID_PROG, bar_mid[newStates[midIdx+1]]);
      }
    }

  }


  // values 44-46 are right
  int barsRemaining = barTarget - barCount;
  if (barsRemaining > 3) {
    newStates[BAR_COUNT-1] = 0;
  } else if (barsRemaining <= 0) {
    newStates[BAR_COUNT-1] = 3;
  } else {
    newStates[BAR_COUNT-1] = 3 - barsRemaining;
  }

  // update the special chars for left and right
  if (newStates[0] != barStates[0]) {
    lcd.createChar(CHAR_LEFT, bar_left[newStates[0]]);
  }

  if (newStates[BAR_COUNT-1] != barStates[BAR_COUNT-1]) {
    lcd.createChar(CHAR_RIGHT, bar_right[newStates[BAR_COUNT-1]]);
  }

  // update display for any mis-matches
  for (uint8_t idx=0; idx<BAR_COUNT; idx++) {
    //if (newStates[idx] != barStates[idx]) {
      lcd.setCursor(idx , 0);
      lcd.write(byte(barChars[idx]));
    //}

    barStates[idx] = newStates[idx];
  }

  
}

/* float version of arduino built-in map(), allows the first button click to round up instead of trunc down */
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void lcdTimer() {

  //digitalWrite(PIN_LED_OK, !digitalRead(PIN_LED_OK));

  unsigned long dur = (millis() - lastSipMillis) / 1000UL;

  boolean printTimer=false;
  if (printTimer) {
    Serial.print("timer - millis=");
    Serial.print(dur);
  }
  
  // hh:mm:ss
  long h = dur / (60*60);
  dur = dur - h*60*60;
  long m = dur / 60;
  dur = dur - m*60;
  long s = dur;

  if (printTimer) {
    Serial.print(" h=");
    Serial.print(h);
    Serial.print(" m=");
    Serial.print(m);
    Serial.print(" s=");
    Serial.println(s);
  }
  
  lcd.setCursor(0, 1);
  if (h < 10) {
    lcd.print("0");
  }
  // truncate to 2 digits, more than 99 hours we really dont care to differentiate
  lcd.print(h%100);

  lcd.print(":");

  if (m < 10) {
    lcd.print("0");
  }
  lcd.print(m);

  lcd.print(":");

  if (s < 10) {
    lcd.print("0");
  }
  lcd.print(s);
}

void lcdSipCount() {
  lcd.setCursor(11,0);
  if (sipCount<10) {
    lcd.print(' ');
  }
  lcd.print(sipCount);
  lcd.setCursor(13,0);
  lcd.print('/');
  lcd.print(sipTarget);
}
