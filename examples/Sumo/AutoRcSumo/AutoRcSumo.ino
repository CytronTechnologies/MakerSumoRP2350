/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example is the full sumo firmware: seven DIP-selectable opening
 * strategies, a defense routine on DIP 5, RC driving on DIP 7, edge
 * escape, and settings stored in flash that can be changed from the Web
 * Serial configurator (protocol 6).
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

#include <EEPROM.h>
#include <MakerSumoRP2350.h>
#include <stdlib.h>
#include <string.h>

// ---- RC receiver ------------------------------------------------------------
constexpr uint8_t RC_SPEED    = GPIO1;   // SERVO1
constexpr uint8_t RC_STEERING = GPIO2;   // SERVO2

constexpr int RC_NEUTRAL_US   = 1500;
constexpr int RC_RANGE_US     = 1000;
constexpr int RC_VALID_MIN_US = 750;
constexpr int RC_VALID_MAX_US = 2250;
constexpr unsigned long RC_TIMEOUT_US = 30000UL;
constexpr float RC_DEADBAND   = 0.1f;

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

uint16_t readRcPulse(uint8_t pin) {
    RcChannel &ch = (pin == RC_SPEED) ? rcSpeed : rcSteering;
    noInterrupts();
    uint16_t width = ch.pulseWidth;
    uint32_t lastAt = ch.lastPulseAt;
    interrupts();
    uint32_t now = micros();
    if (lastAt == 0 || now - lastAt > RC_TIMEOUT_US ||
        width < RC_VALID_MIN_US || width > RC_VALID_MAX_US) return 0;
    return width;
}

bool readRcChannel(uint8_t pin, float &value) {
    uint16_t width = readRcPulse(pin);
    if (!width) return false;
    value = (float)((long)width - RC_NEUTRAL_US) / (float)(RC_RANGE_US / 2);
    value = constrain(value, -1.0f, 1.0f);
    if (value >= -RC_DEADBAND && value <= RC_DEADBAND) value = 0.0f;
    else if (value > 0.0f) value = (value - RC_DEADBAND) / (1.0f - RC_DEADBAND);
    else                   value = (value + RC_DEADBAND) / (1.0f - RC_DEADBAND);
    return true;
}

// ---- Settings ---------------------------------------------------------------
enum AutoField { SEARCH_L, SEARCH_R, BACK_SPEED, BACK_MS, TURN_SPEED, TURN_MS,
                 ATTACK_INITIAL, ATTACK_MS, ATTACK_MAX, AUTO_FIELDS };
enum RunState  { ARMING, WAIT_START, COUNTDOWN, OPENING, DEF_WAIT, DEF_MOVE,
                 FIGHT, BACK_REVERSE, BACK_TURN, BACK_PAUSE, STOPPED };
void enterState(RunState next);   

struct Settings {
    int16_t behaviour[AUTO_FIELDS];
    int16_t defense[5];      
    int16_t steps[7][20];    
};
static_assert(sizeof(Settings) == 308, "EEPROM schema size changed; bump its version");
Settings config;

constexpr int     EEPROM_SIZE            = 512;
constexpr int     CONFIG_ADDRESS         = 32;
constexpr uint8_t CONFIG_VERSION         = 2;
constexpr int     EEPROM_TRIM_MAGIC_ADDR = 16;   
constexpr int     EEPROM_TRIM_VER_ADDR   = 17;  
constexpr int     EEPROM_FWD_TRIM_ADDR   = 18;
constexpr int     EEPROM_BWD_TRIM_ADDR   = 19;
constexpr int     EEPROM_RC_MAP_ADDR     = 20;
constexpr int     EEPROM_RC_MAP_MAGIC_ADDR = 21;
constexpr uint8_t EEPROM_RC_MAP_MAGIC    = 0x5C;

