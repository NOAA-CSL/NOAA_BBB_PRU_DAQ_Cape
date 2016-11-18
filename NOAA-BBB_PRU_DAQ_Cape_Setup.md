#NOAA BeagleBone Black PRU and DAQ Cape Setup

<p>The NOAA BeagleBone Black PRU and DAQ Cape was designed for use with a small Printed Optical 
Particle Spectrometer (POPS).  It well suited for many other applications, and is therefor presented
here for use as is, or to be modified for other specific cases. The Cape has one very high rate 
analog in channel (4 MHz at 16 bits with control lines), 2 analog out cahnnels, a pressure and 
temperature sensor, a thermistor input available, two RS232 UARTS (one that is duplicated TTL), 
digital IO, and a hardware clock.  The more accurate DS3231W clock is used, but the install 
instructions are the same. The software is presented as used for POPS as an example for how to use 
the features of the Cape.
<p>There are may steps to setup a new BeagleBone Black to function correctly with the NOAA_BBB_PRU_DAQ 
Cape.  These steps set up the operating system, the BeagleBone Black to the desired 
configuration, establish a network connection for software update and upload, and install the 
services used by the cape, such as the real-time clock.  These install instruction assume that the 
user has a basic knowledge of starting up a BeagleBone Black.  If not, see: [BeagleBone Getting 
Started] (http://beagleboard.org/getting-started).
<p>Tools to connect to the BeagleBone Black include the web-based Cloud9 interface, PuTTY (on Windows),
and terminal programs on Windows, MAC, or Linux OS.

##Part 1.  BealgeBone Black Setup

1.  Operating system.  The version of the OS this cape is build around is 
'BBB-eMMC-fasher-debian-7.5-2014-05-14-2gb.img' (available from BeagleBoard.org).  Write this image onto a 
uSD card of at least 4 GB with Win32DiskImager.  Installing it will overwrite anything on the 
BeagleBone Black emmc.  Place the uSD card in the slot, power up the BeagleBone Black with an 
appropiate 5 V power supply while holding down the program button near the uSD card.  When the 
BeagleBone Black powers down, programming is complete.  The rest of the steps can be performed 
using the USB cable to connect with the BeagleBone Black.

2.  Host Name and user.  Edit the name in '/etc/hostname' and '/etc/hosts' if you wish.  It is used in 
the configuration file to keep track of multiple instruments.  The user can just be root (and in 
fact to make some of the changes root privilages are needed), or a user can be created with:

    adduser {newuser} then answer the prompts to create the user.
	
3.  Set up the network connection.  The network connection to the BeagleBone Black that is connected
to the laptop via the USB cable is done by issuing this command in the terminal on the BeagleBone 
Black:

    /sbin/route add default gw 192.168.7.1 
	
It is easy to just use the Google nameserver:

    echo nameserver 8.8.8.8 >> /etc/resolv.conf
    echo nameserver 8.8.4.4 >> /etc/resolv.conf

To add a static IP address for the UDP connection (or a TCP/IP connection) edit 
'/etc/network/interfaces'.  The network settings are up to the user.  A dns nameserver is optional.

    auto eth0
    iface eth0 inet static
    address nnn.nnn.nnn.xxx
    netmask 255.255.255.0
    network nnn.nnn.nnn.0
    broadcast nnn.nnn.nnn.255
    gateway nnn.nnn.nnn.1
    dns-nameservers 8.8.8.8
    dns-nameservers 8.8.4.4

On a Windows laptop:

Choose the laptops's network source, open the Properties. On the Sharing tab set the Allow...box and select the LAN## for the BBB.  Choose the BBB LAN and make sure that the IP is set to Obtain automatically and that Obtain DNS ... automatically are selected.  You may have to get IT permission to share a network with the BeagleBone Black.

For UDP: Choose the LAN network and add settings below (with Ethernet cable connected):

    IP nnn.nnn.nnn.yyy
    netmask 255.255.255.0
    default gw nnn.nnn.nnn.1
    DNS 8.8.8.8
    DNS 8.8.4.4

If a Linux laptop is being used consult Chapter 2 of Exploring BeagleBone by Derek Molloy, John 
Wiley & Sons, Inc, 2015.

Test network connection:

    ping 8.8.8.8
    ping www.google.com

If this does not work, retry the network connection section.

4. Disable the HDMI and HDMIN. The pins used by the HDMI are used for the PRU, so this need to 
be disabled.  From a terminal connected to the BeagleBone Black:

    cd  /boot/uboot
    cp uEnv.txt uEnv_backup.txt
    vi (or nano, etc.) uEnv.txt

Uncomment the line to disable HDMI and HDMIN and save. (BUT NOT HDMIN, EMMC line!)

    cat uEnv.txt 					(to make sure it is right)
    shutdown -r now		    		(to reboot the BeagleBone Black)

5.  Additional installs and updates (from a BeagleBone Black terminal):

    apt-get update
    apt-get install usbmount		(thumb drives will now automount after reboot)
    apt-get install ca-certificates
    git config --global http.sslVerify false
    shutdown -r now					(reboot)

Copy POPS subdirectory to /var/lib/cloud9/POPS

6.  Install Libconfig and other packages.

    cd /var/lib/cloud9/POPS
    ./Software_install_1 			(will reboot when finished, installs tree, ntp, minicom and 
									libconfig.)
    ./Software_install_2			(will reboot when finished, sets up rtc-ds1307 links and service,
									copies important files and makes the uSD mount point.)

The uSD card is used for the data storage and configuration file.  Make the Data folder, and copy 
over the configuration file.
    
    mkdir /media/uSD/Data
    cp /var/lib/cloud9/POPS/POPS_BBB.cfg /media/uSD

7.  Build the pops software.   From the /var/lib/cloud9/POPS directory execute the build with:
 
    './build'

If all is right, this will build both PRU binaries and the pops executable.


##Part 2.  BeagleBone Black PRU and DAQ Cape Setup

Attach the NOAA_BBB_PRU_DAQ cape (with the clock battery installed) to the BeagleBone black with the 
Beaglebone Black powered down and disconnected from the laptop.  Standoffs 46-pin will be required on 
P8 and P9 as the cape is designed to mount on both sides.  The microcomputer system (BeagleBone Black 
and NOAA cape) can be powered in two ways.  DO NOT USE THE 5V INPUT AFTER THIS POINT, as damage may 
occur to the cape or the BeagleBone Black.  After the system is powered up, the connection to the 
BeagleBone Black can be either through the USB or ethernet ports.

1.  Set the time.   The time is set by first updating the time from the internet and then writing 
that time to the realtime clock.  From '/var/lib/cloud9/POPS':

    ntpdate -b -s -u time.nist.gov		
    hwclock -w -f /dev/rtc1 			
    hwclock -r -f /dev/rtc1				

This sets the time on the BBB, then writes that time to the hardware clock. The hardware clock is 
read back to make sure that it is right. Now the time will sync to the internet whenever it is 
connected. The hardware clock would then have to be updated manually.

Execute:

    systemctl enable rtc-ds1307.service

Now the hardware clock service will starts on boot, but it can be started and stopped with:

    systemctl start rtc-ds1307.service
    systemctl stop rtc-ds1307.service

2.  Watchdog Timer.  The watchdog is used in the POPS code, and must be accessed once before it is 
available.  Access the watchdog with:

    cd /dev
    /dev ls -l watchdog
    /dev cat > watchdog
    
Then type something, followed by ctrl-c to exit the watchdog.

3.  Serial Communication.  Access the UARTS with minicom on the BBB and TaraTerm (or similar) on the 
laptop can be used to test the serial ports.  Port 1 is output as both RS232 and TTL.  TTL is usually
used for our balloon applications.  UART 2 is used in our application for full data.

    minicom -b 9600 -o -D /dev/ttyO1  		
    minicom -b 115200 -o -D /dev/ttyO2 

In this example UART1 is set to 9600 baud, and UART2 is set to 115200 baud.  
##Other Information:

1. C Compiler and Programming. To compile a C program:

        gcc {name}.c -o {name} -m -lrt -lconfig -lprussdrv -L. -liofunc

where the libraries are:

        math library 	-m		        <math.h>
        time 		    -lrt		    <time.h>
        libconfig 		-lconfig 		<libconfig.h>
        prussdrv 		-lprussdrv  	<prussdrv.h> and <pruss_intc_mapping.h>
        GPIO		    -L. -liofunc	"iolib.h"
			                    	    <linux/watchdog>

2. PRU Programming.  For the 5/14/2014 version of Debian on the BeagleBone Black the NOAA device 
tree files are copied to '/lib/firmware'.  This was done in one of the scrip '<Software_install_2>'.  
Each instruction takes at least 5 ns to execute.  To enable the scratch pad between PRUs, the -V3 
switch must be present in the compile command.  For instance:

    'pasm -V3 -b PRU0_ParData.p'

will produce a binary output that has the scratch pad enabled.

These are activated on boot when the rc.local file is executed.  This file enables the PRUs, loads 
the *.dtbo files, enables the max5802 on the i2c-1 bus (this is the two-channel analog out), mounts 
the uSD card, waits for the time to set and the system to be fully booted, and executes the pops 
program in the background.

rc.local file:

        modprobe uio_pruss
        echo cape-bone-iio > /sys/devices/bone_capemgr.9/slots
        echo NOAA-UART1 > /sys/devices/bone_capemgr.9/slots
        echo NOAA-UART2 > /sys/devices/bone_capemgr.9/slots
        echo NOAA-PRUA > /sys/devices/bonecapemgr.9/slots (all PRU pins)
        echo NOAA-GPIO > /sys/devices/bone_capemgr.9/slots

        echo max5802w 0x0F > /sys/class/i2c-adapter/i2c-1/new_device

        mount /dev/mmcblk0p1 /media/uSD

        sleep 20
        cd /var/lib/cloud9/POPS
        /var/lib/cloud9/POPS/pops &			(runs pops in the background at boot)

        exit 0

											(the file must end with exit 0)

Check that the slots loaded  with:

    cd /sys/devices/bone-capemgr.9
    cat slots 								(to make sure they all loaded right)

    cd /dev
    ls

The UARTS show up as /dev/ttyO1 and ttyO2 and the analog in as /dev/uio0..7.

PRU compile and decompile:

dtc compile:

    dtc -O dtb -o {name}-00A0.dtbo -b 0 -@ {name}.dts

dtc decompile:

    dtc -O dts -I dtb -o {name}.dts {name}-00A0.dtbo



