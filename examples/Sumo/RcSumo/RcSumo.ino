/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example drives the robot from an RC transmitter. DIP switches
 * select live motor alignment modes (LHL = forward, HLH = backward) using
 * the potentiometer; hold START for 2 s to save the trim. Compatible with
 * the Web Serial configurator (protocol 2).
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <EEPROM.h>
#include <MakerSumoRP2350.h>

constexpr uint8_t RC_SPEED    = GPIO1;   // SERVO1
constexpr uint8_t RC_STEERING = GPIO2;   // SERVO2

constexpr uint8_t MODE_FORWARD_ALIGNMENT  = 2;   // LHL
constexpr uint8_t MODE_BACKWARD_ALIGNMENT = 5;   // HLH

constexpr int RC_NEUTRAL_US   = 1500;
constexpr int RC_RANGE_US     = 1000;
constexpr int RC_VALID_MIN_US = 750;
constexpr int RC_VALID_MAX_US = 2250;
constexpr unsigned long RC_TIMEOUT_US = 30000UL;
constexpr float RC_DEADBAND   = 0.1f;

constexpr int SPEED_MAX = 255;

constexpr int POT_LEFT_CENTER  = 460;
constexpr int POT_RIGHT_CENTER = 563;
constexpr int MAX_TRIM_PERCENT = 25;

constexpr int     EEPROM_SIZE                    = 512;
constexpr int     EEPROM_MAGIC_ADDRESS           = 16;
constexpr int     EEPROM_VERSION_ADDRESS         = 17;
constexpr int     EEPROM_FORWARD_TRIM_ADDRESS    = 18;
constexpr int     EEPROM_BACKWARD_TRIM_ADDRESS   = 19;
constexpr int     EEPROM_RC_MAPPING_ADDRESS      = 20;
constexpr int     EEPROM_RC_MAPPING_MAGIC_ADDRESS = 21;
constexpr uint8_t EEPROM_MAGIC            = 0xA7;
constexpr uint8_t EEPROM_VERSION          = 1;
constexpr uint8_t EEPROM_RC_MAPPING_MAGIC = 0x5C;

constexpr unsigned long SAVE_HOLD_MS  = 2000UL;
constexpr unsigned long SAVE_FLASH_MS = 1000UL;

int8_t forwardTrim = 0;
int8_t backwardTrim = 0;
bool rcChannelsSwapped = false;

// ---- RC capture -------------------------------------------------------------
struct RcChannel {
    volatile uint32_t riseAt = 0;
    volatile uint32_t lastPulseAt = 0;
    volatile uint16_t pulseWidth = 0;
};
RcChannel rcSpeed, rcSteering;

static inline void rcEdge(RcChannel &ch, uint8_t pin) {
    uint32_t now = micros();
    if (digitalRead(pin)) {
        ch.riseAt = now;
    } else {
        uint32_t width = now - ch.riseAt;
        if (width <= UINT16_MAX) { ch.pulseWidth = width; ch.lastPulseAt = now; }
    }
}
void rcSpeedIsr()    { rcEdge(rcSpeed, RC_SPEED); }
void rcSteeringIsr() { rcEdge(rcSteering, RC_STEERING); }

void beginRcCapture() {
    pinMode(RC_SPEED, INPUT_PULLUP);
    pinMode(RC_STEERING, INPUT_PULLUP);
    uint32_t now = micros();
    if (digitalRead(RC_SPEED))    rcSpeed.riseAt = now;
    if (digitalRead(RC_STEERING)) rcSteering.riseAt = now;
    attachInterrupt(digitalPinToInterrupt(RC_SPEED), rcSpeedIsr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RC_STEERING), rcSteeringIsr, CHANGE);
}