constexpr uint8_t  EDGE_SENSITIVITY_MIN_PERCENT = 25;
constexpr uint8_t  EDGE_SENSITIVITY_MAX_PERCENT = 75;
constexpr uint16_t BACKOFF_PAUSE_MS   = 50;
constexpr uint32_t BUTTON_COUNTDOWN_MS = 5000;
constexpr uint16_t INPUT_SETTLE_MS    = 1000;
constexpr uint8_t  START_DEBOUNCE_MS  = 25;
constexpr uint16_t MAX_DURATION_MS    = 10000;

RunState state = ARMING;
uint32_t stateAt = 0, inputAt = 0, attackAt = 0;
bool inputArmed = false, startedByIr = false;
bool configSession = false, attacking = false;
bool rcChannelsSwapped = false;
uint8_t selectedMode = 0, stepIndex = 0, defenseCount = 0;
bool turnRight = true;
int8_t forwardTrim = 0, backwardTrim = 0;
char serialLine[192];
uint8_t serialLength = 0;
bool serialOverflow = false;

// ---- Non-blocking buzzer ----------------------------------------------------
const uint16_t POWER_NOTES[]     = { NOTE_C5, 0, NOTE_G5 };
const uint16_t POWER_DURATIONS[] = { 80, 40, 110 };
const uint16_t SAVE_NOTES[]      = { NOTE_E5, 0, NOTE_G5, 0, NOTE_C6 };
const uint16_t SAVE_DURATIONS[]  = { 60, 30, 60, 30, 120 };
const uint16_t *buzzerNotes = nullptr, *buzzerDurations = nullptr;
uint8_t  buzzerStep = 0, buzzerLength = 0;
uint32_t buzzerStepAt = 0;
bool buzzerSounding = false;

void startBuzzer(bool saved) {
    buzzerNotes     = saved ? SAVE_NOTES : POWER_NOTES;
    buzzerDurations = saved ? SAVE_DURATIONS : POWER_DURATIONS;
    buzzerLength    = saved ? 5 : 3;
    buzzerStep = 0;
    buzzerStepAt = millis();
    buzzerSounding = false;
}

void updateBuzzer() {
    if (!buzzerLength) return;
    uint32_t now = millis();
    if (!buzzerSounding) {
        if (buzzerNotes[buzzerStep]) tone(BUZZER, buzzerNotes[buzzerStep]);
        buzzerSounding = true;
    }
    if (now - buzzerStepAt >= buzzerDurations[buzzerStep]) {
        noTone(BUZZER);
        buzzerSounding = false;
        buzzerStepAt = now;
        if (++buzzerStep >= buzzerLength) {
            buzzerLength = 0;
            pinMode(BUZZER, OUTPUT);
            digitalWrite(BUZZER, LOW);
        }
    }
}

// ---- EEPROM helpers ---------------------------------------------------------
void eepromUpdate(int address, uint8_t value) {
    if (EEPROM.read(address) != value) EEPROM.write(address, value);
}

// ---- Motion -----------------------------------------------------------------
void enterState(RunState next) { state = next; stateAt = millis(); }
void stopRobot() { MakerSumo.stop(); digitalWrite(LED, LOW); }

void drive(int left, int right) {
    int8_t lt = left  < 0 ? backwardTrim : forwardTrim;
    int8_t rt = right < 0 ? backwardTrim : forwardTrim;
    if (lt < 0) left  = (long)left  * (100 + lt) / 100;
    if (rt > 0) right = (long)right * (100 - rt) / 100;
    MakerSumo.setMotorSpeed(MOTOR_L, left);
    MakerSumo.setMotorSpeed(MOTOR_R, right);
    digitalWrite(LED, left || right);
}
void drivePercent(int left, int right) { drive(left * 255L / 100, right * 255L / 100); }

uint8_t opponents() { return MakerSumo.readOpponents(); }

