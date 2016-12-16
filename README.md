![pops_bbb_noaa](https://cloud.githubusercontent.com/assets/23479476/21242758/babe1360-c30d-11e6-89c9-b6001e482500.JPG)

# NOAA_BBB_PRU_DAQ_Cape

<p>This cape was developed for the NOAA Printed Optical Particle Spectrometer (POPS) instrument that 
is used to make balloon and aircraft particle concentration measurements.  It has been patented and 
is being presented (with the included disclaimer) free for public download, modification and use.  
The programs that are presented are for the POPS instrument, and are meant as an example and guide 
for developing other applications.

##Cape Features:

* One 16-bit high-speed (4 MHz minimum) analog in channel from a ***** chip (not provided).  The input is 
through an SATA cable. This uses both PRUs to implement, and the data is passed in memory.

* Seven 12-bit on-board analog in channels (0-1.8 V).

* Two 12-bit analog out channels (0-5 V).

* An on-board pressure sensor.

* A hardware realtime clock and battery holder.

* 2 UARTs in RS232. UART1 is duplicated in TTL.

* UDP communications. (TCP/IP communications could be implemented, but was not used for the POPS application.)

* The capability to run from a power supply or battery.

##Provided Information:

* This README.md.

* SOFTWARE DISCLAIMER.md:  This file is to be included with any code that is copied from this 
distribution and used in another application.

* NOAA_BBB_PRU_DAQ_Cape_Setup.md:  This file describes how to setup the BeagleBone Black to 
correctly configure it to work with the cape.

* NOAA_BBB_PRU_DAQ_Cape Pin Use.md:  The pins used by the cape, and other pin information, is 
presented in this document as a table.

* ????:  NOAA_BBB_PRU_DAQ_Cape layout files.

* ????:  Materials list for the cape.
	
##Software and Example:

* build:  A script to build the *.c and *.p files provided.

* cape-bone-iio-00A0.dtbo:  The analog in device tree binary object file.

* clock_init.sh:  The clock initialization shell.

* iolib.c, iolib.h, iolib.o, libiofunc.a and MAKEFILE:  The library libio is used for digital IO in the software.
	
* NOAA *.dts and *.dtbo files:  Used to set the Analog In, GPIO, PRUs and UARTS pins.

* POPS_BBB.c:  The main program for the POPS instrument. Demonstrates the capacity of the cape.

* POPS_BBB.cfg:  The configuration file for the POPS instrument.

* rc.local:  File for startup of services on boot.

* rtc-ds1307.service:  Hardware realtime clock service setup.

* Software_install_1 and Software_install_2:  Scripts for installing the software on the 
Beaglebone Black.
	