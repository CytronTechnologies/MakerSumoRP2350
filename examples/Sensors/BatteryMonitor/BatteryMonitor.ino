/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example prints the motor supply voltage once a second and warns
 * with the buzzer when it drops below LOW_BATTERY_VOLTS. Set that constant
 * for your pack: 6.6 V for 2S, 9.9 V for 3S.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

const float LOW_BATTERY_VOLTS = 6.6;

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
}

void loop() {
    float volts = MakerSumo.readBatteryVoltage();

    Serial.print("Battery: ");
    Serial.print(volts, 2);
    Serial.println(" V");

    if (volts < LOW_BATTERY_VOLTS) {
        MakerSumo.playTone(NOTE_C4, 150);
        delay(100);
        MakerSumo.playTone(NOTE_C4, 150);
        delay(600);
    } else {
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        delay(900);
    }
}
