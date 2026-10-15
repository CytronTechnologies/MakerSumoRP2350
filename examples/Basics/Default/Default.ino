/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * Default demo program for the Cytron Maker Sumo RP2350.
 *
 * On startup the buzzer plays the Cytron startup chime while the user LED
 * blinks every 500 ms. Pressing START plays the chime again.
 *
 * The blink and the chime both use millis() so they run at the same time
 * without blocking each other. The motors are not used.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

const unsigned long ledInterval = 500;
unsigned long lastLedUpdate = 0;
bool ledOn = false;

// Button debounce (START is active LOW)
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

const int chimeFrequencies[] = {
  262, 330, 392, 440, 494, 523,
  1047,
  0,
  1319, 1047, 1319,
  0,
  1047
};

const int chimeDurations[] = {
  140, 140, 140, 140, 140, 140,
  420,
  60,
  55, 55, 55,
  50,
  600
};

const int chimeLength = sizeof(chimeFrequencies) / sizeof(chimeFrequencies[0]);

int chimeIndex = 0;
bool chimePlaying = false;
unsigned long chimeNoteStart = 0;

void playCurrentNote()
{
  if (chimeFrequencies[chimeIndex] == 0) {
    noTone(BUZZER);
  }
  else {
    tone(BUZZER, chimeFrequencies[chimeIndex], chimeDurations[chimeIndex]);
  }
}

void startChime()
{
  chimeIndex = 0;
  chimePlaying = true;
  chimeNoteStart = millis();
  playCurrentNote();
}

void updateChime()
{
  if (!chimePlaying) {
    return;
  }

  if (millis() - chimeNoteStart >= (unsigned long)chimeDurations[chimeIndex]) {
    chimeIndex++;

    if (chimeIndex >= chimeLength) {
      noTone(BUZZER);
      chimePlaying = false;
      return;
    }

    chimeNoteStart = millis();
    playCurrentNote();
  }
}

void updateLed()
{
  if (millis() - lastLedUpdate < ledInterval) {
    return;
  }

  lastLedUpdate = millis();
  ledOn = !ledOn;
  digitalWrite(LED, ledOn);
}

void updateButton()
{
  int reading = digitalRead(START);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        startChime();
      }
    }
  }

  lastButtonState = reading;
}


void setup()
{
  MakerSumo.begin();          
  digitalWrite(LED, HIGH);
  ledOn = true;
  lastLedUpdate = millis();
  startChime();
}


void loop()
{
  updateButton();
  updateLed();
  updateChime();
}
