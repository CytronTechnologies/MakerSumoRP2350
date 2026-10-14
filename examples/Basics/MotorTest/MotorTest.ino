/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example drives both motors through a short pattern when START button 
 * is pressed: forward, backward, turn left, turn right, then stop. Put the
 * robot on a stand before pressing the START button. If it runs backward when it
 * should go forward, swap the motor wires or change MOTOR_FORWARD_LEVEL.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

const int SPEED = 150;      // 0-255

void waitForStart() {
    while (!MakerSumo.isStartPressed()) {
        digitalWrite(LED, (millis() / 500) % 2);  
        delay(10);
    }
    delay(50);                                     
    while (MakerSumo.isStartPressed()) delay(10); 
    digitalWrite(LED, HIGH);
}

void setup() {
    MakerSumo.begin();
}

void loop() {
    waitForStart();

    MakerSumo.playTone(NOTE_C6, 100);
    MakerSumo.moveForward(SPEED);   delay(1000);
    MakerSumo.stop();               delay(300);
    MakerSumo.moveBackward(SPEED);  delay(1000);
    MakerSumo.stop();               delay(300);
    MakerSumo.turnLeft(SPEED);      delay(600);
    MakerSumo.stop();               delay(300);
    MakerSumo.turnRight(SPEED);     delay(600);
    MakerSumo.stop();

    if (MakerSumo.motorFault()) {
        MakerSumo.playTone(NOTE_C4, 400);         
    }
    digitalWrite(LED, LOW);
}
