# avr-ppm-extender

Takes a usual PPM servo pulse and extends it to be suiatable for 270 degree servos.

90 degree servo pulse length: 1 ms between 1 and 2 ms, middle 1.5 ms
270 degree servo pulse length: 2 ms between 0.5 and 2.5 ms, middle 1.5 ms

PPM signal in: PB1/INT0
PPM signal out: PB4
