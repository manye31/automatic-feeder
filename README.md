# Automatic Animal Feeder

*Liam Melegari, Micah Nye, Sean Young*

<img src="docs\animal_feeder.gif" alt="Animal Feeder" width="400"/>

This is an automatic animal feeder for a mechatronics project built off ATMega328P microcontrollers. The prototype feeder can feed user-specified increments of food to a given frequency per day, such as every 6 hours, for example.

During feed, an auger translates food forward from a feeding trough, into a chute which dispenses into the pet's bowl. Under the bowls are load cells which read the weight of the food. Given the density of the food, we can determine exactly how much food is on the bowl and can stop the motor precisely at the time it's reached the increment.

<img src="docs\trough_empty.gif" alt="Animal Feeder Trough"/>

## User Interface
| Input | Options | Purpose |
| ----- | ------- | ------- |
| Serving size | 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0 | Amount of food to feed pet [cups]|
| Current time | HH:MM AM/PM | Initial time to preset system clock to |
| Feed Routine | 6, 12, 24 | Feed frequency to try to feed pet [hr] |
| Animal | cat/dog | Animal type, used to drive motor speed, for larger food |
| # Pets | 1, 2 | Number of pets to feed |
| Manual feed override | -- | Override the clock and manually trigger motor to feed | 

## Motor Control
A 12 V, 5 A high power [Pololu 3206 Brushed DC motor](https://www.pololu.com/product/3206) with a 75:1 gear ratio is used to drive the auger. This was spec'd due to an analysis on the friction in the feed channel and the need to crush jammed food. 

A [stepper motor](https://www.digikey.com/en/products/detail/sparkfun-electronics/ROB-10551/5766908) is used to control the food chute gate, for 2-pet modes. 

## Load Cell
The [4541 load cell](https://www.adafruit.com/product/4541) is used in combination with the [HX711 Amplifiers](https://www.sparkfun.com/products/13879) to measure the load from which the microcontroller can use.

<img src="docs\animal_feeder.jpg" alt="Animal Feeder" width="400"/>
