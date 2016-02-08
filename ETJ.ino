/* ===========================================================================
 *  ELECTRONIC TIP JAR
 * ===========================================================================
 *  AUTHOR:  Joshua Cole (hello@joshuacole.me)
 *  VERSION: See 'Constants > Settings'
 *  DATE:    2016-02-08
 * ---------------------------------------------------------------------------
 *  DESCRIPTION
 * ---------------------------------------------------------------------------
 *  Combined with a 3D printed case this project will allow you to keep a
 *  total of your negative and positive thoughts and behaviours. It will
 *  display the totals on the LCD and is designed to be reset at the end of
 *  each week and the total jotted down elsewhere. It's a great way to improve
 *  your mindfulness skills by helping you to modify your thoughts, behaviours
 *  and habits.
 * ---------------------------------------------------------------------------
 *  THE TOTALS
 * ---------------------------------------------------------------------------
 *  The project will track 2 negative totals and 3 positive totals, use them
 *  as you will but this is what I found most helpful.
 *   - Negative  1  =  A negative thought/behaviour you catch at the time.
 *   - Negative  2  =  A negative thought/behaviour you catch later on.
 *   - Positive  5  =  A positive thought/behaviour.
 *   - Positive 10  =  Taking a small step outside your comfort zone.
 *   - Positive 20  =  Taking a large step outside your comfort zone.
 * ============================================================================
 */

// Required Libraries.
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Constants > Settings.
const String projectVersion          = "2.0.1";     //the version string to track changes.
const int    startupWaitMS           = 5000;        //the time to show the startup message if no buttons are pressed.
const int    incrementWaitMS         = 1000;        //the time to show the increment progress bar before increasing the chosen total.
const int    resetWaitMS             = 3000;        //the time to show the reset progress bar before resetting the current totals.
const byte   progressBarMaxBlocks    = 16;          //the max number of characters to use for the progress bar.
const String progressBarBlockChr     = "*";         //the character to use when displaying progress bars.
const float  roomLightThreshold      = 10.0;        //the percentage of light at which to switch off the LCD when it gets dark.
const int    roomLightWaitMS         = 5000;        //the amount of time to wait when the light changes before switching the LCD on/off.
const bool   enableAutoBacklight     = true;        //set false to keep the LCD backlight always on.
const int    statusWaitMS            = 1000;        //the amount of time to wait whilst a tip button is being pressed.
const int    toneIncrement           = 750;         //the value for tone() when an increment has occurred.
const int    toneReset               = 2000;        //the value for tone() when a reset has occurred.

// Constants > Pins.
const byte   outLCDBacklightPin      =  4;  //atmega pin  6
const byte   inHistoryBtnPin         = 11;  //atmega pin 17
const byte   inResetBtnPin           = 10;  //atmega pin 16
const byte   outPiezoPin             =  9;  //atmega pin 15
const byte   outResetLEDPin          = 12;  //atmega pin 18
const byte   inLightSensorPin        = A0;  //atmega pin 23
const byte   inIncrementBtnLadderPin = A1;  //atmega pin 24
const byte   outLCDRSPin             =  2;  //atmega pin  4 / lcd pin  4
const byte   outLCDEnablePin         =  3;  //atmega pin  5 / lcd pin  6
const byte   outLCDD4Pin             =  5;  //atmega pin 11 / lcd pin 11
const byte   outLCDD5Pin             =  6;  //atmega pin 12 / lcd pin 12
const byte   outLCDD6Pin             =  7;  //atmega pin 13 / lcd pin 13
const byte   outLCDD7Pin             =  8;  //atmega pin 14 / lcd pin 14

// Constants > EEPROM addresses.
const int    intSize                 = sizeof(int);
const byte   memN1                   = intSize * 0;
const byte   memN1History            = intSize * 1;
const byte   memN2                   = intSize * 2;
const byte   memN2History            = intSize * 3;
const byte   memP5                   = intSize * 4;
const byte   memP5History            = intSize * 5;
const byte   memP10                  = intSize * 6;
const byte   memP10History           = intSize * 7;
const byte   memP20                  = intSize * 8;
const byte   memP20History           = intSize * 9;

