# arduino-pico board variant: Cytron Maker Sumo RP2350

This is the Arduino equivalent of the CircuitPython board definition
(`ports/raspberrypi/boards/cytron_maker_sumo_rp2350`). Once it is in the core,
the board appears in Tools > Board as "Maker Sumo RP2350" and enumerates as
`2E8A:112F` "Cytron Maker Sumo RP2350" without any runtime code.

To submit it to https://github.com/earlephilhower/arduino-pico :

1. Copy `variants/cytron_maker_sumo_rp2350/` into the core's `variants/`.
2. Add this line to `tools/makeboards.py` under the `# Cytron` section
   (keep the entries alphabetical):

   ```python
   MakeBoard("cytron_maker_sumo_rp2350", "rp2350", "Cytron", "Maker Sumo RP2350", "0x2e8a", "0x112f", 250, "CYTRON_MAKER_SUMO_RP2350", 2, 0, "none")
   ```

3. Run `python3 tools/makeboards.py` to regenerate `boards.txt` and the
   PlatformIO JSON, then open the PR.

The core defines `ARDUINO_CYTRON_MAKER_SUMO_RP2350` for this board; the
MakerSumoRP2350 library uses that macro to skip its runtime VID/PID setup.

Until the variant is merged, pick Tools > Board > "Generic RP2350" (2 MB
flash) and let the library set the USB identity at runtime.
