# Arduino Library for Cytron Maker Sumo RP2350
This library provides functions for the [Maker Sumo RP2350](https://www.cytron.io) sumo robot controller: motors, opponent and edge sensors, DIP switches, START button, IR start input, buzzer and battery monitor.<br>
It also sets the board's USB identity (VID `0x2E8A`, PID `0x112F`, "Cytron Maker Sumo RP2350") so the board is recognised by name.<br>
Please refer to the examples on how to use the library.<br>
Connections are described in the comment section of each example.

## Installation
1. Open the Arduino IDE, select `File` -> `Preferences` and add this URL to `Additional boards manager URLs`:<br>
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. Select `Tools` -> `Board` -> `Boards Manager...`, search for `Raspberry Pi Pico/RP2040/RP2350` and click `Install`.
3. Select `Tools` -> `Board` -> `Raspberry Pi Pico/RP2040/RP2350` -> `Cytron Maker Sumo RP2350`.<br>
   (If it is not listed yet, choose `Generic RP2350` and set `Flash Size` to `2MB`. The library applies the USB identity itself.)
4. Select `Sketch` -> `Include Library` -> `Manage Libraries...`.
5. Search for `MakerSumoRP2350`.
6. Click `Install` to install the library.
7. Restart the Arduino IDE.
8. Examples can be opened in Arduino IDE from `File` -> `Examples` -> `MakerSumoRP2350`.

## Pin Assignment
| Name | GPIO | Function |
|---|---|---|
| `USR_BTN` / `START` | GP2 | START button, active LOW |
| `LED` | GP3 | User LED |
| `DIP1`, `DIP2`, `DIP3` | GP4, GP5, GP6 | DIP switches, SW1 is the MSB |
| `OPP_L`, `OPP_FL`, `OPP_FC`, `OPP_FR`, `OPP_R` | GP11, GP20, GP21, GP22, GP8 | Opponent sensors, active LOW |
| `PWM_L`, `DIR_L` | GP15, GP14 | Left motor |
| `PWM_R`, `DIR_R` | GP9, GP10 | Right motor |
| `FAULT_L`, `FAULT_R` | GP19, GP25 | Motor driver fault, active LOW |
| `BUZZER` | GP12 | Piezo buzzer |
| `SERVO1`, `SERVO2` | GP16, GP17 | Servo / RC receiver headers |
| `IRST` | GP18 | IR start module input |
| `EDGE_L`, `EDGE_R` | GP26, GP27 | Edge sensors (analog) |
| `POT` | GP28 | Potentiometer (analog) |
| `VM_SENSE` | GP29 | Motor supply voltage, 1:11 divider |

## Examples
Examples are grouped as **Basics**, **Sensors**, **Sumo** and **Diagnostics**. Work through them in order.

### Basics
#### 1. Blink
Flashes the on-board LED and plays a melody on the buzzer.<br>
The first sketch to try: it confirms the core, board and library are installed.

#### 2. ButtonAndDip
Reads the START button and the three DIP switches.<br>
The LED follows the button; changing a DIP switch beeps and prints the new value (0-7, SW1 is the MSB).

#### 3. MotorTest
Drives both motors through forward, backward, turn left and turn right when START is pressed.<br>
Use it to confirm the motor direction. Put the robot on a stand first.

### Sensors
#### 4. OpponentSensors
Prints which of the five opponent sensors see something and lights the LED when any does.

#### 5. EdgeSensors
Prints the edge sensor readings and lights the LED when the white border is seen.<br>
The threshold is calibrated from the black surface at power-on; the potentiometer sets the sensitivity.

#### 6. BatteryMonitor
Prints the motor supply voltage and warns with the buzzer when it is low.<br>
Set `LOW_BATTERY_VOLTS` for your pack (6.6 V for 2S, 9.9 V for 3S).

#### 7. IrStartModule
Shows START and STOP from a competition IR start module on the `IRST` pin, with the START button as fallback.

### Sumo
#### 8. AutoSumo
A complete autonomous sumo program in plain code with all tuning values at the top.<br>
Searches, pushes when the opponent is seen and backs away from the border. Starts from the button (5 s countdown) or the IR module.

#### 9. RcSumo
Drives the robot from an RC transmitter connected to `SERVO1` (throttle) and `SERVO2` (steering).<br>
DIP positions LHL and HLH enable live motor alignment with the potentiometer; hold START for 2 s to save.

#### 10. AutoRcSumo
The full firmware: DIP-selectable opening strategies, a defense routine on DIP 5, RC driving on DIP 7, edge escape, and all settings stored in flash.<br>
Settings can be changed over USB with the Web Serial configurator (protocol 6).

### Diagnostics
#### 11. TestInputs
Prints every input once a second. Use it to check wiring and sensor polarity.

#### 12. TestOutputs
Exercises the buzzer, LED and both motors when START is pressed.

#### 13. UsbIdentity
Prints the USB VID/PID, product strings and library version.

## Functions
```cpp
MakerSumo.begin();                       // Call first in setup(). Sets up pins, PWM, ADC and USB identity.

int     MakerSumo.readDipSwitch();       // 0-7, SW1 = MSB, 1 = ON
bool    MakerSumo.isStartPressed();      // START button
bool    MakerSumo.irStartActive();       // IR start module signalling START
uint8_t MakerSumo.readOpponents();       // bit0 L, bit1 FL, bit2 FC, bit3 FR, bit4 R
float   MakerSumo.readBatteryVoltage();  // volts
bool    MakerSumo.motorFault();          // true if a motor driver reports a fault

void    MakerSumo.stop();
void    MakerSumo.moveForward(int speed);          // speed 0-255
void    MakerSumo.moveBackward(int speed);
void    MakerSumo.turnLeft(int speed);
void    MakerSumo.turnRight(int speed);
void    MakerSumo.setMotorSpeed(int side, int speed);   // MOTOR_L or MOTOR_R, -255 to 255

void    MakerSumo.playTone(int pitch, int ms);
void    MakerSumo.playMelody(const int *pitch, const int *duration, int length);
void    MakerSumo.printIdentity(Stream &out);      // e.g. MakerSumo.printIdentity(Serial);
```
Pitches `NOTE_C4` to `NOTE_C7` are defined for `playTone()` and `playMelody()`.

## Configuration
These defaults can be changed with `#define` before `#include <MakerSumoRP2350.h>`:

| Define | Default | Meaning |
|---|---|---|
| `DIP_ON_LEVEL` | `LOW` | Pin level when a DIP switch is ON |
| `MOTOR_FORWARD_LEVEL` | `LOW` | DIR pin level for forward |
| `MOTOR_PWM_HZ` | `20000` | Motor PWM frequency |
| `VM_DIVIDER_RATIO` | `11.0` | Battery voltage divider ratio |

## Versioning
`MAKER_SUMO_RP2350_LIB_VERSION` in `src/MakerSumoRP2350.h` and `version=` in `library.properties` are kept equal.<br>
`RcSumo` and `AutoRcSumo` report their own firmware version and the library version in the `HELLO` reply.

## License
MIT License. See [LICENSE](LICENSE).