// ---- Validation -------------------------------------------------------------
bool validValues(const int16_t *v, uint8_t kind) {
    if (kind == 0) {
        for (uint8_t i = 0; i < AUTO_FIELDS; i++) {
            int lo = i < 2 ? -100 : 0;
            int hi = (i == ATTACK_MS || i == BACK_MS || i == TURN_MS) ? MAX_DURATION_MS : 100;
            if (v[i] < lo || v[i] > hi) return false;
        }
        return true;
    }
    if (kind == 1) {
        return v[0] >= 0 && v[0] <= 100 && v[1] >= 1 && v[1] <= MAX_DURATION_MS &&
               v[2] >= 0 && v[2] <= 100 && v[3] >= 0 && v[3] <= 100 &&
               v[4] >= 1 && v[4] <= MAX_DURATION_MS;
    }
    for (uint8_t i = 0; i < 20; i += 4) {
        if (v[i] < 0 || v[i] > 1 || abs(v[i + 1]) > 100 || abs(v[i + 2]) > 100 ||
            v[i + 3] < 1 || v[i + 3] > MAX_DURATION_MS) return false;
    }
    return true;
}

uint16_t checksum() {
    uint16_t crc = 0xFFFF;
    const uint8_t *bytes = (const uint8_t *)&config;
    for (uint16_t i = 0; i < sizeof(config); i++) {
        crc ^= bytes[i];
        for (uint8_t b = 0; b < 8; b++) crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
    return crc;
}

void defaults() {
    memset(&config, 0, sizeof(config));
    const int16_t behaviour[] = { 35, 35, 100, 100, 100, 120, 50, 300, 100 };
    const int16_t defense[]   = { 3, 2000, 50, 50, 50 };
    memcpy(config.behaviour, behaviour, sizeof(behaviour));
    memcpy(config.defense, defense, sizeof(defense));
    for (uint8_t s = 0; s < 7; s++)
        for (uint8_t r = 0; r < 5; r++) config.steps[s][r * 4 + 3] = 100;
    const int16_t opening[6][12] = {
        { 0, 0, 0, 100, 0, 0, 0, 100, 0, 0, 0, 100 },
        { 1, 100, -100, 80, 1, 60, 100, 600, 1, -100, 100, 120 },
        { 1, 30, 30, 100, 1, 60, 60, 100, 1, 100, 100, 200 },
        { 1, 100, -100, 40, 1, 70, 100, 350, 0, 0, 0, 100 },
        { 1, -100, 100, 80, 1, 100, 60, 600, 1, 100, -100, 120 },
        { 1, -100, 100, 40, 1, 100, 70, 350, 0, 0, 0, 100 }
    };
    for (uint8_t s = 0; s < 6; s++)
        memcpy(config.steps[s == 5 ? 6 : s], opening[s], sizeof(opening[s]));
}

void saveSettings() {
    eepromUpdate(CONFIG_ADDRESS, 0);                
    EEPROM.put(CONFIG_ADDRESS + 4, config);
    uint16_t crc = checksum();
    EEPROM.put(CONFIG_ADDRESS + 2, crc);
    eepromUpdate(CONFIG_ADDRESS + 1, CONFIG_VERSION);
    eepromUpdate(CONFIG_ADDRESS, 0xAC);
    EEPROM.commit();                                 
    startBuzzer(true);
}

void loadSettings() {
    defaults();
    if (EEPROM.read(CONFIG_ADDRESS) == 0xAC && EEPROM.read(CONFIG_ADDRESS + 1) == CONFIG_VERSION) {
        EEPROM.get(CONFIG_ADDRESS + 4, config);
        uint16_t crc;
        EEPROM.get(CONFIG_ADDRESS + 2, crc);
        bool valid = crc == checksum() && validValues(config.behaviour, 0) && validValues(config.defense, 1);
        for (uint8_t s = 0; s < 7; s++) valid &= validValues(config.steps[s], 2);
        if (!valid) defaults();
    }
    if (EEPROM.read(EEPROM_TRIM_MAGIC_ADDR) == 0xA7 && EEPROM.read(EEPROM_TRIM_VER_ADDR) == 1) {
        forwardTrim  = (int8_t)EEPROM.read(EEPROM_FWD_TRIM_ADDR);
        backwardTrim = (int8_t)EEPROM.read(EEPROM_BWD_TRIM_ADDR);
        if (abs(forwardTrim) > 25)  forwardTrim = 0;
        if (abs(backwardTrim) > 25) backwardTrim = 0;
    }
    if (EEPROM.read(EEPROM_RC_MAP_MAGIC_ADDR) == EEPROM_RC_MAP_MAGIC) {
        rcChannelsSwapped = EEPROM.read(EEPROM_RC_MAP_ADDR) == 1;
    }
}

// ---- Match flow -------------------------------------------------------------
void startOpening() {
    // Threshold between the calibrated white and black values; POT moves it.
    // Without a stored calibration the surface under the robot is taken as black.
    int edgeSensitivity = map(analogRead(POT), 0, 1023,
                              EDGE_SENSITIVITY_MIN_PERCENT, EDGE_SENSITIVITY_MAX_PERCENT);
    MakerSumo.updateEdgeThresholds(edgeSensitivity);
    stepIndex = defenseCount = 0;
    enterState(selectedMode == 5 ? DEF_WAIT : OPENING);
}

void runRc() {
    float speed, steering;
    uint8_t speedPin    = rcChannelsSwapped ? RC_STEERING : RC_SPEED;
    uint8_t steeringPin = rcChannelsSwapped ? RC_SPEED : RC_STEERING;
    if (!readRcChannel(speedPin, speed) || !readRcChannel(steeringPin, steering)) { stopRobot(); return; }
    int left  = constrain((int)((speed + steering) * 255), -255, 255);
    int right = constrain((int)((speed - steering) * 255), -255, 255);
    int8_t trim = speed > 0 ? forwardTrim : speed < 0 ? backwardTrim : 0;
    if (trim < 0) left  = left  * (100 + trim) / 100;
    if (trim > 0) right = right * (100 - trim) / 100;
    MakerSumo.setMotorSpeed(MOTOR_L, left);
    MakerSumo.setMotorSpeed(MOTOR_R, right);
    digitalWrite(LED, left || right);
}

// The DIP switches are followed live while the robot is idle (waiting for
// START, stopped, or in RC mode). Once a match is running the mode is frozen
// until it ends, so a nudged switch cannot change the strategy mid-fight.
void followDipSwitch() {
    uint8_t dip = MakerSumo.readDipSwitch();
    if (dip == selectedMode) return;
    bool idle = selectedMode == 7 || state == ARMING || state == WAIT_START || state == STOPPED;
    if (!idle) return;
    selectedMode = dip;
    stopRobot();
    if (selectedMode != 7) {          // back to auto: arm again from the start
        inputAt = millis();
        inputArmed = false;
        startedByIr = false;
        enterState(ARMING);
    }
}

// Leave configuration mode: motors unlocked, back to the arming wait so the
// next START (or IR) runs a match. Called by "CONFIG OFF" and automatically
// when the host closes the serial port (cable pulled, browser disconnected).
void exitConfigSession() {
    configSession = false;
    stopRobot();
    selectedMode = MakerSumo.readDipSwitch();
    inputAt = millis();
    inputArmed = false;
    startedByIr = false;
    enterState(ARMING);
}

void runRobot() {
    uint32_t now = millis();
    if (configSession) {
        if (!Serial) { exitConfigSession(); }   // port closed by the host
        else { stopRobot(); return; }
    }
    followDipSwitch();
    if (selectedMode == 7) { runRc(); return; }

    bool btn = MakerSumo.isStartPressed();
    bool ir  = MakerSumo.irStartActive();

    if (state == ARMING) {
        stopRobot();
        if (btn || ir) inputAt = now;
        if (now - inputAt >= INPUT_SETTLE_MS) { inputArmed = false; enterState(WAIT_START); }
        return;
    }
    if (state == STOPPED) { stopRobot(); return; }
    if (state == WAIT_START) {
        stopRobot();
        if (!btn && !ir) { inputArmed = true; inputAt = now; }
        else if (inputArmed) {
            if (ir) { startedByIr = true; startOpening(); }
            else if (now - inputAt >= START_DEBOUNCE_MS) { startedByIr = false; enterState(COUNTDOWN); }
        }
        return;
    }
    if (startedByIr && !ir) { enterState(STOPPED); stopRobot(); return; }
    if (state == COUNTDOWN) {
        stopRobot();
        digitalWrite(LED, (now - stateAt) % 500 < 250);
        if (now - stateAt >= BUTTON_COUNTDOWN_MS) startOpening();
        return;
    }

    int16_t *a = config.behaviour;
    bool edgeL = MakerSumo.isEdgeDetected(EDGE_L);
    bool edgeR = MakerSumo.isEdgeDetected(EDGE_R);
    if (state != BACK_REVERSE && state != BACK_TURN && state != BACK_PAUSE && (edgeL || edgeR)) {
        turnRight = edgeL; attacking = false; enterState(BACK_REVERSE); stateAt = now;
    }
    uint8_t opp = opponents();
    switch (state) {
        case BACK_REVERSE:
            drivePercent(-a[BACK_SPEED], -a[BACK_SPEED]);
            if (now - stateAt >= (uint16_t)a[BACK_MS]) enterState(BACK_TURN);
            break;
        case BACK_TURN:
            drivePercent(turnRight ? a[TURN_SPEED] : -a[TURN_SPEED],
                         turnRight ? -a[TURN_SPEED] : a[TURN_SPEED]);
            if (now - stateAt >= (uint16_t)a[TURN_MS]) enterState(BACK_PAUSE);
            break;
        case BACK_PAUSE:
            stopRobot();
            if (now - stateAt >= BACKOFF_PAUSE_MS) enterState(FIGHT);
            break;
        case OPENING: {
            int16_t *rows = config.steps[selectedMode];
            while (stepIndex < 5 && !rows[stepIndex * 4]) stepIndex++;
            if (stepIndex == 5) { stopRobot(); enterState(FIGHT); break; }
            int16_t *row = rows + stepIndex * 4;
            drivePercent(row[1], row[2]);
            if (now - stateAt >= (uint16_t)row[3]) { stepIndex++; stateAt = now; }
            break;
        }
        case DEF_WAIT:
        case DEF_MOVE:
            if (opp || defenseCount >= config.defense[0]) { stopRobot(); enterState(FIGHT); break; }
            if (state == DEF_WAIT) {
                stopRobot();
                if (now - stateAt >= (uint16_t)config.defense[1]) enterState(DEF_MOVE);
            } else {
                drivePercent(config.defense[2], config.defense[3]);
                if (now - stateAt >= (uint16_t)config.defense[4]) { stopRobot(); defenseCount++; enterState(DEF_WAIT); }
            }
            break;
        case FIGHT:
            if (!(opp & 4)) attacking = false;
            if (!opp) drivePercent(a[SEARCH_L], a[SEARCH_R]);
            else if (opp & 4) {
                if (!attacking) { attacking = true; attackAt = now; }
                int speed = now - attackAt >= (uint16_t)a[ATTACK_MS] ? a[ATTACK_MAX] : a[ATTACK_INITIAL];
                drivePercent(speed, speed);
            }
            else if (opp & 2) drivePercent(0, a[ATTACK_MAX]);
            else if (opp & 8) drivePercent(a[ATTACK_MAX], 0);
            else if (opp & 1) drivePercent(-a[ATTACK_MAX], a[ATTACK_MAX]);
            else              drivePercent(a[ATTACK_MAX], -a[ATTACK_MAX]);
            break;
        default:
            stopRobot();
            break;
    }
}

// ---- Serial -----------------------------------------------------------------
void printValues(const int16_t *values, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) { Serial.print(' '); Serial.print(values[i]); }
    Serial.println();
}

