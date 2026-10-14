/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example prints which of the five opponent sensors see something
 * and lights the LED when any of them does. Put your hand in front of each
 * sensor to test. 
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
}

void loop() {
    uint8_t opp = MakerSumo.readOpponents();   // bit0 L, bit1 FL, bit2 FC, bit3 FR, bit4 R

    bool left        = opp & 0x01;
    bool frontLeft   = opp & 0x02;
    bool frontCenter = opp & 0x04;
    bool frontRight  = opp & 0x08;
    bool right       = opp & 0x10;

    digitalWrite(LED, opp != 0);

    Serial.print(left        ? "L  " : ".  ");
    Serial.print(frontLeft   ? "FL " : ".  ");
    Serial.print(frontCenter ? "FC " : ".  ");
    Serial.print(frontRight  ? "FR " : ".  ");
    Serial.println(right     ? "R"   : ".");
    delay(100);
}