bool readRcChannel(uint8_t pin, float &value) {
    RcChannel &ch = (pin == RC_SPEED) ? rcSpeed : rcSteering;
    noInterrupts();
    uint16_t width = ch.pulseWidth;
    uint32_t lastAt = ch.lastPulseAt;
    interrupts();

    uint32_t now = micros();
    if (lastAt == 0 || now - lastAt > RC_TIMEOUT_US ||
        width < RC_VALID_MIN_US || width > RC_VALID_MAX_US) return false;

    value = (float)((long)width - RC_NEUTRAL_US) / (float)(RC_RANGE_US / 2);
    value = constrain(value, -1.0f, 1.0f);
    if (value >= -RC_DEADBAND && value <= RC_DEADBAND) value = 0.0f;
    else if (value > 0.0f) value = (value - RC_DEADBAND) / (1.0f - RC_DEADBAND);
    else                   value = (value + RC_DEADBAND) / (1.0f - RC_DEADBAND);
    return true;
}

// ---- Trim -------------------------------------------------------------------
int8_t readPotTrim() {
    int pot = analogRead(POT);
    if (pot < POT_LEFT_CENTER)  return (int8_t)map(pot, 0, POT_LEFT_CENTER, -MAX_TRIM_PERCENT, 0);
    if (pot > POT_RIGHT_CENTER) return (int8_t)map(pot, POT_RIGHT_CENTER, 1023, 0, MAX_TRIM_PERCENT);
    return 0;
}

void applyTrim(int &leftSpeed, int &rightSpeed, int8_t trim) {
    int scalePercent = 100 - abs(trim);
    if (trim < 0)      leftSpeed  = leftSpeed  * scalePercent / 100;
    else if (trim > 0) rightSpeed = rightSpeed * scalePercent / 100;
}

// ---- EEPROM -----------------------------------------------------------------
void eepromUpdate(int address, uint8_t value) {
    if (EEPROM.read(address) != value) EEPROM.write(address, value);
}

void loadAlignment() {
    bool valid = EEPROM.read(EEPROM_MAGIC_ADDRESS) == EEPROM_MAGIC &&
                 EEPROM.read(EEPROM_VERSION_ADDRESS) == EEPROM_VERSION;
    if (!valid) { forwardTrim = backwardTrim = 0; return; }
    forwardTrim  = (int8_t)EEPROM.read(EEPROM_FORWARD_TRIM_ADDRESS);
    backwardTrim = (int8_t)EEPROM.read(EEPROM_BACKWARD_TRIM_ADDRESS);
    if (abs(forwardTrim)  > MAX_TRIM_PERCENT) forwardTrim = 0;
    if (abs(backwardTrim) > MAX_TRIM_PERCENT) backwardTrim = 0;
}

void playSaveSound() {
    MakerSumo.playTone(NOTE_E5, 60); delay(30);
    MakerSumo.playTone(NOTE_G5, 60); delay(30);
    MakerSumo.playTone(NOTE_C6, 120);
}

void saveTrim(char direction, int8_t trim) {
    if (direction == 'F')      forwardTrim = trim;
    else if (direction == 'B') backwardTrim = trim;
    else return;

    eepromUpdate(EEPROM_FORWARD_TRIM_ADDRESS, (uint8_t)forwardTrim);
    eepromUpdate(EEPROM_BACKWARD_TRIM_ADDRESS, (uint8_t)backwardTrim);
    eepromUpdate(EEPROM_VERSION_ADDRESS, EEPROM_VERSION);
    eepromUpdate(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC);
    EEPROM.commit();
    playSaveSound();
}

// ---- Mode feedback ----------------------------------------------------------
uint8_t previousMode = 0xFF;
bool startWasPressed = false, saveCompletedForThisPress = false;
unsigned long startPressedAt = 0, saveFlashStartedAt = 0;

void updateModeSound(uint8_t mode) {
    if (mode == previousMode) return;
    previousMode = mode;
    if (mode == MODE_FORWARD_ALIGNMENT) {
        MakerSumo.stop();
        MakerSumo.playTone(NOTE_G5, 120);                  // one beep
    } else if (mode == MODE_BACKWARD_ALIGNMENT) {
        MakerSumo.stop();
        MakerSumo.playTone(NOTE_C5, 90); delay(70);        // two beeps
        MakerSumo.playTone(NOTE_C5, 90);
    }
}

