/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example exercises every output when START is pressed: buzzer, LED,
 * then each motor forward and backward at half speed. Lift the robot off
 * the table first.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

// Press the START button to run the output test: buzzer, LED, then each motor
// forward and backward at half speed. Lift the robot off the table first.
#include <MakerSumoRP2350.h>

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
}

void loop() {
    if (!MakerSumo.isStartPressed()) return;
    delay(50);
    while (MakerSumo.isStartPressed()) delay(10);

    MakerSumo.playTone(NOTE_C5, 100);
    MakerSumo.playTone(NOTE_G5, 150);
    for (int i = 0; i < 3; i++) { digitalWrite(LED, HIGH); delay(100); digitalWrite(LED, LOW); delay(100); }

    Serial.println("Left forward");   MakerSumo.setMotorSpeed(MOTOR_L,  128); delay(800); MakerSumo.stop(); delay(300);
    Serial.println("Left backward");  MakerSumo.setMotorSpeed(MOTOR_L, -128); delay(800); MakerSumo.stop(); delay(300);
    Serial.println("Right forward");  MakerSumo.setMotorSpeed(MOTOR_R,  128); delay(800); MakerSumo.stop(); delay(300);
    Serial.println("Right backward"); MakerSumo.setMotorSpeed(MOTOR_R, -128); delay(800); MakerSumo.stop();
    Serial.printf("Fault: %d\n", MakerSumo.motorFault());
}
