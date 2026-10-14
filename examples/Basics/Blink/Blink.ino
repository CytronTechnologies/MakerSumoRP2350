/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example flashes the on-board LED and plays a short melody on the
 * buzzer. It is the first sketch to try: it confirms that the arduino-pico
 * core, the board and the library are installed and that uploading works.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

const int melody[]   = { NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6 };
const int duration[] = { 8, 8, 8, 4 };   // 8 = eighth note, 4 = quarter note

void setup() {
    MakerSumo.begin();
    MakerSumo.playMelody(melody, duration, 4);
}

void loop() {
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    delay(800);
}
