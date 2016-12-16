#POPS Software

The BeagleBone Black (BBB) must be configured to the specification in the NOAA_BBB_PRU_DAQ_Cape_Setup.md file before use.
The POPS software consists of a main program written in C and assembly language programs for the two PRU's. The program 
features are outlines below.  The build files for the programs are included. The file called `build` creates the program
`pops` that has data output that included the particle peak, position of the maximum, width and number saturated. The
file called `build_dt` creates `popsdt` that has data output of particle peak, width, and cycles from last particle. 
This enables the calculation of the time difference between particles. 

##POPS_BBB.c Code Features

* Capable of measuring up to 30,000 particles/second.
* Reads a configuration file for instrument specific parameters.
* Creates log, one-second data (housekeeping), and peak data files.
* Configures the PRUS and PRU memory and starts the PRU programs.
* Reads the BBB analog in and converts to engineering units. 
* Interfaces with the MAX5802 analog out chip via i2c-1 to set laser power and pump voltage.
* Has a digitally implemented SPI interface to MS5607 chip to read onboard pressure and temperature.
* Implements a watchdog that will reboot the BBB if the software hangs.
* Sends data and reads commands out via serial and UDP connections.
* Uses PRU1 RAM rolling buffer for reading baseline data and sending the current baseline and baseline + threshold 
to PRU1. Also uses PRU1 RAM for keeping track of current buffer addresses.
* Uses PRU0 RAM to read raw data and send a sample out. Very useful in debugging.
* Uses Shared PRU RAM for particle data. Every particle is written to a binary file. It is also binned to create a 
log10 histogram of size.
* Has a one second outer loop, with multiple calls to process the data to keep the buffers from overflowing.
* Has multiple calls to recalculate the baseline to keep the value current.

##PRU1_All.p and PRU1_All_dt.p Features

* Sets up PRU DRAM to pass parameters and baseline data between the CPU program.
* Enables the scratchpad.
* Used data read and data not ready signal to sync the reading of a data point.
* Reads the high byte of the data, and requests the low byte from PRU0 using the scratch pad.
* Sorts the point into a baseline or particle point, and keeps track of particle parameters.
* Uses a cycle counter to determine the time between particles (dt version).
* Writes baseline, raw and particle data to the RAM, and writes out any addresses to be able to read the 
rolling buffers. 
* Checks for a stop condition, and notifies PRU0 and the POPS program to stop.


##PRU0_ParData.p Features

* Waits for a request for the high byte of data.
* Reads the high byte and sends it to PRU1 via the scratch pad.