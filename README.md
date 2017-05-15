![POPS Instrument View](https://cloud.githubusercontent.com/assets/23479476/24631331/67d93452-18af-11e7-8163-dda7b6a48b0f.jpg)

# NOAA BeagleBone Black (BBB) PRU DAQ Cape

<p>The NOAA BeagleBone Black PRU and DAQ Cape was developed for the NOAA Printed Optical Particle 
Spectrometer (POPS) instrument that is used to make balloon and aircraft particle concentration measurements. The 
POPS instrument is patented and can be licensed for $500 per year.  Contact Derek Parks (Derek.Parks@NOAA.gov) for 
more information. This cape and software has been patented and is being presented (with the included disclaimer) 
free for public download, modification and use. The programs that are presented are for the POPS instrument, and are
meant as an example and guide for developing other applications.

Cape Features
-----------------

* A high-speed LVDT serial interface using a DS90C124 deserializer.  It connects via a SATA cable to an 
LT2203 16 bit A/D converter located on the photomultiplier (PMT) interface board, sampling at 4 MHz.
This uses both PRUs to implement, and the data is passed in memory.

* MAX5802 D/A converter providing two 12-bit analog out channels (0-5 V) used for controlling the 
instrument's flow rate and laser power (0-5 V).

* An on-board MS5607-02BA03 pressure/temperature sensor.

* A DS3231 hardware realtime clock and battery holder.

* MAX3223 RS232 interface for two UARTs. UART1 is duplicated in TTL.

* M24256 cape EEPROM.


Provided Information
------------------------

* This README.md.

* SOFTWARE DISCLAIMER.md:  This file is to be included with any code that is copied from this 
distribution and used in another application.

* NOAA_BBB_PRU_DAQ_Cape_Setup.md:  This file describes how to setup the BeagleBone Black to 
correctly configure it to work with the cape.

* NOAA_BBB_PRU_DAQ_Cape Pin Use.md:  The pins used by the cape, and other pin information, is 
presented in this document as a table.


Hardware
------------

* README.md file for the hardware.

* NOAA_BBB_PRU_DAQ_Cape, PMT interface board, and base plate Eagle PCB files.

* BOM as an Excel spreadsheet for the boards.
	
    
Software and Examples
-------------------------

* build and build_dt:  Scripts to build the *.c and *.p files provided.

* cape-bone-iio-00A0.dtbo:  The analog in device tree binary object file.

* clock_init.sh:  The clock initialization shell.

* iolib.c, iolib.h, iolib.o, libiofunc.a and MAKEFILE:  The library libio is used for digital IO in the software.
	
* NOAA *.dts and *.dtbo files:  Used to set the Analog In, GPIO, PRUs and UARTS pins.

* PRU0_ParData.p, PRU1_All.p and PRU1_All_dt.p: Assembly language programs for the POPS instrument.

* POPS_BBB.c and POPS_BBB_dt.c:  The main program for the POPS instrument. Demonstrates the capacity of the cape.

* POPS_BBB.cfg:  The configuration file for the POPS instrument.

* rc.local:  File for startup of services on boot.

* rtc-ds1307.service:  Hardware realtime clock service setup.

* Software_install_1 and Software_install_2:  Scripts for installing the software on the 
Beaglebone Black.


Disclaimer
----------------------------------

This repository is a scientific product and is not official communication of the National Oceanic and Atmospheric 
Administration, or the United States Department of Commerce. All NOAA GitHub project code is provided on an ‘as is’ basis 
and the user assumes responsibility for its use. NOAA has relinquished control of the information and no longer has 
responsibility to protect the integrity, confidentiality, or availability of the information. Any claims against the Department 
of Commerce or Department of Commerce bureaus stemming from the use of this GitHub project will be governed by all applicable 
Federal law. Any reference to specific commercial products, processes, or services by service mark, trademark, manufacturer,
or otherwise, does not constitute or imply their endorsement, recommendation or favoring by the Department of Commerce. The 
Department of Commerce seal and logo, or the seal and logo of a DOC bureau, shall not be used in any manner to imply endorsement 
of any commercial product or activity by DOC or the United States Government.