bool handleSaveButton(uint8_t mode, int8_t liveTrim) {
    bool alignmentMode = mode == MODE_FORWARD_ALIGNMENT || mode == MODE_BACKWARD_ALIGNMENT;
    bool pressed = MakerSumo.isStartPressed();
    unsigned long now = millis();

    if (!alignmentMode) { startWasPressed = saveCompletedForThisPress = false; return false; }

    if (pressed) {
        if (!startWasPressed) { startWasPressed = true; saveCompletedForThisPress = false; startPressedAt = now; }
        if (!saveCompletedForThisPress && now - startPressedAt >= SAVE_HOLD_MS) {
            saveTrim(mode == MODE_FORWARD_ALIGNMENT ? 'F' : 'B', liveTrim);
            saveCompletedForThisPress = true;
            saveFlashStartedAt = now;
        }
        return true;
    }
    startWasPressed = saveCompletedForThisPress = false;
    return saveFlashStartedAt != 0 && now - saveFlashStartedAt < SAVE_FLASH_MS;
}

void updateLed(uint8_t mode, bool motorsMoving) {
    unsigned long now = millis();
    if (saveFlashStartedAt != 0 && now - saveFlashStartedAt < SAVE_FLASH_MS) {
        digitalWrite(LED, ((now - saveFlashStartedAt) / 80) % 2);   // fast flash after save
        return;
    }
    saveFlashStartedAt = 0;

    unsigned long phase = now % 1200UL;
    if (mode == MODE_FORWARD_ALIGNMENT)       digitalWrite(LED, phase < 150UL);
    else if (mode == MODE_BACKWARD_ALIGNMENT) digitalWrite(LED, phase < 150UL || (phase >= 300UL && phase < 450UL));
    else                                      digitalWrite(LED, motorsMoving);
}

// ---- Serial protocol 2 ------------------------------------------------------
constexpr uint8_t SERIAL_BUFFER_SIZE = 40;
char serialBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialBufferLength = 0;

void handleSerialCommand(char *command) {
    if (strcmp(command, "HELLO") == 0) {
        Serial.println(F("OK DEVICE=MAKER_SUMO_RP2350 FW=RC VERSION=1.0.0 PROTOCOL=2 BOARD=RP2350 LIB=" MAKER_SUMO_RP2350_LIB_VERSION));
        return;
    }
    if (strcmp(command, "GET SENSOR") == 0) {
        Serial.print(F("SENSOR ")); Serial.print(MakerSumo.readOpponents());
        Serial.print(' '); Serial.print(analogRead(EDGE_L));
        Serial.print(' '); Serial.print(analogRead(EDGE_R));
        Serial.print(' '); Serial.print(digitalRead(USR_BTN));
        Serial.print(' '); Serial.print(MakerSumo.readDipSwitch());
        Serial.print(F(" 98 ")); Serial.println(MakerSumo.readBatteryVoltage(), 2);
        return;
    }
    if (strcmp(command, "GET CONFIG") == 0) {
        Serial.print(F("CONFIG F=")); Serial.print(forwardTrim);
        Serial.print(F(" B="));       Serial.println(backwardTrim);
        return;
    }
    if (strcmp(command, "GET RC") == 0) {
        Serial.print(F("RC MAP=")); Serial.println(rcChannelsSwapped ? 1 : 0);
        return;
    }
    if (strcmp(command, "SAVE RC 0") == 0 || strcmp(command, "SAVE RC 1") == 0) {
        MakerSumo.stop();
        rcChannelsSwapped = command[8] == '1';
        eepromUpdate(EEPROM_RC_MAPPING_ADDRESS, rcChannelsSwapped ? 1 : 0);
        eepromUpdate(EEPROM_RC_MAPPING_MAGIC_ADDRESS, EEPROM_RC_MAPPING_MAGIC);
        EEPROM.commit();
        playSaveSound();
        Serial.print(F("OK SAVED RC=")); Serial.println(rcChannelsSwapped ? 1 : 0);
        return;
    }
    if (strncmp(command, "SAVE ", 5) == 0 && (command[5] == 'F' || command[5] == 'B') && command[6] == ' ') {
        char *valueText = command + 7, *endText;
        long value = strtol(valueText, &endText, 10);
        if (*valueText == '\0' || *endText != '\0' || value < -MAX_TRIM_PERCENT || value > MAX_TRIM_PERCENT) {
            Serial.println(F("ERROR INVALID_VALUE")); return;
        }
        MakerSumo.stop();
        saveTrim(command[5], (int8_t)value);
        Serial.print(F("OK SAVED ")); Serial.print(command[5]); Serial.print('='); Serial.println(value);
        return;
    }
    Serial.println(F("ERROR UNKNOWN_COMMAND"));
}

