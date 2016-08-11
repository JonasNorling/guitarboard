This directory contains the hardware design in KiCAD format.

Board changelog and errata
--------------------------

### Errata for board revision C

- Line out is missing footprints for some of the resistors recommended
  in the datasheet. It has not been tested.
- Missing a discharge path for the audio input, as recommended in the
  datasheet. I don't think it's necessary in this application.
- There is too much capacitance on the board to comply with the USB
  standard, and inrush current is not limited.

### Changes in board revision C

- The ground plane has been redesigned; the audio and digital parts
  are now joined in only one point. This was done with the hope of
  minimising audible interference from the MCU.
- Hole and pad sizes for test points have been adjusted, because ITEAD
  Studio had trouble manufacturing rev.B boards without meddling with
  holes and pads.
- Now using two smaller voltage regulators to further separate audio
  and MCU.
- C28, the bulk capacitor before the voltage regulator has been
  removed.
- The diode ORing power from USB or the other connector has been
  removed. It was felt that the voltage drop was slightly troublesome
  and it was better to allow a slightly lower battery voltage instead.
- Footprints for SMD LEDs removed; use 2.54mm header or leaded LEDs.
- Moved stuff around a bit ("optimised component placement").
- RST header.

### Errata for board revision B

- The bulk capacitor upstream of the voltage regulator, C28, should
  not be mounted. It has been discovered that it couples noise from
  the MCU into the ground plane close to the codec. Putting a
  capacitor there makes the audible noise more noticable.
- Putting a resistor (roughly 2-30 ohms) or an inductor in the analog
  power supply path improves audible noise. This can be achieved by
  scraping open the net tie W2 on the backside of the board and
  mounting a 0805 on 1206 resistor there.
- The pin marked PA8 on the silkscreen is in fact not PA8.
