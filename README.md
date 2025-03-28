# avr-ppm-extender

Takes a ordinary PPM servo pulse and expands it so that is is suitable for 270 degree servos.

90 degree servo pulse length: 1 ms; between 1 and 2 ms, middle 1.5 ms

270 degree servo pulse length: 2 ms; between 0.5 and 2.5 ms, middle 1.5 ms

MCU: ATtiny13A

CPU clock: internal 9.6 MHz oscillator, prescaling factor 1, changed by software from 1/8 to 1 on startup
TIMER 0 clock: prescaling factor 1/8 = 1.2 MHz

PPM signal in: PB1/INT0, internal pull-up resistor activated

PPM signal out: PB4

A demo application that moves the servo from left to right and vice versa can be created by defining 
the variable DEMO at the make commandline, e.g., make clean build DEMO=1

This demo application does not make use of any servo input.