// Global Variables.
unsigned long curTime                = 0;
bool          isRoomDark             = false;
float         curRoomLight           = 0.0;
unsigned long roomLightDetectTime    = 0;
int           isLCDBacklightOn       = true;
int           progressBarCurBlock    = 0;
unsigned long progressBarPrevTime    = 0;
String        incrementDetectBtn     = String("");
unsigned long resetDetectTime        = 0;

// Global Variables > Totals.
int           totalN1                = 0;
int           totalN1History         = 0;
int           totalN2                = 0;
int           totalN2History         = 0;
int           totalP5                = 0;
int           totalP5History         = 0;
int           totalP10               = 0;
int           totalP10History        = 0;
int           totalP20               = 0;
int           totalP20History        = 0;

// Global Variables > Buttons.
int           anyButtonsPressed      = false;
int           btnN1                  = LOW;
int           btnN1Prev              = LOW;
int           btnN2                  = LOW;
int           btnN2Prev              = LOW;
int           btnP5                  = LOW;
int           btnP5Prev              = LOW;
int           btnP10                 = LOW;
int           btnP10Prev             = LOW;
int           btnP20                 = LOW;
int           btnP20Prev             = LOW;
int           btnHistory             = LOW;
int           btnHistoryPrev         = LOW;
int           btnReset               = LOW;
int           btnResetPrev           = LOW;

// Global Variables > Mode.
//  "startup"   The startup message shown on boot.
//  "totals"    The main display showing the totals.
//  "increment" The progress bar shown when holding a total button.
//  "history"   The history display showing totals from the previous week.
//  "reset"     The progress bar shown when holding the reset button.
String mode = String("");

// Initialise LCD.
LiquidCrystal lcd (outLCDRSPin, outLCDEnablePin, outLCDD4Pin, outLCDD5Pin, outLCDD6Pin, outLCDD7Pin);

/*
 * Initialise components.
 */
void setup () {

  // Setup the pins.
  pinMode(inHistoryBtnPin,         INPUT);
  pinMode(inResetBtnPin,           INPUT);
  pinMode(outPiezoPin,             OUTPUT);
  pinMode(outResetLEDPin,          OUTPUT);
  pinMode(outLCDBacklightPin,      OUTPUT);
  pinMode(inLightSensorPin,        INPUT);
  pinMode(inIncrementBtnLadderPin, INPUT);

  // Write initial pin values.
  digitalWrite(outPiezoPin,        LOW);
  digitalWrite(outResetLEDPin,     LOW);
  digitalWrite(outLCDBacklightPin, HIGH);

  // Prepare the LCD for use.
  lcd.begin(16, 2);
  lcd.clear();

  // Display the boot message.
  lcd.print("ETJ v" + projectVersion);

  // Debugging.
  Serial.begin(9600);

  // Read EEPROM.
  memLoadStoredTotals();

  // Change the mode.
  setMode("startup", "");

}

/*
 * Main program loop.
 */
