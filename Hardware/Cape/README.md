Beagle Bone Black (BBB) POPS Cape Notes
=======================================

<p>This cape is intended to be a part of a stack of printed circuit boards that comprise 
of the POPS instrument.  As you might expect, the cape plugs into the BBB and has M-F 
pass through connectors.  As currently used, the cape then plugs into a ‘base plate’ that
has some signal conditioning, and the rest of the instrument attaches to the base plate.  
Some signals are passed via the usual P8-P9 connectors, while other signals are passed to
the base plate via JP1, a 14 pin connector of similar style as P8 and P9.  If a base 
plate is not to be used, these signals are broken out on other connectors.

![POPS Instrument View](https://cloud.githubusercontent.com/assets/23479476/24631331/67d93452-18af-11e7-8163-dda7b6a48b0f.jpg)

Note: The BeagelBone Black is mounted upside down on the base plate.



<p>The cape is powered by a signal labeled ‘V-Batt’.  This comes in via J1 pins 3 and 4,
or if a baseplate is not used, via the connector named ‘BATTERY_IN’.  This voltage is 
scaled and monitored via A/D channel AI6 pin P9.35.  To turn on the cape (which powers 
the BBB) the ‘ON’ button is pressed.  There is an ON button located on the cape, and can 
also have one on the baseplate using pin 12 of JP1.  The ON button turns on FET T3 which 
supplies +12V to the cape and to the base plate via J1 pins 5 and 6.  On the cape is a 5V 
regulator which then supplies +5V to the BBB via P9 pins P9.5 and P9.6.  The BBB then 
regulates the 5V down to +3.3V and supplies it back to the cape  via pins P9.3 and P9.4.  
Among other things, this 3.3V is then used to hold on FET T3 so the system remains powered 
up after the ON button is released.  When the BBB shuts down, 3.3V is removed from P9 and 
if the ON button is not pressed, power is removed from the rest of the system.

![BBBCapeTop](https://cloud.githubusercontent.com/assets/23479476/24631333/67dacbaa-18af-11e7-96b3-28ceaf36fbef.jpg)

POPS Cape top view.



![BBBCapeBottom](https://cloud.githubusercontent.com/assets/23479476/24631332/67d9be0e-18af-11e7-90d4-8bdb3782f89e.jpg)

POPS Cape bottom view.


One of the functions of the cape is to read a differential pressure signal, compare against a setpoint (set by one of the two D/A channels) and control a small pump to maintain the setpoint.  The flow signal is the difference in pressure between the two ports on a laminar flow element.  This sub-system provides for a constant volume of gas flow.  The pump power is supplied via JP1 pins 1 and 2.  If a base plate is not used, connector P6 can be used.
The flow signal from the differential pressure sensor is supplied to the cape either via JP1 pin 13 or if the baseplate is not used, via J4.  It is scaled (conditioned) and is presented to the BBB via AIN0 P9.39.
Two 10K thermistors are measured on the POPS instrument.  One is external to the instrument connected to a connector on the base plate.  This in turn connects to the cape via JP1 pins 9 and 11.  This signal is scaled and provided to the BBB via AIN5 on P9.36.  The other thermistor monitors the laser diode temperature and connects to the POPS optical chamber via 5 pin P1 (note: not JP1!) pins 4 and 5.  Scaled signal is provided to the BBB via AIN2 on P9.367.  On both thermistors, one end is connected to a 3.6019V reference voltage and the other end connects to a 1.24K resistor to ground.  1.24K is also the resistance of the thermistor at its maximum expected temperature of 80.24 deg. C, which presents 1.8V to the BBB.  At 25 deg. C the thermistor value is 10K which presents 0.728V to the BBB.
The last signal on JP1 is named ‘PUSH_2_STOP’.  There is a button on the cape and can have a button on the baseplate.  This signal is buffered and presented to the BBB via P8.30.  It is monitored by the software and when asserted, (list the steps the software does, such as turn off laser, pump, stop taking data, close the files, run the shutdown sequence which removes the +3.3V signal from T3 and removes power to the BBB and cape).

Analog In Scale Factors.

AI0: Flow Signal.  5.036V from diff. pressure signal presents 1.8V to the BBB.
AI1: Pump Feedback.  15.000V from pump control circuit presents 1.8V to BBB.
AI2: Laser Temperature (10K Thermistor).  Max temperature is 80.24 deg. C which presents 1.8V to BBB.  25 deg. C presents 0.728V to BBB.
AI3: Laser Feedback. 5.036V from laser driver presents 1.8V to BBB.
AI4: Laser Photodiode Monitor. 5.036V from laser driver presents 1.8V to BBB.
AI5: External 10K Thermistor.  Max temperature is 80.24 deg. C which presents 1.8V to BBB.  25 deg. C presents 0.728V to BBB.
AI6: Battery Voltage. 20.119V input presents 1.8V to the BBB.

U1: I2C M24256 EEPROM. 

On this cape is a typical EEPROM which is not currently used with our software.  Solder jumpers SJ3 and SJ4 select the address.  By default (no solder jumpers) the address is 0x57.  0x54, 0x55, and 0x56 are the other options.  When SJ5 is shorted with a solder blob it write protects the EEPROM.  It is not write protected by default.  The SDA line is connected to P9.20 and the SCL line is connected to P9.19.

U2: I2C DS3231MZ/V Real Time Clock.

On this cape is a DS3231 battery backed up Real Time Clock.  This particular device has it’s own internal temperature compensated oscillator which keeps it accurate to +/- 5ppm (+/- 0.432 seconds per day) over it’s temperature range.  It’s fixed address is 0x68.  The SDA line is connected to P9.20 and the SCL line is connected to P9.19.  Backup power is provided by a BR1225 coin cell permanently soldered to the cape.  Not using a battery holder insures the coin cell won’t be jarred free.

U3: I2C MAX5802 12 bit dual D/A converter.

On this cape is a MAX5802 12 bit dual D/A converter.  It’s address is set to 0x0F by default and can be changed to 0x0C by moving solder jumper JMP7 to Vcc, or 0x0E by not connecting the solder jumper to either pad.  It is pin selected to have both outputs at 0V on power up and software selected max output voltage of 4.096V.  Ch. 1 controls the laser current and has a 0 to 100mA range.  The laser diode is rated for 100 mA continuous.  Out Ch. 2 provides the set point for the flow controller.  The controller servos the voltage to the pump so that this set point matches the signal from the flow sensor.  Since the actual flow signal is dependent on the laminar flow element geometries and particular differential pressure sensor used, the final calibration is done for each individual instrument.  Typical values have been around 3.5V from the D/A equal to 3 cc/second.

PRES1: SPI MS5607-02BA03 Barometric Pressure sensor with Temperature.

The MS5607 Barometric pressure sensor is used to measure altitude of the POPS instrument.  It has a range of 10 to 1200 mbar and a resolution of 0.024 mbar (20 cm of altitude).  Included is a 0.01 deg. C resolution, +/- 0.8 deg. C. accurate temperature sensor 
which is used to monitor the cape’s temperature.  The BBB talks to it via a bit-banged SPI interface with the SCLK to P9.13, the chip select signal connected to P9.14, the SDO to P9.15 and SDI to P9.16.

Serial Ports: RS-232 and TTL

There are two serial ports brought out on this cape.  UART1 TX signal (data leaving the cape) is on pin 9.24 and RX signal (data entering the cape) is on pin 9.26.  UART2 TX signal is on pin 9.21 and RX signal is on pin 9.22.  UART1 has two connectors.  J5 has the 3.3V TTL signals on it and J2 has the same data but buffered for RS-232 signal levels.  As for the UART1 RX signal, there is a solder jumper JMP2 that selects between the signal from the RS-232 buffer chip IC6 (MAX3223) or the signal from J5.  Since for our current applications no data is received on UART1 we currently don’t populate either side of solder jumper JMP2.  There is a through hole jumper connector that serves the same function as JMP2 called JP2.  If we had to frequently change which source of data JP2 would allow this without use of a soldering iron.  UART1 is used to send status data to either aircraft telemetry (the RS-232 buffered signal) or send 3.3V level status data to an IMET radiosonde via J5.  UART2 is always buffered to RS-232 signal levels and is on connector J3.  UART2 is used to communicate with a laptop running LabVIEW user interface software.

P2: High Speed A/D connector

In order to reduce interference and noise pickup, the high speed A/D converter is located as close to the signal source (Photo Multiplier Tube) as possible.  This converter is a Linear Technology LTC2203 16 bit, 25 MSPS converter with a parallel output, running at 4 MSPS.  The digital signals from the converter are serialized using a TI DS90C241 serializer.  The LVDS serial stream uses standard SATA connectors and cables to connect to the cape.  SATA cables were used for their low cost and availability.  Connector P2 on this cape has the serial data stream and provides power to the PMT and signal conditioning electronics.  The signals transferred are listed below.
Over/Underflow: P8.29
Data Ready: P8.27
AD15: P9.25
AD14: P9.41
AD13: P9.27
AD12: P9.42
AD11: P9.28
AD10: P9.30
AD9: P9.29
AD8: P9.31
AD7: P8.40
AD6:P9.39
AD5: P8.42
AD4: P8.41
AD3: P8.44
AD2: P8.43
AD1: P8.46
AD0: P8.45

There are 2 LEDs associated with the deserializer.  One to display the Over/Underflow status and another to show the status of the link between the deserializer and serializer.

P1: Laser connector

Connector P1 has 5 pins and connects to the laser inside the optical chamber.  Pin 1 is the laser diode Anode, pin 2 is the laser diode Cathode, and Pin 3 is the photodiode internal to the laser module.  0 to 100mA of current is controlled by the value on D/A channel A and the signal on pin 3 is signal conditioned and reported back to the BBB on channel AIN4.  Pin 4 has the reference voltage for the thermistor that is mounted on the laser’s heat sink, and pin 5 connects to ground through a 1.24K resistor.  The voltage on pin 5 is conditioned and sent to the BBB on channel AIN2.