void processSerial() {
    while (Serial.available() > 0) {
        char incoming = (char)Serial.read();
        if (incoming == '\r') continue;
        if (incoming == '\n') {
            serialBuffer[serialBufferLength] = '\0';
            if (serialBufferLength > 0) handleSerialCommand(serialBuffer);
            serialBufferLength = 0;
            continue;
        }
        if (serialBufferLength < SERIAL_BUFFER_SIZE - 1) serialBuffer[serialBufferLength++] = incoming;
        else { serialBufferLength = 0; Serial.println(F("ERROR COMMAND_TOO_LONG")); }
    }
}

void setup() {
    MakerSumo.begin();
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    beginRcCapture();
    loadAlignment();
    if (EEPROM.read(EEPROM_RC_MAPPING_MAGIC_ADDRESS) == EEPROM_RC_MAPPING_MAGIC) {
        rcChannelsSwapped = EEPROM.read(EEPROM_RC_MAPPING_ADDRESS) == 1;
    }
    MakerSumo.stop();
    digitalWrite(LED, LOW);
    MakerSumo.playTone(NOTE_C5, 80); delay(40);
    MakerSumo.playTone(NOTE_G5, 110);
}

void loop() {
    processSerial();

    uint8_t mode = MakerSumo.readDipSwitch();
    updateModeSound(mode);
    int8_t liveTrim = readPotTrim();

    if (handleSaveButton(mode, liveTrim)) { MakerSumo.stop(); updateLed(mode, false); return; }

    float speedPercent, steeringPercent;
    uint8_t speedPin    = rcChannelsSwapped ? RC_STEERING : RC_SPEED;
    uint8_t steeringPin = rcChannelsSwapped ? RC_SPEED : RC_STEERING;
    if (!readRcChannel(speedPin, speedPercent) || !readRcChannel(steeringPin, steeringPercent)) {
        MakerSumo.stop(); updateLed(mode, false); return;   // failsafe
    }

    int leftSpeed  = constrain((int)((speedPercent + steeringPercent) * SPEED_MAX), -SPEED_MAX, SPEED_MAX);
    int rightSpeed = constrain((int)((speedPercent - steeringPercent) * SPEED_MAX), -SPEED_MAX, SPEED_MAX);

    if (speedPercent > 0.0f)      applyTrim(leftSpeed, rightSpeed, mode == MODE_FORWARD_ALIGNMENT  ? liveTrim : forwardTrim);
    else if (speedPercent < 0.0f) applyTrim(leftSpeed, rightSpeed, mode == MODE_BACKWARD_ALIGNMENT ? liveTrim : backwardTrim);

    MakerSumo.setMotorSpeed(MOTOR_L, leftSpeed);
    MakerSumo.setMotorSpeed(MOTOR_R, rightSpeed);
    updateLed(mode, leftSpeed != 0 || rightSpeed != 0);
}
