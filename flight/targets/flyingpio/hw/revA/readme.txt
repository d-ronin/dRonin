FlyingPi was made with CircuitMaker.  The design is published on the
CircuitMaker hub.

FlyingPi RevA is released to the public domain, according to the CC0 public
domain waiver:

To the extent possible under law, Michael Lyle has waived all copyright and related or neighboring rights to FlyingPi. This work is published from: United States.
https://creativecommons.org/publicdomain/zero/1.0/

Known errata in flyingpi RevA:

* Polarity marking on the second LED in the BoM is the OPPOSITE of the other
two.  Use caution and refer to datasheet when assembling.  The board art
has cathode lines, but the middle LED has an anode line on the package.
* Schematic: text refers to receiverport pin as PB0 when it's in fact PB1.
It's electrically fine, though.
* Board silkscreen labels for 'RCVR' and 'SERIAL' are swapped.
* Schematic and board layout have PWM pins in opposite order.  Electrically
correct, but PWM0 is labelled on board art (and in firmware) as Servo6,
PWM1 as Servo5, ...
