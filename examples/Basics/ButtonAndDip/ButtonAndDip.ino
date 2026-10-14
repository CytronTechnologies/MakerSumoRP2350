/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example reads the START button and the three DIP switches.
 * The LED follows the button. Changing the DIP switches beeps once and
 * prints the new value. DIP value is 0-7 with SW1 as the most significant
 * bit, so ON OFF OFF = 4.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

int lastDip = -1;

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
}

void loop() {
    digitalWrite(LED, MakerSumo.isStartPressed()); // Press the START Button and you will see the User LED turns ON

    int dip = MakerSumo.readDipSwitch();
    if (dip != lastDip) {
        lastDip = dip;
        Serial.print("DIP = ");
        Serial.print(dip);
        Serial.print("  (SW1 ");
        Serial.print((dip & 4) ? "ON" : "OFF");
        Serial.print(", SW2 ");
        Serial.print((dip & 2) ? "ON" : "OFF");
        Serial.print(", SW3 ");
        Serial.print((dip & 1) ? "ON" : "OFF");
        Serial.println(")");
        MakerSumo.playTone(NOTE_G5, 60);
    }
    delay(20);
}
