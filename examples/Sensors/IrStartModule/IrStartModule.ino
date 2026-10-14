/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example shows how a competition IR start module on the IRST pin
 * works: output LOW = waiting, HIGH = START, back to LOW = STOP. Without a
 * module the START button does the same job. Only the LED and buzzer are
 * used here; see Sumo/AutoSumo for the full match flow with motors.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

bool running = false;

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
}

void loop() {
    bool startSignal = MakerSumo.irStartActive() || MakerSumo.isStartPressed();

    if (!running && startSignal) {
        running = true;
        Serial.println("START");
        MakerSumo.playTone(NOTE_C6, 100);
        digitalWrite(LED, HIGH);
    } else if (running && !MakerSumo.irStartActive() && !MakerSumo.isStartPressed()) {
        running = false;
        Serial.println("STOP");
        MakerSumo.playTone(NOTE_C4, 200);
        digitalWrite(LED, LOW);
    }

    if (!running) {
        digitalWrite(LED, (millis() / 500) % 2);
    }
    delay(10);
}
