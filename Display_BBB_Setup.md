BeagleBone Black Setup for Display
==================================
<p>The 4D Systems LCD 4.3" Display (4DCAPE-43T) is mounted to a BeagleBone Black (BBB) that requires some setup for the 
POPS display application to function properly. In general, it must have the operating system installed and configured, and
software installed. The Qt display application must be installed and made to autorun at boot.

<p>The operating system used is '<BBB-eMMC-fasher-debian-7.5-2014-05-14-2gb.img>' (available from 
BeagleBoard.org). This is flashed on to the eemc memory.  No changes need to be made to the uEnv.txt file, and no capes need 
to be loaded.  The BBB is connected to a laptop with an internet connection
(or directly to the internet) to install software and perform an update.  These instructions are found in the 
NOAA_BBB_PRU_DAQ_Cape_Setup.md file under the POPS instrument.  

<p>The uSD card is available for transfering files or making backups;  this is configured with:

    sudo mkdir /media/uSD
    sudo chmod 777 /media/uSD
    
<p>The rc.local file is used to automatically mount the uSD on boot by inserting the line below before 'exit 0'.

    mount /dev/mmcblk0p1 /media/uSD

<p>The files that must be transfered to the /home/debian directory are the POPS directory, the UDP directory (only used to 
troubleshoot communications) and QCustomPLot.tar.gz downloaded from [QCustomPlot](http://www.qcustomplot.com/index.php/download).

<p>With this much done, connect to the internet, and set the time with '<ntpdate -b -s -u time.nist.gov>'.  Then do the following:

    sudo apt-get update
    sudo apt-cache search qt4
    sudo apt-get install qt4-dev-tools
    sudo apt-get install qtcreator
    tar -zxvf qcustomplot.tar.gz    
    
<p>Edit '</etc/network/interfaces>' to include the static port on eth0:

    auto eth0
    iface eth0 inet static
    address 10.1.1.4
    netmask 255.255.255.0
    network 10.1.1.0
    broadcast 10.1.1.255
    
<p>The next steps are more easily performed using a monitor and keyboard plugged in to the BBB.    
Prepare the desktop for the POPS Data Display:

*Move the lower panel by right clicking on it and selecting "Panel Settins".  Under "Geometery" select Position right and top, size 25 pixels wide.
In "Appearance" select solid color. In "Panel Appleta" delete the clock and desktop pager. 
*Right click on the Desktop and select "Desktop Preferences". In the Wallpaper mode select Fill with background color only.
*If there is a top white bar, that will need to be removed also.

Make the POPS Data Display automatically start on boot by editing the file '</etc/xdg/lxsession/LXDE/autostart. Add the 
line:

  @/home/debian/POPS/pops.sh
  
<p>Open QtCreator to configure it.  First under Tools open Options.  Add the QCustomPlot Documentation by selecting "Help", and the 
Documentation tab. Click Add, and find the file '</home/debian/qcustomplot/qcustomplot.qch>'.  Next setup the build kit by selecting 
"Build & Run" and checking compilers. If one was not found automatically, add it manually by selecting "Add-GCC" and finding it at 
'</usr/bin/gcc>'. Similarly, the debugger is at '</usr/bin/gdb>'.  The Qt version should show up now without errors.  It is not necessary 
to recompile the program on the BBB, but the setup is done is case any modifications need to be made.  QCustomPlot is used by adding 
the files qcustomplot.h and qcustomplot.cpp to the directory created for the project, then adding them to the project in QtCreator
by right clicking on the main project name and selecting "Add Existing Files". Select the files and "OK". The *.pro file needs to have 
the line '<Qt+= widgets>' appended with '<printsupport>'.

<p>The BBB is now ready to have the 4DCAPE-43T cape installed. Power off completely and disconnect power and the monitor before doing this. 
It should now start witht the POPS Display running.  

POPS Instrument Setup
---------------------

<p>The instrument BBB needs to be running the POPS_BBB_dt_disp.c version of the POPS software, with the POPS_BBB.cfg set to 25 bins, with the logmin
set to 1.4 and the logmax set to 4.817.  The serial communication should be set to false for both uarts, and the UDP[1] should also be set to false.
The display data is sent through the status UDP[0] to 10.1.1.4 (the Display BBB) on port 8000. The instrument is set to the IP address of 
10.1.1.3. The instrument needs to boot first and start sending data before the display Qt application starts. The two are connected with an ethernet 
cross cable.

<p>To restart the network '<sudo /etc/init.d/networking restart>'.  This may be useful if changing cables brings the network down.
    
    