void loop () {

  // Store this globally (overflows to zero after approx. 50 days).
  curTime = millis();

  // Check if any buttons are being pressed and store the result in the global variables.
  detectButtons();

  // Switch the LCD backlight on/off depending on the brightness of the room.
  manageLCDBacklight();

  // MODE: STARTUP
  if (isMode("startup")) {
    if (showProgressBar(startupWaitMS) == true || anyButtonsPressed == true) {
      lcd.clear();
      resetProgressBar();
      setMode("totals", "");
    }
  }

  // MODE: TOTALS
  else if (isMode("totals")) {
    //no need for anything here.
  }

  // MODE: INCREMENT
  else if (isMode("increment")) {
    // Cancel the increment if the button is no longer pressed.
    if (
      (incrementDetectBtn == "N1"  && !btnN1)  ||
      (incrementDetectBtn == "N2"  && !btnN2)  ||
      (incrementDetectBtn == "P5"  && !btnP5)  ||
      (incrementDetectBtn == "P10" && !btnP10) ||
      (incrementDetectBtn == "P20" && !btnP20)
    ) {
      lcd.clear();
      resetProgressBar();
      setMode("totals", "");
    }
    
    // Do the increment.
    if (showProgressBar(incrementWaitMS) == true) {
      lcd.clear();
      resetProgressBar();

      if      (incrementDetectBtn == "N1")  { totalN1++;  }
      else if (incrementDetectBtn == "N2")  { totalN2++;  }
      else if (incrementDetectBtn == "P5")  { totalP5++;  }
      else if (incrementDetectBtn == "P10") { totalP10++; }
      else if (incrementDetectBtn == "P20") { totalP20++; }

      // Save to EEPROM.
      memWriteTotal(incrementDetectBtn);

      tone(outPiezoPin, toneIncrement, 400);
      
      setMode("status", "Incremented!");
    }
  }

  // MODE: HISTORY
  else if (isMode("history")) {
    //no need for anything here.
  }

  // MODE: RESET
  else if (isMode("reset")) {
    // Cancel the reset if the button is no longer pressed.
    if (!btnReset) {
      lcd.clear();
      resetProgressBar();
      digitalWrite(outResetLEDPin, LOW);
      setMode("totals", "");
    }

    // Do the reset.
    if (showProgressBar(resetWaitMS) == true) {
      lcd.clear();
      resetProgressBar();
      digitalWrite(outResetLEDPin, LOW);

      totalN1History  = totalN1;
      totalN2History  = totalN2;
      totalP5History  = totalP5;
      totalP10History = totalP10;
      totalP20History = totalP20;
      totalN1         = 0;
      totalN2         = 0;
      totalP5         = 0;
      totalP10        = 0;
      totalP20        = 0;

      // Update the values stored in memory.
      memMoveToHistory();

      tone(outPiezoPin, toneReset, 800);

      setMode("status", "Reset!");
    }
  }

  else if (isMode("status")) {

  }
  
  delay(10);
  
}

/*
 * Returns true if the given string matches the current mode.
 */
bool isMode (String str) {
  if (mode == str) { return true; }
  return false;
}

/*
 * Changes the current mode.
 */
void setMode (String newMode, String extra) {

  mode = newMode;

  if (newMode == "startup") {
    
  }

  else if (newMode == "totals") {
    printTotalsScreen(false);
  }

  else if (newMode == "increment") {
    incrementDetectBtn  = extra;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Inc. " + extra + "...");
  }

  else if (newMode == "history") {
    printTotalsScreen(true);
  }

  else if (newMode == "reset") {
    resetDetectTime = curTime;

    digitalWrite(outResetLEDPin, HIGH);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Resetting...");
  }

  else if (newMode == "status") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(extra);

    delay(statusWaitMS);

    setMode("totals", "");
  }
  
}

/*
 * Displays either the standard totals screen or the history totals screen depending on the parameter.
 */
void printTotalsScreen (bool showHistory) {

  String useN1;
  String useN2;
  String useP5;
  String useP10;
  String useP20;

  // Standard totals screen.
  if (showHistory == false) {
    useN1  = String(totalN1);
    useN2  = String(totalN2);
    useP5  = String(totalP5);
    useP10 = String(totalP10);
    useP20 = String(totalP20);
  }

  // History totals screen.
  else {
    useN1  = String(totalN1History);
    useN2  = String(totalN2History);
    useP5  = String(totalP5History);
    useP10 = String(totalP10History);
    useP20 = String(totalP20History);
  }

  // Print to the LCD.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(useN1);
  lcd.setCursor(12, 0);
  lcd.print(useN2);
  lcd.setCursor(0, 1);
  lcd.print(useP5);
  lcd.setCursor(6, 1);
  lcd.print(useP10);
  lcd.setCursor(12, 1);
  lcd.print(useP20);
  
}

/*
 * Displays a progress bar on the second row of the LCD and increments it over a specified number of milliseconds.
 * Returns true once the progress bar has run its course.
 */
