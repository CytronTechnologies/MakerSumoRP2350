/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example prints the two edge sensor readings (0-1023) and lights
 * the LED when either one sees the white ring border. The black surface
 * reads high and the white border reads low. The threshold is taken from
 * the reading at power-on, so start with the robot on the black surface.
 * The potentiometer sets the sensitivity.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

int thresholdL, thresholdR;

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
    delay(200);

    // 25 % (pot fully left) to 75 % (pot fully right) of the black reading
    int sensitivity = map(analogRead(POT), 0, 1023, 25, 75);
    thresholdL = analogRead(EDGE_L) * sensitivity / 100;
    thresholdR = analogRead(EDGE_R) * sensitivity / 100;

    Serial.print("Thresholds: L=");
    Serial.print(thresholdL);
    Serial.print(" R=");
    Serial.println(thresholdR);
}

void loop() {
    int left  = analogRead(EDGE_L);
    int right = analogRead(EDGE_R);
    bool edgeL = left  < thresholdL;
    bool edgeR = right < thresholdR;

    digitalWrite(LED, edgeL || edgeR);

    Serial.print("L=");
    Serial.print(left);
    Serial.print(edgeL ? " EDGE   " : "        ");
    Serial.print("R=");
    Serial.print(right);
    Serial.println(edgeR ? " EDGE" : "");
    delay(100);
}
