# POPS Display and File Conversion

<p>A POPS Display user interface and POPS Peak binary file conveter in compiled LabVIEW are provided for 
calibration of the POPS instrument and conversion of the binary peak data to a comma separated data file that can
then be used in a variety of ways by the experimenter. Installation is accomplished on a Window 7 or 10 PC by 
unzipping the POPS_Installer.zip file and running ..\POPS_Installer\Volume\setup.exe by double clicking on it. This installs 
POPS_BBB_Display and Read_POPS_Binary_Files. The POPS instrument can either be conneced to the PC by a serial to USB cable connection 
to UART2, or an ethernet cross cable. Usually the POPS is set to an IP address of 10.1.1.150 and the PC is set to 10.1.1.100, 
but this can be changed on the POPS in the /etc/network/interfaces file and the POPS_BBB.cfg file edited to match. Instructions for 
using the file conversion vi are on the front panel.