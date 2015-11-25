# dRonin

Open source hardware information (where available) lives in this repository at flight/hardware/*/hw.

## Getting involved
Click that big fork button on github and start coding!  We use pull requests as reviews so expect a lot of constructive feedback!

Chat on freenode.net #dronin

## Code Layout

Here is a quick breakdown of the main directories to get you oriented

* flight - contains the firmware components of the code
* flight/target - the location of the board targets (e.g. flight/targets/freedom)
* flight/PiOS - contains the drivers
* flight/Modules - the flight control logic, broken into modules that communicate via UAVObjects
* flight/tests - unit tests for some components of the flight code
* ground - contains the GCS code
* shared - contains UAV Object definitions shared between the GCS and the flight firmware
* androidgcs - contains the ground control software for android
