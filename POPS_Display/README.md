# POPS Display and File Conversion

<p>A POPS Display user interface and POPS Peak binary file conveter in compiled LabVIEW are provided for 
calibration of the POPS instrument and conversion of the binary peak data to a comma separated data file that can
then be used in a variety of ways by the experimenter. Versions are provided for Windows (7 or 10) and MacIntosh PCs. Both of these 
installations start by downloading and installing the Runtime Engine for LabVIEW 2018 from the Nationa Instruments web site 
(ni.com, this software is free).
Then copy the applications (for a Mac) or executable (for a PC) files to your PC. They will open and run when double clicked. 
The POPS instrument can be connected to the display program on the PC by a serial cable to UART2, or an ethernet cross cable 
between the PC and the POPS. 
Usually the POPS is set to an IP address of 10.1.1.150 and the PC is set to 10.1.1.100 with communication over port 10150,
but this can be changed on the POPS in the /etc/network/interfaces file and the POPS_BBB.cfg file edited to match.  
If no data comes through, then COM3 is probably not the right serial port. 
Stop the display program (Red STOP button) and discover the correct port by pressing on the down arrow on the right side of the control
and then restart it with the run arrow on the upper left. Instructions for using the file conversion vi are on the front panel.