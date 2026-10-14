/*******************************************************************************
 * MakerSumoRP2350 - board support for the Cytron Maker Sumo RP2350 Controller
 *
 * Pin map matches the CircuitPython board definition:
 *   ports/raspberrypi/boards/cytron_maker_sumo_rp2350/pins.c
 *
 * Core: arduino-pico (Earle Philhower). Either USB stack works.
 ******************************************************************************/
#pragma once

#include <Arduino.h>

// ---- Library version --------------------------------------------------------
#define MAKER_SUMO_RP2350_LIB_VERSION        "1.0.0"
#define MAKER_SUMO_RP2350_LIB_VERSION_MAJOR  1
#define MAKER_SUMO_RP2350_LIB_VERSION_MINOR  0
#define MAKER_SUMO_RP2350_LIB_VERSION_PATCH  0

// ---- USB identity (raspberrypi/usb-pid allocation) --------------------------
#define MAKER_SUMO_USB_VID           0x2E8A
#define MAKER_SUMO_USB_PID           0x112F
#define MAKER_SUMO_USB_MANUFACTURER  "Cytron"
#define MAKER_SUMO_USB_PRODUCT       "Maker Sumo RP2350"

// ---- Pin assignment ---------------------------------------------------------
#define USR_BTN   2     // user / START button, active LOW
#define START     USR_BTN
#define LED       3
#define DIP1      4     // SW1 (MSB of readDipSwitch)
#define DIP2      5
#define DIP3      6     // SW3 (LSB)
#define OPP_R     8     // opponent sensors, active LOW
#define PWM_R     9
#define DIR_R     10
#define OPP_L     11
#define BUZZER    12
#define DIR_L     14
#define PWM_L     15
#define SERVO1    16
#define SERVO2    17
#define IRST      18    // IR start module: LOW = idle/stop, HIGH = start
#define FAULT_L   19    // motor driver fault, active LOW
#define OPP_FL    20
#define OPP_FC    21
#define OPP_FR    22
#define FAULT_R   25
#define EDGE_L    26    // A0
#define EDGE_R    27    // A1
#define POT       28    // A2
#define VM_SENSE  29    // A3, motor supply through 1:11 divider
#define VBATT     VM_SENSE

#define GPIO1     SERVO1
#define GPIO2     SERVO2

#define MOTOR_L   0
#define MOTOR_R   1

// ---- Hardware polarity ------------------------------------------------------
#ifndef DIP_ON_LEVEL
#define DIP_ON_LEVEL          LOW    // switch ON pulls the pin to GND
#endif
#ifndef MOTOR_FORWARD_LEVEL
#define MOTOR_FORWARD_LEVEL   LOW    // same as the Maker Mini Sumo driver
#endif
#ifndef MOTOR_PWM_HZ
#define MOTOR_PWM_HZ          20000  // above audible range
#endif
#ifndef VM_DIVIDER_RATIO
#define VM_DIVIDER_RATIO      11.0f
#endif

// ---- Melody pitches (subset of the original list) ---------------------------
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_G6  1568
#define NOTE_C7  2093

class CytronMakerSumoRP2350 {
public:
    // Configures every pin, motor PWM, 10-bit ADC and the USB identity.
    // Call before Serial.begin().
    void begin();

    // Switch inputs
    int  readDipSwitch();        // 0-7, SW1 = MSB, 1 = ON
    bool isStartPressed();       // USR_BTN pressed
    bool irStartActive();        // IRST module says START

    // Sensors
    uint8_t readOpponents();     // bit0 L, bit1 FL, bit2 FC, bit3 FR, bit4 R
    float   readBatteryVoltage();
    bool    motorFault();        // true if either driver reports a fault

    // Motors (speed -255..255)
    void stop();
    void moveForward(int speed);
    void moveBackward(int speed);
    void turnLeft(int speed);
    void turnRight(int speed);
    void setMotorSpeed(int side, int speed);

    // Buzzer (blocking, like the original library)
    void playTone(int pitch, int duration);
    void playMelody(const int *pitch, const int *duration, int length);

    // Re-applies VID/PID/strings. Called by begin(); public for custom use.
    void applyUsbIdentity();

    // Prints "DEVICE=... VID=... PID=... LIB=..." to any Stream (Serial).
    // Handy for a HELLO reply or a boot banner.
    void printIdentity(Stream &out);
};

extern CytronMakerSumoRP2350 MakerSumo;
