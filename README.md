# Organ

[[_TOC_]]

## Introduction

Copyright 2026 Remco Post

[License AGPL](./LICENSE.md)

A small controller for a pipe organ

## Layout

The organ has 3 registers, of 42 notes each. The cade currently has hard coded that C3 (note 48) is the base of all three registers.

## Shift

To enhance the functionality, the registers can be shifted up or down. While the code supports a single key shift, in reality one should only shift by 12.

## MIDI

MIDI channel 1 is used for tone on/off, MIDI channel 0 for registers (stops)

### Register (stop) on/off

In electronic organs it's customary to not use MIDI control change to switch stops.

| key | register | shift |
|-:|-:|-:|
| 0 | - | - |
| 1 | - | - |
| 2 | 0 | -2 |
| 3 | 0 | -1 |
| 4 | 0 | 0 |
| 5 | 0 | +1 |
| 6 | 0 | +2 |
| 7 | - | - |
|8|-|-|
|9|-|-|
| 10 | 1 | -2 |
| 11 | 1 | -1 |
| 12 | 1 | 0 |
| 13 | 1 | +1 |
| 14 | 1 | +2 |
| 15 | - | - |
| 16 | - | - |
| 17 | - | - |
| 18 | 2 | -2 |
| 19 | 2 | -1 |
| 20 | 2 | 0 |
| 21 | 2 | +1 |
| 22 | 2 | +2 |
| 23 | - | - |

For each stop, the last 3 bits are ignored on the note off, so 'note off 8' throgh to 'note off 15' can be used to switch the stop off.

## Control

The pipes are controlled via a set of 74hc594 shift registers. These are casecaded, QH7' from the first connected to SER of the next with a maximum of 16 shift registers in series. We have the low note of the first register at the end of the 128 bit series.

### Pinout

As seen from the (male) input connector.

```
_____________
\ 1 2 3 4 5 /
 \ 6 7 8 9 /
  ---------
```

1. Vcc (+)
2. /SHR - SCLR - shift register clear
3. SHCP - SCLK - shift register clock
4. Vdd (-)
5. n.c.
6. n.c.
7. STCP - RCLK - output register clock
8. DS - SOUT - data
9. n.c.

The RCLR - output register clear is not connected on the Musicom boards.

On the output connector (female) pin 8 is conencted to QH7' of the last shift register.

### Indicator leds

The output indicator leds are powered via the load (relais etc.) and are placed in parallel to the output FET.