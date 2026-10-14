/*******************************************************************************
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTY AND SUPPORT
 * IS APPLICABLE TO THIS SOFTWARE IN ANY FORM. CYTRON TECHNOLOGIES SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 ********************************************************************************
 * DESCRIPTION:
 *
 * This example prints the USB identity the library applies (VID 0x2E8A,
 * PID 0x112F, "Cytron" / "Maker Sumo RP2350") and the library version.
 * Check the device name in Device Manager, lsusb or System Information.
 *
 * AUTHOR   : Alif Aiman
 * COMPANY  : Cytron Technologies Sdn Bhd
 * WEBSITE  : www.cytron.io
 * EMAIL    : support@cytron.io
 *
 *******************************************************************************/

// Shows the USB identity the library applies and the library version.
// After upload, the board enumerates as 2E8A:112F "Cytron Maker Sumo RP2350".
// Check with: lsusb (Linux), Device Manager (Windows), System Information (macOS).
#include <MakerSumoRP2350.h>

void setup() {
    MakerSumo.begin();          // applies the identity before Serial starts
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.print("Maker Sumo RP2350 - ");
    MakerSumo.printIdentity(Serial);
    Serial.println();
    Serial.printf("Manufacturer: %s\nProduct:      %s\n",
                  MAKER_SUMO_USB_MANUFACTURER, MAKER_SUMO_USB_PRODUCT);
#ifdef ARDUINO_CYTRON_MAKER_SUMO_RP2350
    Serial.println("Board variant: arduino-pico 'Maker Sumo RP2350' (identity from the core)");
#else
    Serial.println("Board variant: generic (identity applied at runtime by the library)");
#endif
}

void loop() {
    digitalWrite(LED, (millis() / 500) % 2);
    delay(50);
}