bool parseValues(int16_t *values, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        char *token = strtok(NULL, " ");
        if (!token) return false;
        char *end;
        long n = strtol(token, &end, 10);
        if (*end || n < -32767 || n > 32767) return false;
        values[i] = n;
    }
    return strtok(NULL, " ") == NULL;
}

void handleCommand() {
    if (!strcmp(serialLine, "HELLO")) {
        Serial.println(F("OK DEVICE=MAKER_SUMO_RP2350 FW=AutoRC VERSION=1.0.0 PROTOCOL=6 BOARD=RP2350 LIB=" MAKER_SUMO_RP2350_LIB_VERSION));
        return;
    }
    if (!strcmp(serialLine, "CONFIG ON")) {
        configSession = true; stopRobot(); Serial.println(F("OK CONFIG")); return;
    }
    if (!strcmp(serialLine, "CONFIG OFF")) {
        if (configSession) exitConfigSession();
        Serial.println(F("OK CONFIG OFF")); return;
    }
    if (!strcmp(serialLine, "GET SENSOR")) {
        Serial.print(F("SENSOR ")); Serial.print(opponents());
        Serial.print(' '); Serial.print(analogRead(EDGE_L));
        Serial.print(' '); Serial.print(analogRead(EDGE_R));
        Serial.print(' '); Serial.print(analogRead(POT));
        Serial.print(' '); Serial.print(readRcPulse(RC_SPEED));
        Serial.print(' '); Serial.print(readRcPulse(RC_STEERING));
        Serial.print(' '); Serial.print(digitalRead(USR_BTN));
        Serial.print(' '); Serial.print(MakerSumo.readDipSwitch());
        Serial.print(' '); Serial.print(configSession ? 99 : (int)state);
        Serial.print(' '); Serial.println(MakerSumo.readBatteryVoltage(), 2);
        return;
    }
    if (!strcmp(serialLine, "GET CONFIG")) {
        Serial.print(F("CONFIG F=")); Serial.print(forwardTrim);
        Serial.print(F(" B="));       Serial.println(backwardTrim);
        return;
    }
    if (!strcmp(serialLine, "GET RC")) {
        Serial.print(F("RC MAP=")); Serial.println(rcChannelsSwapped ? 1 : 0); return;
    }
    if (!strcmp(serialLine, "GET EDGE")) {
        int bL, bR, wL, wR;
        MakerSumo.getEdgeCalibration(bL, bR, wL, wR);
        Serial.print(F("EDGE ")); Serial.print(bL); Serial.print(' '); Serial.print(bR);
        Serial.print(' '); Serial.print(wL); Serial.print(' '); Serial.println(wR);
        return;
    }
    if (!strcmp(serialLine, "GET AUTO")) { Serial.print(F("AUTO")); printValues(config.behaviour, AUTO_FIELDS); return; }
    if (!strcmp(serialLine, "GET DEF"))  { Serial.print(F("DEF"));  printValues(config.defense, 5); return; }

    char *action = strtok(serialLine, " ");
    char *group  = strtok(NULL, " ");
    if (!action || !group) { Serial.println(F("ERROR COMMAND")); return; }
    bool save = !strcmp(action, "SAVE");
    if (save && !configSession) { Serial.println(F("ERROR CONFIG_REQUIRED")); return; }

    int16_t values[20];
    if (save && !strcmp(group, "RC")) {
        if (!parseValues(values, 1) || (values[0] != 0 && values[0] != 1)) { Serial.println(F("ERROR VALUE")); return; }
        rcChannelsSwapped = values[0] == 1;
        eepromUpdate(EEPROM_RC_MAP_ADDR, rcChannelsSwapped ? 1 : 0);
        eepromUpdate(EEPROM_RC_MAP_MAGIC_ADDR, EEPROM_RC_MAP_MAGIC);
        EEPROM.commit();
        startBuzzer(true);
        Serial.print(F("OK SAVED RC=")); Serial.println(rcChannelsSwapped ? 1 : 0);
        return;
    }
    if (save && !strcmp(group, "EDGE")) {
        // SAVE EDGE blackL blackR whiteL whiteR  (readings 0-1023, black > white)
        if (!parseValues(values, 4)) { Serial.println(F("ERROR VALUE")); return; }
        for (uint8_t i = 0; i < 4; i++) if (values[i] < 0 || values[i] > 1023) { Serial.println(F("ERROR VALUE")); return; }
        if (!MakerSumo.saveEdgeCalibration(values[0], values[1], values[2], values[3])) { Serial.println(F("ERROR VALUE")); return; }
        startBuzzer(true);
        Serial.println(F("OK SAVED EDGE"));
        return;
    }
    if (save && (!strcmp(group, "F") || !strcmp(group, "B"))) {
        char direction = group[0];
        if (!parseValues(values, 1) || abs(values[0]) > 25) { Serial.println(F("ERROR VALUE")); return; }
        if (direction == 'F') forwardTrim = values[0]; else backwardTrim = values[0];
        eepromUpdate(EEPROM_FWD_TRIM_ADDR, (uint8_t)forwardTrim);
        eepromUpdate(EEPROM_BWD_TRIM_ADDR, (uint8_t)backwardTrim);
        eepromUpdate(EEPROM_TRIM_VER_ADDR, 1);
        eepromUpdate(EEPROM_TRIM_MAGIC_ADDR, 0xA7);
        EEPROM.commit();
        startBuzzer(true);
        Serial.print(F("OK SAVED ")); Serial.print(direction); Serial.print('='); Serial.println(values[0]);
        return;
    }

    int16_t *target = NULL;
    uint8_t count = 0, kind = 0;
    int strategy = -1;
    if (!strcmp(group, "AUTO")) { target = config.behaviour; count = AUTO_FIELDS; }
    if (!strcmp(group, "DEF"))  { target = config.defense;   count = 5; kind = 1; }
    if (!strcmp(group, "STR")) {
        char *token = strtok(NULL, " ");
        if (!token || strlen(token) != 1 || token[0] < '0' || token[0] > '6' || token[0] == '5') {
            Serial.println(F("ERROR STRATEGY")); return;
        }
        strategy = token[0] - '0'; target = config.steps[strategy]; count = 20; kind = 2;
    }
    if (!target) { Serial.println(F("ERROR COMMAND")); return; }
    if (!strcmp(action, "GET") && strategy >= 0 && !strtok(NULL, " ")) {
        Serial.print(F("STR ")); Serial.print(strategy); printValues(target, count); return;
    }
    if (!save || !parseValues(values, count) || !validValues(values, kind)) { Serial.println(F("ERROR VALUE")); return; }
    memcpy(target, values, count * sizeof(int16_t));
    saveSettings();
    Serial.print(F("OK SAVED ")); Serial.print(group);
    if (strategy >= 0) { Serial.print(' '); Serial.print(strategy); }
    Serial.println();
}

void processSerial() {
    for (uint8_t budget = 0; budget < 24 && Serial.available(); budget++) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            serialLine[serialLength] = 0;
            if (serialOverflow) Serial.println(F("ERROR TOO_LONG"));
            else if (serialLength) handleCommand();
            serialLength = 0; serialOverflow = false;
            return;
        }
        if (serialLength < sizeof(serialLine) - 1 && !serialOverflow) serialLine[serialLength++] = c;
        else serialOverflow = true;
    }
}

void setup() {
    MakerSumo.begin();          // pins + USB identity, before Serial.begin()
    Serial.begin(115200);
    stopRobot();
    EEPROM.begin(EEPROM_SIZE);
    // Hold START while powering on to calibrate the edge sensors
    // (LED on: START on black; LED blinks: START on the white line).
    if (MakerSumo.isStartPressed()) MakerSumo.calibrateEdgeSensors();
    MakerSumo.loadEdgeCalibration();
    beginRcCapture();
    loadSettings();
    selectedMode = MakerSumo.readDipSwitch();
    inputAt = millis();
    enterState(ARMING);
    startBuzzer(false);
}

void loop() {
    runRobot();
    processSerial();
    updateBuzzer();
}
