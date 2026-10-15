#include "MakerSumoRP2350.h"
#include <EEPROM.h>

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

// ---- Edge sensor calibration (EEPROM 0-11) ----------------------------------
// 0: magic 0xED, 1: version, 2-3 blackL, 4-5 blackR, 6-7 whiteL, 8-9 whiteR, 10: checksum
static const int     EDGE_EEPROM_ADDR    = 0;
static const uint8_t EDGE_EEPROM_MAGIC   = 0xED;
static const uint8_t EDGE_EEPROM_VERSION = 1;
static const int     EDGE_MIN_CONTRAST   = 40;    // black - white must be at least this

static void eepromEnsureBegun() {
    if (EEPROM.length() == 0) EEPROM.begin(512);
}

int CytronMakerSumoRP2350::readEdgeAverage(uint8_t pin, int samples) {
    long sum = 0;
    for (int i = 0; i < samples; i++) { sum += analogRead(pin); delay(2); }
    return (int)(sum / samples);
}

void CytronMakerSumoRP2350::waitForStartPress(bool blinkLed) {
    while (isStartPressed()) delay(10);                       // wait for release
    while (!isStartPressed()) {
        digitalWrite(LED, blinkLed ? ((millis() / 250) % 2) : HIGH);
        delay(10);
    }
    delay(30);                                                // debounce
    while (isStartPressed()) delay(10);
}

bool CytronMakerSumoRP2350::calibrateEdgeSensors() {
    stop();
    playTone(NOTE_C5, 80); delay(40); playTone(NOTE_C5, 80);  // entering calibration

    waitForStartPress(false);                                 // LED solid: robot on BLACK
    int bL = readEdgeAverage(EDGE_L), bR = readEdgeAverage(EDGE_R);
    playTone(NOTE_G5, 120);

    waitForStartPress(true);                                  // LED blinking: sensors on WHITE
    int wL = readEdgeAverage(EDGE_L), wR = readEdgeAverage(EDGE_R);
    playTone(NOTE_G5, 80); delay(40); playTone(NOTE_G5, 80);

    digitalWrite(LED, LOW);
    if (!saveEdgeCalibration(bL, bR, wL, wR)) {
        playTone(NOTE_C4, 400);                               // low tone: not enough contrast
        return false;
    }
    playTone(NOTE_E5, 60); delay(30); playTone(NOTE_G5, 60); delay(30); playTone(NOTE_C6, 120);
    return true;
}

bool CytronMakerSumoRP2350::saveEdgeCalibration(int blackL, int blackR, int whiteL, int whiteR) {
    if (blackL - whiteL < EDGE_MIN_CONTRAST || blackR - whiteR < EDGE_MIN_CONTRAST) return false;
    eepromEnsureBegun();
    uint8_t buf[11] = { EDGE_EEPROM_MAGIC, EDGE_EEPROM_VERSION,
        (uint8_t)(blackL & 0xFF), (uint8_t)(blackL >> 8), (uint8_t)(blackR & 0xFF), (uint8_t)(blackR >> 8),
        (uint8_t)(whiteL & 0xFF), (uint8_t)(whiteL >> 8), (uint8_t)(whiteR & 0xFF), (uint8_t)(whiteR >> 8), 0 };
    uint8_t sum = 0;
    for (int i = 2; i < 10; i++) sum += buf[i];
    buf[10] = sum;
    for (int i = 0; i < 11; i++) {
        if (EEPROM.read(EDGE_EEPROM_ADDR + i) != buf[i]) EEPROM.write(EDGE_EEPROM_ADDR + i, buf[i]);
    }
    EEPROM.commit();
    _blackL = blackL; _blackR = blackR; _whiteL = whiteL; _whiteR = whiteR;
    _edgeCalibrated = true;
    return true;
}

bool CytronMakerSumoRP2350::loadEdgeCalibration() {
    eepromEnsureBegun();
    _edgeCalibrated = false;
    if (EEPROM.read(EDGE_EEPROM_ADDR) != EDGE_EEPROM_MAGIC ||
        EEPROM.read(EDGE_EEPROM_ADDR + 1) != EDGE_EEPROM_VERSION) return false;
    uint8_t b[8], sum = 0;
    for (int i = 0; i < 8; i++) { b[i] = EEPROM.read(EDGE_EEPROM_ADDR + 2 + i); sum += b[i]; }
    if (EEPROM.read(EDGE_EEPROM_ADDR + 10) != sum) return false;
    int bL = b[0] | (b[1] << 8), bR = b[2] | (b[3] << 8), wL = b[4] | (b[5] << 8), wR = b[6] | (b[7] << 8);
    if (bL > 1023 || bR > 1023 || wL > 1023 || wR > 1023 ||
        bL - wL < EDGE_MIN_CONTRAST || bR - wR < EDGE_MIN_CONTRAST) return false;
    _blackL = bL; _blackR = bR; _whiteL = wL; _whiteR = wR;
    _edgeCalibrated = true;
    return true;
}

void CytronMakerSumoRP2350::getEdgeCalibration(int &blackL, int &blackR, int &whiteL, int &whiteR) const {
    blackL = _edgeCalibrated ? _blackL : 0;
    blackR = _edgeCalibrated ? _blackR : 0;
    whiteL = _edgeCalibrated ? _whiteL : 0;
    whiteR = _edgeCalibrated ? _whiteR : 0;
}

void CytronMakerSumoRP2350::updateEdgeThresholds(int sensitivity) {
    sensitivity = constrain(sensitivity, 1, 99);
    if (_edgeCalibrated) {
        _thresholdL = _whiteL + (long)(_blackL - _whiteL) * sensitivity / 100;
        _thresholdR = _whiteR + (long)(_blackR - _whiteR) * sensitivity / 100;
    } else {
        // No calibration: assume we are on black now; white = 0.
        _thresholdL = (long)readEdgeAverage(EDGE_L, 8) * sensitivity / 100;
        _thresholdR = (long)readEdgeAverage(EDGE_R, 8) * sensitivity / 100;
    }
}

int CytronMakerSumoRP2350::edgeThreshold(uint8_t pin) {
    return pin == EDGE_L ? _thresholdL : _thresholdR;
}

bool CytronMakerSumoRP2350::isEdgeDetected(uint8_t pin) {
    return analogRead(pin) < edgeThreshold(pin);
}
