# dRonin'

dRonin' is an autopilot/flight controller firmware for controllers in the OpenPilot/Tau Labs family.  It's aimed at a variety of use cases: acro/racing, autonomous flight, and vehicle research.

dRonin' is production ready and released under the GPL. 

Presently supported targets include:

- OpenPilot Revolution
- AeroQuad32
- Quanton
- Sparky
- Sparky2
- BrainFPV (see below)
- Team Black Sheep Colibri / Gemini hexcopter
- DiscoveryF3 and DiscoveryF4 evaluation boards from ST Micro (informal support only)
- CC3D (limited, non-navigation functionality)
- Naze32 (limited, non-navigation functionality)

# BrainFPV

- MPU9250 is on I2C: two separate drivers exist, need to be factored later to decide what to do on the bust in via probing or link time configuration.
