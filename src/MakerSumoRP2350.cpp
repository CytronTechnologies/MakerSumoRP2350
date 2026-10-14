#include "MakerSumoRP2350.h"

#ifdef USE_TINYUSB
  #include <Adafruit_TinyUSB.h>
#else
  #include <USB.h>
#endif

CytronMakerSumoRP2350 MakerSumo;

void CytronMakerSumoRP2350::begin() {
    // Motor driver: direction first, then PWM at 0 so nothing twitches.
    pinMode(DIR_L, OUTPUT);
    pinMode(DIR_R, OUTPUT);
    digitalWrite(DIR_L, MOTOR_FORWARD_LEVEL);
    digitalWrite(DIR_R, MOTOR_FORWARD_LEVEL);
    pinMode(PWM_L, OUTPUT);
    pinMode(PWM_R, OUTPUT);
    analogWriteFreq(MOTOR_PWM_HZ);
    analogWriteRange(255);          // keep the 0-255 speed scale
    analogWrite(PWM_L, 0);
    analogWrite(PWM_R, 0);
    pinMode(FAULT_L, INPUT);
    pinMode(FAULT_R, INPUT);

    // User interface
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
    PinMode dipMode = (DIP_ON_LEVEL == LOW) ? INPUT_PULLUP : INPUT_PULLDOWN;
    pinMode(DIP1, dipMode);
    pinMode(DIP2, dipMode);
    pinMode(DIP3, dipMode);
    pinMode(USR_BTN, INPUT);
    pinMode(IRST, INPUT);  // reads LOW when no module is connected

    // Sensors
    pinMode(OPP_L,  INPUT);
    pinMode(OPP_FL, INPUT);
    pinMode(OPP_FC, INPUT);
    pinMode(OPP_FR, INPUT);
    pinMode(OPP_R,  INPUT);
    analogReadResolution(10);       // 0-1023, same scale as the ATmega firmware

#ifndef ARDUINO_CYTRON_MAKER_SUMO_RP2350
    // No board variant installed (e.g. "Generic RP2350" selected), so set the
    // identity at runtime. With the variant the core already has it.
    applyUsbIdentity();
#endif
}

void CytronMakerSumoRP2350::applyUsbIdentity() {
#ifdef USE_TINYUSB
    if (!TinyUSBDevice.isInitialized()) {
        TinyUSBDevice.begin(0);
    }
    TinyUSBDevice.setID(MAKER_SUMO_USB_VID, MAKER_SUMO_USB_PID);
    TinyUSBDevice.setManufacturerDescriptor(MAKER_SUMO_USB_MANUFACTURER);
    TinyUSBDevice.setProductDescriptor(MAKER_SUMO_USB_PRODUCT);
    if (TinyUSBDevice.mounted()) {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
    }
#else
    USB.disconnect();
    USB.setVIDPID(MAKER_SUMO_USB_VID, MAKER_SUMO_USB_PID);
    USB.setManufacturer(MAKER_SUMO_USB_MANUFACTURER);
    USB.setProduct(MAKER_SUMO_USB_PRODUCT);
    USB.connect();
#endif
}

// ---- Switches ---------------------------------------------------------------
int CytronMakerSumoRP2350::readDipSwitch() {
    int value = 0;
    if (digitalRead(DIP1) == DIP_ON_LEVEL) value |= 4;
    if (digitalRead(DIP2) == DIP_ON_LEVEL) value |= 2;
    if (digitalRead(DIP3) == DIP_ON_LEVEL) value |= 1;
    return value;
}

bool CytronMakerSumoRP2350::isStartPressed() {
    return digitalRead(USR_BTN) == LOW;
}

bool CytronMakerSumoRP2350::irStartActive() {
    return digitalRead(IRST) == HIGH;
}

// ---- Sensors ----------------------------------------------------------------
uint8_t CytronMakerSumoRP2350::readOpponents() {
    const uint8_t pins[] = { OPP_L, OPP_FL, OPP_FC, OPP_FR, OPP_R };
    uint8_t mask = 0;
    for (uint8_t i = 0; i < 5; i++) {
        if (digitalRead(pins[i]) == LOW) mask |= (1 << i);
    }
    return mask;
}

float CytronMakerSumoRP2350::readBatteryVoltage() {
    int adc = analogRead(VM_SENSE);
    float adcVolt = (float)adc * (3.3f / 1023.0f);
    return adcVolt * VM_DIVIDER_RATIO;
}

bool CytronMakerSumoRP2350::motorFault() {
    return digitalRead(FAULT_L) == LOW || digitalRead(FAULT_R) == LOW;
}

// ---- Motors -----------------------------------------------------------------
void CytronMakerSumoRP2350::stop() {
    analogWrite(PWM_L, 0);
    analogWrite(PWM_R, 0);
}

void CytronMakerSumoRP2350::setMotorSpeed(int side, int speed) {
    speed = constrain(speed, -255, 255);
    int direction = MOTOR_FORWARD_LEVEL;
    if (speed < 0) {
        speed = -speed;
        direction = (MOTOR_FORWARD_LEVEL == LOW) ? HIGH : LOW;
    }

    switch (side) {
        case MOTOR_L:
            digitalWrite(DIR_L, direction);
            analogWrite(PWM_L, speed);
            break;
        case MOTOR_R:
            digitalWrite(DIR_R, direction);
            analogWrite(PWM_R, speed);
            break;
    }
}

void CytronMakerSumoRP2350::moveForward(int speed)  { setMotorSpeed(MOTOR_L,  speed); setMotorSpeed(MOTOR_R,  speed); }
void CytronMakerSumoRP2350::moveBackward(int speed) { setMotorSpeed(MOTOR_L, -speed); setMotorSpeed(MOTOR_R, -speed); }
void CytronMakerSumoRP2350::turnLeft(int speed)     { setMotorSpeed(MOTOR_L, -speed); setMotorSpeed(MOTOR_R,  speed); }
void CytronMakerSumoRP2350::turnRight(int speed)    { setMotorSpeed(MOTOR_L,  speed); setMotorSpeed(MOTOR_R, -speed); }

// ---- Buzzer -----------------------------------------------------------------
void CytronMakerSumoRP2350::playTone(int pitch, int duration) {
    if (pitch <= 0) {
        delay(duration);
        return;
    }
    tone(BUZZER, pitch);
    delay(duration);
    noTone(BUZZER);
    pinMode(BUZZER, OUTPUT);        // hand the pin back to plain GPIO
    digitalWrite(BUZZER, LOW);
}

void CytronMakerSumoRP2350::playMelody(const int *pitch, const int *duration, int length) {
    for (int i = 0; i < length; i++) {
        int noteDuration = 1000 / duration[i];
        playTone(pitch[i], noteDuration);
        delay(noteDuration * 3 / 10);
    }
}

// ---- Identity ---------------------------------------------------------------
void CytronMakerSumoRP2350::printIdentity(Stream &out) {
    out.print(F("DEVICE=MAKER_SUMO_RP2350 VID=0x"));
    out.print(MAKER_SUMO_USB_VID, HEX);
    out.print(F(" PID=0x"));
    out.print(MAKER_SUMO_USB_PID, HEX);
    out.print(F(" LIB="));
    out.print(MAKER_SUMO_RP2350_LIB_VERSION);
}
