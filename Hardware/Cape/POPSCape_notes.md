Beagle Bone Black (BBB) POPS Cape Notes
=======================================

<p>This cape is intended to be a part of a stack of printed circuit boards that comprise 
of the POPS instrument.  As you might expect, the cape plugs into the BBB and has M-F 
pass through connectors.  As currently used, the cape then plugs into a ‘base plate’ that
has some signal conditioning, and the rest of the instrument attaches to the base plate.  
Some signals are passed via the usual P8-P9 connectors, while other signals are passed to
the base plate via JP1, a 14 pin connector of similar style as P8 and P9.  If a base 
plate is not to be used, these signals are broken out on other connectors.


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