bool showProgressBar (int displayForMS) {

  // Setup.
  int timePerBlock = displayForMS / progressBarMaxBlocks;
  if (progressBarPrevTime == 0) {
    progressBarPrevTime = curTime;
    progressBarCurBlock = 0;
  }

  // It's time to show the next block.
  if (curTime - progressBarPrevTime >= timePerBlock) {

    // We're finished - the last block has now been shown for the same time interval.
    if (progressBarCurBlock > progressBarMaxBlocks) {
      resetProgressBar();
      return true;
    }

    // Add a block.
    progressBarCurBlock++;

    // Build the block string.
    String blockStr = String("");
    for (int i = 1 ; i < progressBarCurBlock ; i++) {
      blockStr = blockStr + progressBarBlockChr;
    }

    // Display the block string.
    lcd.setCursor(0, 1);
    lcd.print(blockStr);

    // Remember the time we showed the last block.
    progressBarPrevTime = curTime;
    
  }

  // Not finished - we still have blocks to show.
  return false;
  
}

/*
 * Resets the progress bar global variables. This must be called manually if the progress bar is stopped halfway through.
 */
void resetProgressBar () {
  progressBarPrevTime = 0;
  progressBarCurBlock = 0;
}

/*
 * Stores the values on each of the buttons at the current moment in time.
 */
bool detectButtons () {

  // Read buttons.
  int incVal = analogRead(inIncrementBtnLadderPin);
  btnHistory = digitalRead(inHistoryBtnPin);
  btnReset   = digitalRead(inResetBtnPin);

  // Reset ladder buttons.
  btnN1      = LOW;
  btnN2      = LOW;
  btnP5      = LOW;
  btnP10     = LOW;
  btnP20     = LOW;

  // Debugging for the resistor ladder.
  //Serial.print("ladder val: ");
  //Serial.println(incVal);

  // Handle the history button.
  if (btnHistory && !isMode("history")) { setMode("history", ""); }
  else if (!btnHistory && isMode("history")) { setMode("totals", ""); }

  // Handle the reset button.
  if (btnReset && !isMode("reset")) { setMode("reset", ""); }
  
  // Handle the ladder buttons.
  if      (incVal >= 235 && incVal <  255) { btnP20 = HIGH; }
  else if (incVal >= 315 && incVal <  335) { btnP10 = HIGH; }
  else if (incVal >= 470 && incVal <  490) { btnP5  = HIGH; }
  else if (incVal >= 900 && incVal <  920) { btnN2  = HIGH; }
  else if (incVal >= 990 && incVal < 1010) { btnN1  = HIGH; }

  if      (btnP20 && !isMode("increment")) { setMode("increment", "P20"); }
  else if (btnP10 && !isMode("increment")) { setMode("increment", "P10"); }
  else if (btnP5  && !isMode("increment")) { setMode("increment", "P5");  }
  else if (btnN2  && !isMode("increment")) { setMode("increment", "N2");  }
  else if (btnN1  && !isMode("increment")) { setMode("increment", "N1");  }
  
  // Flag if ANY of the buttons are being pressed.
  if (btnN1 || btnN2 || btnP5 || btnP10 || btnP20 || btnHistory || btnReset) {
    
    anyButtonsPressed = true;

    // Activate the LCD backlight if we've pressed a button.
    if (isRoomDark) { 
      isRoomDark = false;
      activateLCDBacklight();
    }
    
  } else {
    anyButtonsPressed = false;
  }
  
}

/*
 * Stores the value on the light sensor.
 */
void manageLCDBacklight () {

  // Get the light level as a percentage.
  float lightReading = analogRead(inLightSensorPin);
  curRoomLight = (lightReading / 1023.0) * 100.0;

  // Debugging for the LDR.
  //Serial.print("curRoomLight: ");
  //Serial.println(curRoomLight);

  // Room is currently marked as 'light'.
  if (isRoomDark == false) {

    // Light has fallen below the threshold for the first time, start the timer.
    if (roomLightDetectTime == 0 && curRoomLight < roomLightThreshold) {
      roomLightDetectTime = curTime;
    }

    // Timer has started.
    else if (roomLightDetectTime > 0) {

      // Timer is finished, turn off the LCD.
      if (curTime - roomLightDetectTime >= roomLightWaitMS) {
        deactivateLCDBacklight();
        roomLightDetectTime = 0;
        isRoomDark          = true;
      }

      // Light has gone up again, stop the timer.
      else if (curRoomLight >= roomLightThreshold) {
        roomLightDetectTime = 0;
      }
      
    }
    
  }

  // Room is currently marked as 'dark'.
  else {

    // Light has risen above the threshold for the first time, start the timer.
    if (roomLightDetectTime == 0 && curRoomLight >= roomLightThreshold) {
      roomLightDetectTime = curTime;
    }

    // Timer has started.
    else if (roomLightDetectTime > 0) {

      // Timer is finished, turn on the LCD.
      if (curTime - roomLightDetectTime >= roomLightWaitMS) {
        activateLCDBacklight();
        roomLightDetectTime = 0;
        isRoomDark          = false;
      }

      // Light has gone down again, stop the timer.
      else if (curRoomLight < roomLightThreshold) {
        roomLightDetectTime = 0;
      }
      
    }
    
  }
  
}

