/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example prints every input once 250ms: DIP switches, START
 * button, IR start input, the five opponent sensors, both edge sensors,
 * the potentiometer, battery voltage and the motor driver fault lines.
 * Use it first to check the board and sensor wiring.
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
    uint8_t opp = MakerSumo.readOpponents();
    Serial.printf("DIP = %d  BTN = %d  IR = %d  OPP[L FL FC FR R] = %d %d %d %d %d  ",
                  MakerSumo.readDipSwitch(), MakerSumo.isStartPressed(),
                  MakerSumo.irStartActive(),
                  (opp >> 0) & 1, (opp >> 1) & 1, (opp >> 2) & 1,
                  (opp >> 3) & 1, (opp >> 4) & 1);
    Serial.printf("EDGE_L = %d  EDGE_R = %d  POT = %d  VM = %.2fV  FAULT = %d\n",
                  analogRead(EDGE_L), analogRead(EDGE_R), analogRead(POT),
                  MakerSumo.readBatteryVoltage(), MakerSumo.motorFault());
    digitalWrite(LED, MakerSumo.isStartPressed());
    delay(250);
}
