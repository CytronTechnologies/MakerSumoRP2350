/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example is a complete, easy-to-read autonomous sumo program.
 * Change the constants at the top to tune it. Power on with the robot on
 * the black surface, then press START for a 5 s countdown or use an IR
 * start module. The robot searches, pushes when it sees the opponent and
 * backs away from the white border. For adjustable strategies and the USB
 * configurator, see Sumo/AutoRcSumo.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <MakerSumoRP2350.h>

// ---- Tuning ------------------------------------------------------------
const int SEARCH_SPEED_L  = 120;    
const int SEARCH_SPEED_R  = 80;
const int ATTACK_SPEED    = 255;
const int TURN_SPEED      = 180;
const int ESCAPE_SPEED    = 255;   
const int ESCAPE_BACK_MS  = 250;
const int ESCAPE_TURN_MS  = 300;
const int COUNTDOWN_MS    = 5000;

// ---- State -------------------------------------------------------------
int  thresholdL, thresholdR;
bool startedByIr = false;

void calibrateEdges() {
    int sensitivity = map(analogRead(POT), 0, 1023, 25, 75);   // % of black reading
    thresholdL = analogRead(EDGE_L) * sensitivity / 100;
    thresholdR = analogRead(EDGE_R) * sensitivity / 100;
}

void waitForStart() {
    MakerSumo.stop();
    for (;;) {
        digitalWrite(LED, (millis() / 500) % 2);         
        if (MakerSumo.irStartActive()) {                   
            startedByIr = true;
            break;
        }
        if (MakerSumo.isStartPressed()) {                 
            delay(50);
            while (MakerSumo.isStartPressed()) delay(10);
            unsigned long t0 = millis();
            while (millis() - t0 < COUNTDOWN_MS) {
                digitalWrite(LED, (millis() / 250) % 2);    
                delay(10);
            }
            startedByIr = false;
            break;
        }
        delay(10);
    }
    calibrateEdges();
    MakerSumo.playTone(NOTE_C6, 100);
    digitalWrite(LED, HIGH);
}

void escapeEdge(bool leftSide) {
    MakerSumo.moveBackward(ESCAPE_SPEED);
    delay(ESCAPE_BACK_MS);
    if (leftSide) MakerSumo.turnRight(TURN_SPEED);   
    else          MakerSumo.turnLeft(TURN_SPEED);
    delay(ESCAPE_TURN_MS);
}

void setup() {
    MakerSumo.begin();
    waitForStart();
}

void loop() {
    // Stop command from the IR module ends the match.
    if (startedByIr && !MakerSumo.irStartActive()) {
        MakerSumo.stop();
        digitalWrite(LED, LOW);
        while (true) delay(100);
    }

    // Edge first: never drive off the ring.
    bool edgeL = analogRead(EDGE_L) < thresholdL;
    bool edgeR = analogRead(EDGE_R) < thresholdR;
    if (edgeL || edgeR) {
        escapeEdge(edgeL);
        return;
    }

    // Opponent: bit0 L, bit1 FL, bit2 FC, bit3 FR, bit4 R
    uint8_t opp = MakerSumo.readOpponents();
    if (opp & 0x04) {                       // dead ahead: push
        MakerSumo.moveForward(ATTACK_SPEED);
    } else if (opp & 0x02) {                // front-left: curve left
        MakerSumo.setMotorSpeed(MOTOR_L, ATTACK_SPEED / 2);
        MakerSumo.setMotorSpeed(MOTOR_R, ATTACK_SPEED);
    } else if (opp & 0x08) {                // front-right: curve right
        MakerSumo.setMotorSpeed(MOTOR_L, ATTACK_SPEED);
        MakerSumo.setMotorSpeed(MOTOR_R, ATTACK_SPEED / 2);
    } else if (opp & 0x01) {                // left side: spin left
        MakerSumo.turnLeft(TURN_SPEED);
    } else if (opp & 0x10) {                // right side: spin right
        MakerSumo.turnRight(TURN_SPEED);
    } else {                                // nothing seen: search
        MakerSumo.setMotorSpeed(MOTOR_L, SEARCH_SPEED_L);
        MakerSumo.setMotorSpeed(MOTOR_R, SEARCH_SPEED_R);
    }
    delay(5);
}