/*
 * Turns on the LCD backlight.
 */
void activateLCDBacklight () {
  digitalWrite(outLCDBacklightPin, HIGH);
  isLCDBacklightOn = true;
}

/*
 * Turns off the LCD backlight.
 */
void deactivateLCDBacklight () {
  if (enableAutoBacklight) {
    digitalWrite(outLCDBacklightPin, LOW);
    isLCDBacklightOn = false;
  }
}

/*
 * Reads in all the stored totals from the EEPROM.
 */
void memLoadStoredTotals () {

  int tmpN1;
  EEPROM.get(memN1, tmpN1);
  if (tmpN1 > -1) { totalN1 = tmpN1; }

  int tmpN1History;
  EEPROM.get(memN1History, tmpN1History);
  if (tmpN1History > -1) { totalN1History = tmpN1History; }

  int tmpN2;
  EEPROM.get(memN2, tmpN2);
  if (tmpN2 > -1) { totalN2 = tmpN2; }

  int tmpN2History;
  EEPROM.get(memN2History, tmpN2History);
  if (tmpN2History > -1) { totalN2History = tmpN2History; }

  int tmpP5;
  EEPROM.get(memP5, tmpP5);
  if (tmpP5 > -1) { totalP5 = tmpP5; }

  int tmpP5History;
  EEPROM.get(memP5History, tmpP5History);
  if (tmpP5History > -1) { totalP5History = tmpP5History; }

  int tmpP10;
  EEPROM.get(memP10, tmpP10);
  if (tmpP10 > -1) { totalP10 = tmpP10; }

  int tmpP10History;
  EEPROM.get(memP10History, tmpP10History);
  if (tmpP10History > -1) { totalP10History = tmpP10History; }

  int tmpP20;
  EEPROM.get(memP20, tmpP20);
  if (tmpP20 > -1) { totalP20 = tmpP20; }

  int tmpP20History;
  EEPROM.get(memP20History, tmpP20History);
  if (tmpP20History > -1) { totalP20History = tmpP20History; }

  //Serial.print("XXX: ");
  //Serial.println(XXX);
  
}

/*
 * Writes the given total to the EEPROM.
 */
void memWriteTotal (String which) {

  byte memAddr;
  int  value;

  if      (which == "N1")  { memAddr = memN1;  value = totalN1;  }
  else if (which == "N2")  { memAddr = memN2;  value = totalN2;  }
  else if (which == "P5")  { memAddr = memP5;  value = totalP5;  }
  else if (which == "P10") { memAddr = memP10; value = totalP10; }
  else if (which == "P20") { memAddr = memP20; value = totalP20; }

  EEPROM.put(memAddr, value);
  
}

/*
 * Writes the new history totals to the EEPROM and zeroes the current totals.
 */
void memMoveToHistory () {

  EEPROM.put(memN1,         totalN1);
  EEPROM.put(memN2,         totalN2);
  EEPROM.put(memP5,         totalP5);
  EEPROM.put(memP10,        totalP10);
  EEPROM.put(memP20,        totalP20);
  
  EEPROM.put(memN1History,  totalN1History);
  EEPROM.put(memN2History,  totalN2History);
  EEPROM.put(memP5History,  totalP5History);
  EEPROM.put(memP10History, totalP10History);
  EEPROM.put(memP20History, totalP20History);
  
}

