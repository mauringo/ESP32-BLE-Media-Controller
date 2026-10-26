# Wiring notes

The firmware's current pin assignments are:

| Signal | ESP32-S3 SuperMini GPIO | Notes |
| --- | ---: | --- |
| Rotary encoder A | 12 | Quadrature input |
| Rotary encoder B | 11 | Quadrature input |
| Encoder push switch | 13 | Mute |
| Analog button ladder output | 8 | ADC input |
| On-board RGB LED | 48 | Firmware status indicator |

The analog input supports three buttons on a shared ADC pin. The firmware's
single-button ADC windows are 600–810, 1400–1620, and 2200–2380 (12-bit ADC);
the measured first+third pairing gesture uses 2410–2502. A calibration command
can update the pairing window, but not the three single-button windows.

## Important limitation

The repository records GPIO assignments and measured ADC ranges, but not the
button ladder's resistor values, complete wiring topology, encoder part
number, or connector pinout. The [prototype photos](images/controller-front.jpg)
show an analog button module and rotary control, while
[the interior photo](images/controller-internals.jpg) shows the assembled
board, battery, and charging module. These photos do not expose enough detail
to establish every electrical connection. For that reason, the included
[pin map](pin-map.svg) shows logical signal connections only. It is not a
fabrication-ready schematic. Confirm the actual board and input circuit before
wiring; do not infer resistor values from the ADC windows.

## Firmware input states

- Encoder rotation sends one volume step per encoder count.
- Encoder push sends mute, with a 500 ms repeat guard.
- ADC buttons map to previous, play/pause, and next in that order.
- During light sleep, the analog input is sampled every 50 ms. Encoder pins
  provide GPIO wakeup.
- GPIO 48 is the built-in RGB status LED; sleep turns it off.
