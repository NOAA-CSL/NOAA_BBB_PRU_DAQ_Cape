/*
// Filename: POPS_BBB.c
// Version: 3.0
// Cape Version: 20160612
// Version with point timing.
//
// Project: NOAA - POPS
// Author: Laurel Watts
// Contact: laurel.a.watts@noaa.gov
// Date	: 30 Sept 2017
//
// Description - This program controls the Printed Optical Particle Spectrometer 
// (POPS) instrument with Analog Out, reads the onboard ADC channels and
// converts them to actual readings, handles time
// and date stamps, and writes data to files on the BBB.
// The particle data is read on the PRUs at 4 MHz, and
// a baseline and data point determined and passed to the BBB.
// For each particle the peak above baseline, width (number or points),
// and time since last point in us is reported. UART and UDP communications are 
// implemented with two channels each.  The configuration file
// POPS_BBB.cfg is used to configure the instrument parameters.
//
// Revision History:
//  1.0	First public version
//  2.0 Add timing for particles. This changes the Read_PRU_Data, Write_Files 
//  and the PRU programs.
//  3.0 Merge various versions of the POPS program to function on balloons, UAV, 
//  Manta, WB57, etc with writing to either the uSD or usb drive. Config file 
//  must be on the uSD card.
//  4.0 Allows different flow rates to be set with a pressure trip.
*/
/*DISCLAIMER
----------------------------------------------
The United States Government makes no warranty, expressed or implied, as to the 
usefulness of this software and documentation for any purpose. The U.S. 
Government, its instrumentalities, officers, employees, and agents assume no 
responsibility (1) for the use of the software and documentation contained in 
this package, or (2) to provide technical support to users.

USE OF GOVERNMENT DATA, PRODUCTS, AND SOFTWARE
----------------------------------------------
The information on government servers are in the public domain, unless specifically 
annotated otherwise, and may be used without charge for any lawful purpose so long as you 
do not (1) claim it is your own (e.g., by claiming copyright for government information), 
(2) use it in a manner that implies an endorsement or affiliation with the government, or 
(3) modify its content and then present it as official government material. You also cannot 
present information of your own in a way that makes it appear to be official government 
information.

Use of the NOAA (National Oceanic and Atmospheric Administration) or ESRL (Earth System 
Research Laboratory) names and/or visual identifiers are protected under trademark law and 
may not be used without permission from NOAA. Use of these names and/or visual identifiers 
to identify unaltered NOAA content or links to NOAA websites are allowable uses. Permission 
is not required to display unaltered NOAA products which include the NOAA or ESRL names and/
or visual identifiers as part of the original product. Neither the names nor the visual 
identifiers may be used, however, in a manner that implies an endorsement or affiliation 
with NOAA.

Before using information obtained from government servers, special attention should be 
given to the date & time of the data and products being displayed. This information shall 
not be modified in content and then presented as official government material.
The user assumes the entire risk related to its use of this software.  NOAA is providing 
this software "as is," and NOAA disclaims any and all warranties, whether express or 
implied, including (without limitation) any implied warranties of merchantability or 
fitness for a particular purpose. In no event will NOAA be liable to you or to any third 
party for any direct, indirect, incidental, consequential, special or exemplary damages or 
lost profit resulting from any use or misuse of this software.

As required by 17 U.S.C. 403, third parties producing copyrighted works consisting 
predominantly of material obtained from the government must provide notice with such 
work(s) identifying the government material incorporated and stating that such material is 
not subject to copyright protection.

USE OF THIRD-PARTY SOFTWARE AND INFORMATION
----------------------------------------------
Third-party information are used under license from the individual third-party provider. 
This third-party information may contain trade names, trademarks, service marks, logos, 
domain names, and other distinctive brand features to identify the source of the 
information. This does not imply an endorsement of the third-party data/products or their 
provider by NOAA.  Please contact the third-party provider for information on your rights 
to further use these data/products.	

This software package may include non-government source code and may reference non-
government software libraries which are each subject to their own license restrictions. 
Any non-government code and software library references are listed below. If you choose 
to use this software package, including references to third-party libraries, for your own 
purposes, you must abide by their terms of use and license restrictions.

Non-government code and/or software libraries referenced by this software package:
 1. GNU (General Public Software) License for various packages that must be installed to 
 make the software compile and function. These include the Beaglebone Black Debian 
 operating system, gcc, libconfig, ntp, usbmount, iolib, minicom, libtool, perl, bison and 
 flex.
 2. TI (Texas Instruments, Inc) public software and documentation are used for compiling 
 the binary PRU code (PASM and Code Composer Studio).
 3. MAXUM software examples and documentation were used for programming the MAX5802 analog 
 out chip.
 4. Measurement Specialties, Inc software examples and documentation were used for 
 programming the MS5606 barometric pressure and temperature chip.
 5. Code examples were used as a starting point for several parts of the application from 
 Derek Molloy (2015) Exploring Beaglebone John Wiley & Sons, Inc.
 
This notice, in its entirety, shall be included in all copies or substantial portions of 
the Software.*/

//******************************************************************************
//
// Include files:
//
//******************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <float.h>
#include <errno.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <libconfig.h>
#include "iolib.h"
#include <linux/watchdog.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//******************************************************************************
//
// Definitions
//
//******************************************************************************

#define Billion  1000000000L                    // For time conversion
#define PRU_NUM0 0                              // PRU 0 high byte
#define PRU_NUM1 1                              // PRU 1 low byte and ctrl

// MAX5802 Constants
#define MAX5802_I2C_WADDR               (0x0F)  // A1 and A0 to gnd.
#define MAX5802_2500_EXT_REF            (0)     // 2.500V external reference for AO
#define MAX5802_2500_MV_REF             (5)     // 2.500V internal reference for AO
#define MAX5802_2048_MV_REF             (6)     // 2.048V internal reference for AO
#define MAX5802_4096_MV_REF             (7)     // 4.096V internal reference for AO
#define MAX5802_DEFAULT_RETURN          0x80    // Rest sel DAC channel(s) to value in RETURN reg

// MS5607 Constants
#define MS5607_CONV_DELAY_MS            (2.5)   // ms for T and P

// MS5607 Commands
#define MS5607_CMD_RESET                (0x1E)  // Reset P and T
#define MS5607_CMD_READ_ADC             (0x00)  // Read T&P data
#define MS5607_CMD_READ_PROM_BASE       (0xA0)  // Coeficient vase address xA0 to xAE
#define MS5607_CMD_CONVERT_D1_P         (0x44)  // Convert P 1024 resolution
#define MS5607_CMD_CONVERT_D2_T         (0x54)  // Convert T 1024 resolution

#define WATCHDOG "/dev/watchdog"                // For watchdog timer

typedef enum MAX5802_status
{
	MAX5802_status_ok = 0,
	MAX5802_status_i2c_transfer_error = 1,
	MAX5802_status_set_CODE_reg_error = 2,
	MAX5802_status_LOAD_DAC_error = 3
}MAX5802_status;

typedef enum ms5607_status
{
	ms5607_status_ok = 0,
	ms5607_status_transfer_error =1,
	ms5607_status_crc_error = 2,
	ms5607_status_heater_on_error = 3
}ms5607_status;

//******************************************************************************
//
// Function prototypes:
//
//******************************************************************************

void makeFileNames(void);
int Read_POPS_cfg(void);
void POPS_Output (void);
void Calc_WidthSTD(void);
void Write_Files(void);
void UpdatePumpTime(void);
void ReadAI(void);
int Open_Serial(int port, int baud);
void Close_Serial (int UART);
int Send_Serial(int UART, char msg[]);
int Read_Serial(int UART);
void getTimes(void);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);
void Calc_Baseline(void);
void Read_RawData(void);
void CalcBins(void);
void CompressBins(void);
void Implement_CMD(int source);
void Check_Stop(void);
int MIN (int a, int b);
int MAX (int a, int b);
MAX5802_status Set_AO(unsigned int n, double Read_V);
void CheckFlowStep(void);
MAX5802_status MAX5802_initialize(void);
MAX5802_status MAX5802_set_internal_reference( short unsigned int uchReferenceCommand);
MAX5802_status MAX5802_set_DAC_12_bit_value(short unsigned int uchChannelToSet,
    unsigned int unRegisterSetting);
MAX5802_status MAX5802_LOAD_DAC_from_CODE_register(short unsigned int uchChannelToSet);
MAX5802_status MAX5802_set_CODE_register(short unsigned int uchChannelToSet,
    unsigned int unRegisterSetting);
MAX5802_status MAX5802_set_default_DAC_settings(void);
MAX5802_status MAX5802_send_sw_clear(void);
MAX5802_status MAX5802_send_sw_reset(void);
void delay(int nStopValue);
void Initialize_GPIO(void);
void Send_TP_CMD(unsigned int CMD, unsigned int del);
unsigned int Read_TP_Reply(unsigned int del);
ms5607_status ms5607_reset(void);
ms5607_status ms5607_read_prom(void);
ms5607_status ms5607_read_TP(void);
unsigned char CRC4(long unsigned int ms5607_prom_coeffs[]);
int Make_Watchdog( int interval);
void InitPRU_Mem(void);
void Read_PRU_Data ( void );
int Open_Socket_Write(int i);
int Open_Socket_Read(int i);
int Open_Socket_Broadcast(int i);
int Write_UDP(int fd, int i, char msg[]);
int UDP_Read_Data(int fd, int i);
void Close_UDP_Socket(int UDPID);

//******************************************************************************
//
// Global variables:
//
//******************************************************************************

char gCode_Version[25] = {""};              // Software version installed
char gBBB_SN[20] = {""};                    // BBB Serial Number
char gPOPS_SN[20] = {""};                   // Instrument serial number
char gDaughter_Board[20] = {""};            // Daughter board part number

double gPumpLife;                           // Pump Life, hours saved to file
char gPumpFile[] = "/media/uSD/gPumpFile.txt";  // /media/uSD/gPumpFile

int gStop = 0;                              // Global Stop, 1 = Stop
                                            // PRU1 bit r31.t11 low=Stop
int gReboot = 0;                            // Global reboot, 1= reboot
int gIntStatus=1;                           // Status integer. 1=startup, 3=run
                                            // 17 = low flow, 32 = failed
char gStatus_Type[20];                      // iMet, UAV or iMet_ANG

unsigned int gRaw_Data[512];                // Raw data, max of 1024 pts
unsigned int gRaw_Read[512];                // Raw data read in
unsigned int gRawBL_Data[512];              // Raw BL Data pts

double gFullSec;                            // Timestamp with partial sec.
char gTimestamp[16], gDatestamp[9];         // String YYYYMMDDThhmmss
                                            // String YYYYMMDD
char gDispTime[25];                         // String "dd Mon YYYY hh:mm:ss"
struct timeval StartTime;                   // Start of 1 sec timer
int msfd, sfd;                              // File descriptors for ms and s timers

int gSkip_Save;                             // Skip n Between Save in peak files.
int gn_between = 0;                         // Counter for skip.
char gMedia[10];                            // Storage media for data, uSD or usb0
char gBaseAddr[25] = {""};                  // Base path for data files.
char gPOPS_BBB_cfg[20] = {""};              // POPS BBB configuration file
char gHK_File[50] = {""};                   // Path for Housekeeping File.
char gPeakFile[50] = {""};                  // Path for Peak File.
char gPeakFShort[25]={""};                  // Short peak file name for display
char gLogFile[50] = {""};                   // Path for Log File.
char gMessage[1000] = {""};                 // Message strings.
char gRawFile[50] = {""};                   // Raw data file. First n pts/sec
								
int gHist[200] = {0};                       // Histogram of particle sizes
unsigned int gPart_Num=0;                   // Particles per second
double gPartCon_num_cc;                     // Particle concentration, per cc
unsigned int i_head = 0, i_tail = 0;        // buffer positions for the data
unsigned int m_head = 0, m_tail = 0;        // buffer positions Shared Mem
unsigned int y_data[10000], nold = 0;       // data and carryover

short unsigned int gBLTH;                   // Baseline plus threshold
short unsigned int gBaseline;               // Baseline
short unsigned int gBL_Start;               // Starting value of the Baseline
double gSTD;                                // STD of Baseline
double gTH_Mult;                            // Threshold multiplier * STD
double gWidthSTD;                           // STD of peak width
double gAW;                                 // Average Width of peaks

int UART1, UART2;                           // Serial port references
unsigned char gCMD[512] = {""};             // Serial data revieved - UART1 (0-9)
char gStatus[4094] = {""};                  // Status to send.
char gFull[4094] = {""};                    // Full data to send
char gHK[4094] = {""};                      // Housekeeping data to save
char gRaw_Out[4094] = {""};                 // Raw Data out and save
char gAC[1024] = {""};                      // Aircraft Data

static void *pru1DRAM;                      // pointer for baseline data RAM
                                            // memory buffer
static unsigned int *pru1DRAM_int;          // values at the memory location
static void *pruSharedMem;                  // pointer to shared pru memory
static unsigned int *pruSharedMem_int;      // values of the shared memory
static void *pru0DRAM;                      // pointer to pru0 DRAM
static unsigned int *pru0DRAM_int;          // value of the memory location

unsigned int gMinPeakPts;                   // minimum points to make a peak
unsigned int gMaxPeakPts;                   // maximum points in one peak

long unsigned int ms5607_prom_coeffs[7];
double	T=0, P=0;                           // Pressure and temperature of P Chip

struct Raw {                                // Structures for Raw data to
    bool view;                              // Send to ground
    bool save;                              // Save to file
    int pts;                                // Data points to save per second
    int blpts;                              // Baseline points per second
    int ct;                                 // Count of points
} gRaw;

// AI Conversion Enum
enum con_type
{rawai,                                     //raw ai mv 0-1800
mV,                                         // mV 0-1800 direct reading
V,                                          // V 0-1.8 V
Therm,                                      // Thermocouple
BatV,                                       // Battery V
Flow,                                       // Flow using offset and divisor
RH_pct,                                     // Relative Humidity %
Pres};                                      // Pressure, mBar
float gFlow_Offset;                         // Flow offset
float gFlow_Divisor;                        // Flow divisor
struct AI {                                 // structure for the AI data
    char name[20];
    enum con_type conv;
    double  value;
};
struct gAI_Data {
    struct AI ai[7];
} gAI_Data;

struct AO {                                 // structure for the AO data
    char name[20];
    double set_V;
    unsigned int set_num;
    double maxV;
    double minV;
    double Ki;
    bool use_pid;
};
struct gAO_Data {
    struct AO ao[2];
} gAO_Data;

struct Serial_Port {                        // structure for the serial ports
    int port;
    int baud;
    char type[3];                           // S = Status, F = Full data
    bool use;
    bool open;
};
struct gSerial_Ports {
    struct Serial_Port serial_port[2];
} gSerial_Ports;

struct gBins {                              // structure for the bins
    unsigned int nbins;
    double logmin;
    double logmax;
} gBins;

struct Peaks {                              // structure for peak data
    unsigned int max;
    unsigned int w;
    unsigned int dt;
};
struct gData {
    struct Peaks peak[30000];
} gData;                                    // global data structure
unsigned int gArray_Size = 0;               // Size of the data array

int UDPStat, UDP0R, UDP1S, UDP1R, UDP2S, UDP2R, UDPAC; // UDP references

struct UDP {
    char IP[16];                            // IP address to connect to
    unsigned int port;                      // connection port
    char type[2];                           // S = Status, F = Full, A = Aircraft
    bool use;                               // use this connection?
    struct sockaddr_in myaddr;              // address info
    struct sockaddr_in remaddr;             // address info
} UDP;

struct gUDP {
    struct UDP udp[6];
} gUDP;

bool gFlowStepUse;                          // use plow step at low pressure
int gFlowSteps;                             // number of flow steps
double gStartFlowV;                         // starting high pressure flow
int gFlowFlag = 0;                          // 0 = inital, 1 = 1st pressure etc. 
struct step {                               // steps are the pressure where low
    double Press;                           //   flow starts
    double PumpV;
} step;
struct gFlowStep {
    struct step Step[10];
}gFlowStep;

//******************************************************************************
//
// Main program:
//
//******************************************************************************

void main()
{

    int i, j, ret, SerialTest, lp, first_call, UDPRec, UDPSend, ieq, eqct;
    struct timeval  LoopStart, LoopStop, LoopLeft, TimeNow;
    char * str;
    MAX5802_status nReturnValue;
    ms5607_status stat;
    gRaw.ct = 0;
    int WD_Timer;
    bool blink=true;

//*****************************
// Check to make sure program running as root.
//*****************************

    if(getuid()!=0)
    {
        printf("You must run this program as root. Exiting.\n");
        exit(EXIT_FAILURE);
    }
	
//******************************
// Read the configuration file
//******************************

    Read_POPS_cfg();
    gStartFlowV = gAO_Data.ao[1].set_V;

//******************************
//If the Status Type is Manta, set flow to 3.0 cc/s
//******************************
    if(strcmp(gStatus_Type, "Manta")==0) gAI_Data.ai[0].value = 3.0;

//******************************
// Initialize the time and Files
//******************************

    getTimes();

    makeFileNames();

    strcat(gMessage,gTimestamp);
    strcat(gMessage,"\tStarted program.\n");
    
//******************************
// Git initail Pump Time
//******************************
    FILE *fp;
    int num;
    char pl[20];
    if((fp =fopen(gPumpFile,"r" ))==NULL)
    {
        gPumpLife = 0.0;
        fp= fopen(gPumpFile, "w+");
    }
    else
    {
         num = fscanf(fp,"%s", pl);
         gPumpLife = atof(pl);
    }
    fclose(fp);    
//******************************
//Initialize the aircraft data if used
//******************************
    if(gUDP.udp[3].use) 
    {
        strcpy(gAC,"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n");
    }
    
 //*****************************
 // Initialize the AO - MAX5802
 //*****************************

    nReturnValue = MAX5802_initialize();

    delay(100);

    Set_AO(0,0.0);      //Set AO0 set value from cfg
    Set_AO(1,0.0);      //Set AO1 set value from cfg

    strcat(gMessage,"\tInitialized the MAX5802.\n");

//******************************
// Initialize the on-board P and T
//******************************

    Initialize_GPIO();

    stat = ms5607_reset();

// Read the PROM

    stat = ms5607_read_prom();

    strcat(gMessage,"\tInitialized the MS5607.\n");

//*****************************
// Open serial Ports
//*****************************

    if(gSerial_Ports.serial_port[0].use)
    {
        UART1 = Open_Serial(gSerial_Ports.serial_port[0].port,
            gSerial_Ports.serial_port[0].baud);
        if(gSerial_Ports.serial_port[0].open) strcat(gMessage,"\tUART1 opened.\n");
        else strcat(gMessage,"\tUART1 failed to open.\n");
    }
    if(gSerial_Ports.serial_port[1].use)
    {
        UART2 = Open_Serial(gSerial_Ports.serial_port[1].port,
            gSerial_Ports.serial_port[1].baud);
        if(gSerial_Ports.serial_port[1].open) strcat(gMessage,"\tUART2 opened.\n");
        else strcat(gMessage,"\tUART2 failed to open.\n");
    }

//*****************************
// Open udp Ports
//*****************************

    if(gUDP.udp[0].use) UDPStat = Open_Socket_Broadcast(0);         //Status out
    if(gUDP.udp[0].use) UDP0R = Open_Socket_Read(0);                //Status In
    if(gUDP.udp[1].use) UDP1S = Open_Socket_Write(1);               //Full Ir
    if(gUDP.udp[1].use) UDP1R = Open_Socket_Read(1);                //Full Ir
    if(gUDP.udp[2].use) UDP2S = Open_Socket_Write(2);               //Full Ku
    if(gUDP.udp[2].use) UDP2R = Open_Socket_Read(2);                //Full Ku
    if(gUDP.udp[3].use) UDPAC = Open_Socket_Read(3);                //GH AC in
    if(gUDP.udp[0].use || gUDP.udp[1].use) strcat(gMessage, "\tUDP sockets opened.\n");

//*****************************
// Make a watchdog timer with 5 second timeout
//*****************************

    WD_Timer=(Make_Watchdog(5));

//*****************************
// Initialize the PRUs
//*****************************

// Initialize structure used by prussdrv_pruintc_intc
// PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

// Allocate and initialize memory
    prussdrv_init ();
	
    ret = prussdrv_open (PRU_EVTOUT_0);
    if (ret)
    {
        strcat(gMessage,"PRU_EVTOUT_0 failed.\n");
        return;
    }
    ret = prussdrv_open (PRU_EVTOUT_1);
    if (ret)
    {
        strcat(gMessage,"PRU_EVTOUT_1 failed.\n");
        return;
    }

// Map PRU memory
    prussdrv_map_prumem (PRUSS0_PRU1_DATARAM, &pru1DRAM);
    pru1DRAM_int = (unsigned int *) pru1DRAM;
    prussdrv_map_prumem (PRUSS0_SHARED_DATARAM, &pruSharedMem);
    pruSharedMem_int = (unsigned int *) pruSharedMem;
    prussdrv_map_prumem (PRUSS0_PRU0_DATARAM, &pru0DRAM);
    pru0DRAM_int = (unsigned int *) pru0DRAM;

// Map PRU's interrupts
    prussdrv_pruintc_init(&pruss_intc_initdata);

//*****************************
// Program initialization. Initialize the BL and BLTH, and clear the
// BL and Point data memory.
//*****************************

// Initial value of the Baseline + Threshold
    pru1DRAM_int[256] = (gBL_Start+0x30);   // BLTH
    pru1DRAM_int[257] = gBL_Start;          // BL
    pru1DRAM_int[258] = 0x00010000;         // start of points
    pru1DRAM_int[259] = 0x00000000;         // stop

    for(i=0; i<256; i++)
    {
        pru1DRAM_int[i] = 0x75007550;
    }

    for (i=0; i<3072; i++)
    {
        pruSharedMem_int[i] = 0x00000000;
    }

    strcat(gMessage,"\tPRUs initialized.\n");

//*****************************
// Start PRU0 and PRU1
//*****************************

    prussdrv_exec_program (PRU_NUM1, "./PRU1_All.bin");
    prussdrv_exec_program (PRU_NUM0, "./PRU0_ParData.bin");
    strcat(gMessage,"\tPRU1 and PRU0 are now running.\n");

//*****************************
// Set up for first loop
//*****************************

    first_call = 1;
    usleep(50);
    Calc_Baseline();
    Check_Stop();                           // PRU1 r31.b11 P8.30

//*****************************
// Make timers for 1 sec loop
//*****************************

    j=0;

    getTimes();
    gettimeofday(&LoopStart, NULL);
    LoopStop.tv_sec = LoopStart.tv_sec + 1;
    LoopStop.tv_usec = LoopStart.tv_usec;

//*****************************
// Main Loop
//
// Maximum of 6.4 ms between Read_PRU_Data for 30,000 particle/second
// Loop within 1 ms
//*****************************

    while(!gStop)                           // Main Data Loop
    {
        if(!first_call)
        {
            CalcBins();
            // Histogram is compressed for some status outputs
            if (!strcmp(gStatus_Type, "iMet")) CompressBins();
            if (!strcmp(gStatus_Type, "iMet_TRM")) CompressBins();
            if (!strcmp(gStatus_Type, "WB57")) CompressBins();
            Read_RawData();
            POPS_Output();
            Calc_WidthSTD();
            Write_Files();
        }
        
        Read_PRU_Data();
        first_call = 0;
        getTimes();

        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
        
// Update the Pump Timer********************************************************
        UpdatePumpTime();
        
        Read_PRU_Data();
        first_call = 0;
        getTimes();

        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
        
// Analog IN *******************************************************************
        ReadAI();

        Calc_Baseline();

        if (gAI_Data.ai[0].value > 0.0) // flow rate cc/s
        {
            gPartCon_num_cc = gPart_Num/gAI_Data.ai[0].value;
        }
        else
        gPartCon_num_cc = 0.0;
    
        Read_PRU_Data();
        Check_Stop();
        if(gStop) goto Shutdown;

// P and T *********************************************************************
        ms5607_read_TP();

        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
        
// Flow as fcn of Pressure******************************************************
        if(gFlowStepUse) CheckFlowStep();

        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
        
// UART1 Status ****************************************************************
        if (gSerial_Ports.serial_port[0].use && !gSerial_Ports.serial_port[0].open)
        {
            Close_Serial(UART1);
            UART1 = Open_Serial(gSerial_Ports.serial_port[0].port,
                gSerial_Ports.serial_port[0].baud);
        }
        
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;

        if (gSerial_Ports.serial_port[0].use && gSerial_Ports.serial_port[0].open)
        {
            Send_Serial(UART1,gStatus);
        }

        usleep(50);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;

        if (gSerial_Ports.serial_port[0].use && gSerial_Ports.serial_port[0].open)
        {
            SerialTest = Read_Serial(UART1);
            if(strlen(gCMD) > 0 ) Implement_CMD(1);
        }
        usleep(50);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;

// UART2 Full data and Raw data ************************************************
        if (gSerial_Ports.serial_port[1].use && !gSerial_Ports.serial_port[1].open)
        {
            Close_Serial(UART2);
            UART2 = Open_Serial(gSerial_Ports.serial_port[1].port,
                gSerial_Ports.serial_port[1].baud);
        }

        if (gSerial_Ports.serial_port[1].use && gSerial_Ports.serial_port[1].open) \
            Send_Serial(UART2,gFull);
			
        usleep(10);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;

        if(gSerial_Ports.serial_port[1].use &&gRaw.view && gSerial_Ports.serial_port[1].open )
        {
            Send_Serial(UART2, gRaw_Out);
        }
        usleep(10);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
		
        if (gSerial_Ports.serial_port[1].use && gSerial_Ports.serial_port[1].open)
        {
            SerialTest = Read_Serial(UART2);
            if(strlen(gCMD) > 0) Implement_CMD(2);
        }
        
        usleep(10);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
		
//UDP **************************************************************************

        if(gUDP.udp[0].use) 
        {
            UDPSend = Write_UDP(UDPStat, 0, gStatus);
            UDPRec = UDP_Read_Data(UDP0R, 0);
            if(strlen(gCMD) > 0) Implement_CMD(1);
        }

        usleep(10);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;

        if(gUDP.udp[1].use)
        {
            UDPSend = Write_UDP(UDP1S, 1, gFull);
            usleep(10);
            UDPSend = Write_UDP(UDP1S, 1, gRaw_Out);
            usleep(10);
            UDPRec = UDP_Read_Data(UDP1R, 1);
            if(strlen(gCMD) > 0) Implement_CMD(2);
        }
        
        if(gUDP.udp[2].use)
        {
            UDPSend = Write_UDP(UDP2S, 2, gFull);
            usleep(10);
            UDPSend = Write_UDP(UDP2S, 2, gRaw_Out);
            usleep(10);
            UDPRec = UDP_Read_Data(UDP2R, 2);
            if(strlen(gCMD) > 0) Implement_CMD(2);
        }
        
        if(gUDP.udp[3].use)
        {
            UDPRec = UDP_Read_Data(UDPAC, 3);
            if(UDPRec > 0)
            {
                strncpy(gAC,gCMD+5,strlen(gCMD)+5); 
                strcpy(gCMD,"");
            }
        }
        
//******************************************************************************
        usleep(10);
        Read_PRU_Data();
        Calc_Baseline();
        Check_Stop();
        if(gStop) goto Shutdown;
        

// Loop on Process and Baseline until 1 sec or stop ****************************
        j=0;
        gettimeofday(&TimeNow, NULL);

        while( !(timeval_subtract(&LoopLeft, &LoopStop, &TimeNow )))
        {
            Read_PRU_Data();
            Calc_Baseline();
            Check_Stop();
            if(gStop) goto Shutdown;
            Read_RawData();
            j+=1;
            iolib_delay_ms(1);
            gettimeofday(&TimeNow, NULL);
        }

        Check_Stop();
        if(gStop) goto Shutdown;
        LoopStart = LoopStop;
        LoopStop.tv_sec +=1.;	// Add one sec to the loop timer for the next iteration
        if (blink) pin_high(8,13);
        else pin_low(8,13);
        blink = !blink;
        ioctl(WD_Timer, WDIOC_KEEPALIVE, NULL);	 // Wack the watchdog

    } // End of Main Loop

//************************************************
// Shutdown
//************************************************

Shutdown:

    gAO_Data.ao[0].set_V = 0.0;
    gAO_Data.ao[1].set_V = 0.0;
    Set_AO(0,0.0);      //Set AO0 to 0.0
    Set_AO(1,0.0);      //Set AO1 to 0.0

    pin_low(8,13);
    iolib_free();       //Clear GPIO

    Close_Serial(UART1);
    Close_Serial(UART2);

    Close_UDP_Socket(UDPStat);
    Close_UDP_Socket(UDP1S);
    Close_UDP_Socket(UDP1R);
    Close_UDP_Socket(UDP2S);
    Close_UDP_Socket(UDP2R);
    Close_UDP_Socket(UDPAC);
	
    close(WD_Timer);

    prussdrv_pru_disable(0);
    prussdrv_pru_disable(1);
    prussdrv_exit();

// Shutdown or reboot the BBB (Comment these out to prevent shutdown on stop.)
    if (gReboot == true) system ("shutdown -r now");
    else if (gStop == true) system ("shutdown -h now");

}
// End of main program**********************************************************

//******************************************************************************
//
//  FUNCTIONS
//
//******************************************************************************

//******************************************************************************
//
//  getTimes
//
//  Get the times and pass these with global variables.
//
//******************************************************************************

void getTimes(void)
{
    time_t seconds;
    struct tm *gmtNow;
    struct timeval tval, StartTime;

    time(&seconds);
    gmtNow = gmtime(&seconds);

    sprintf(gDatestamp, "%4d%02d%02d",(gmtNow->tm_year+1900), (gmtNow->tm_mon+1)\
    , (gmtNow->tm_mday));

    sprintf(gTimestamp, "%04d%02d%02dT%02d%02d%02d", (gmtNow->tm_year+1900), \
    (gmtNow->tm_mon+1), (gmtNow->tm_mday),(gmtNow->tm_hour),(gmtNow->tm_min), \
    (gmtNow->tm_sec));
    
    sprintf(gDispTime, "%04d-%02d-%02d %02d:%02d:%02d   ", (gmtNow->tm_year+1900), \
    (gmtNow->tm_mon+1), (gmtNow->tm_mday),(gmtNow->tm_hour),(gmtNow->tm_min), \
    (gmtNow->tm_sec));

    gettimeofday(&tval, NULL);
    gFullSec = ((double) tval.tv_sec) + ((double) tval.tv_usec)/1000000.0;

    StartTime= tval;

}

//******************************************************************************
//
//  Read_POPS_cfg
//
//  Read the configuration file.
//  Default values will be used for anything not read in.
//
//******************************************************************************

int Read_POPS_cfg()
{
    config_t cfg;
    config_setting_t *setting;
    const char *str;
    int test, i, count;
    int usei;

    config_init(&cfg);

//	  Read the file. If there is an error, log it and goto Default.
    if(! config_read_file(&cfg, "/media/uSD/POPS_BBB.cfg"))
    {
        strcat(gMessage,"Error opening the POPS_BBB_AC.cfg file.\n");
        goto Defaults;
// We actually want to continue and use default values rather than fail.
    }

    strcpy(gBaseAddr,"/media/");
//Get the Storage Media
    if(config_lookup_string(&cfg, "gMedia", &str))
    {
        strcpy(gMedia, str);
        strcat(gBaseAddr, str);
    }
    else
    {
        strcat(gMessage, "No 'Media' setting in configuration file.\n");
        strcpy(gMedia,"uSD");
        strcpy(gBaseAddr,"/media/uSD");
    }

//Get the BBB Serial Number/Name
    if(config_lookup_string(&cfg, "BBB_SN", &str))
    {
        strcpy(gBBB_SN, str);
    }
    else
    {
        strcat(gMessage, "No 'BBB serial number/name' setting in configuration file.\n");
        strcpy(gBBB_SN, "Snoopy#");
	}

//Get the POPS Serial Number
    if(config_lookup_string(&cfg, "POPS_SN", &str))
    {
        strcpy(gPOPS_SN, str);
    }
    else
    {
        strcat(gMessage, "No 'POPS serial number' setting in configuration file.\n");
        strcpy(gPOPS_SN, "POPS#");
    }

//Get the Daughter Board Serial Number
    if(config_lookup_string(&cfg, "Daughter_Board", &str))
    {
        strcpy(gDaughter_Board, str);
    }
    else
    {
        strcat(gMessage, "No 'Daughter Board Number' setting in configuration file.\n");
        strcpy(gDaughter_Board, "Rev2");
    }

//Get the Code Version
    if(config_lookup_string(&cfg, "Code_Version", &str))
    {
        strcpy(gCode_Version, str);
    }
    else
    {
        strcat(gMessage,"No 'Code Version' setting in configuration file.\n");
        strcpy(gCode_Version, "CodeVer_1.0");
    }

//Get flow settings
    setting = config_lookup(&cfg, "Setting.Flow");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *value = config_setting_get_elem(setting, i);
// Only output the settings if all of the expected fields are present.
            double offset, divisor;

            if(!(config_setting_lookup_float(value,"offset", &offset)
                && config_setting_lookup_float(value,"divisor", &divisor)))
            {
                gFlow_Offset = 0;
                gFlow_Divisor = 1;
                strcat(gMessage,"Using default flow offset and divisor.\n");
            }
            else
            {
                gFlow_Offset = offset;
                gFlow_Divisor = divisor;
            }
        }
    }

//Get the Bin settings
    setting = config_lookup(&cfg, "Setting.gBins");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *value = config_setting_get_elem(setting, i);
// Only output the settings if all of the expected fields are present.
            int nbins;
            double logmin, logmax;
            if(!(config_setting_lookup_int(value,"nbins", &nbins)
                && config_setting_lookup_float(value,"logmin", &logmin)
                && config_setting_lookup_float(value,"logmax", &logmax)))
            {
                gBins.nbins = 8;
                gBins.logmin = 1.4;
                gBins.logmax = 4.817;
                strcat(gMessage,"Using default nbins, logmin and logmax.\n");
            }
            else
            {
                gBins.nbins = nbins;
                gBins.logmin = logmin;
                gBins.logmax = logmax;
            }
        }
    }

//Get the AI settings
    setting = config_lookup(&cfg, "Setting.AI");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
        config_setting_t *AI = config_setting_get_elem(setting, i);
// Only output the record if all of the expected fields are present.
        const char *name, *sconv;
        char *str1;
        if(!(config_setting_lookup_string(AI, "name", &name)
            && config_setting_lookup_string(AI, "conv", &sconv)))
            {
                sprintf(gAI_Data.ai[i].name,"AI[%d]",i);
                gAI_Data.ai[i].conv = V;		// default is V
                strcat(gMessage,"Using default AI setup.\n");
            }
            else
            {
                strcpy(gAI_Data.ai[i].name, name);
                gAI_Data.ai[i].conv=rawai;			// default is V
                if((strcmp(sconv,"rawai")) ==0) gAI_Data.ai[i].conv=rawai;
                if((strcmp(sconv,"mV")) ==0) gAI_Data.ai[i].conv=mV;
                if((strcmp(sconv,"Therm"))==0) gAI_Data.ai[i].conv=Therm;
                if((strcmp(sconv,"BatV"))==0) gAI_Data.ai[i].conv=BatV;
                if((strcmp(sconv,"Flow"))==0) gAI_Data.ai[i].conv=Flow;
                if((strcmp(sconv,"RH_pct"))==0) gAI_Data.ai[i].conv=RH_pct;
                if((strcmp(sconv,"Pres"))==0) gAI_Data.ai[i].conv=Pres;
            }
        }
    }

//Get the AO settings
    setting = config_lookup(&cfg, "Setting.AO");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; i++)
        {
            config_setting_t *AO = config_setting_get_elem(setting, i);
            const char *name;
            double set_V, maxV, minV, Ki;
            int use_pid;

            if(!(config_setting_lookup_string(AO, "name", &name)
                && config_setting_lookup_float(AO, "set_V", &set_V)
                && config_setting_lookup_float(AO, "maxV", &maxV)
                && config_setting_lookup_float(AO, "minV", &minV)
                && config_setting_lookup_float(AO, "Ki", &Ki)
                && config_setting_lookup_bool(AO, "use_pid", &use_pid)))

                strcpy(gAO_Data.ao[i].name, name);
                gAO_Data.ao[i].set_V = set_V;
                gAO_Data.ao[i].maxV = maxV;
                gAO_Data.ao[i].minV = minV;
                gAO_Data.ao[i].Ki = Ki;
                gAO_Data.ao[i].use_pid = use_pid;

// no defaults if they are not set up.
        }
    }
// Get the Flow Step settings
    if(config_lookup_bool(&cfg, "Setting.FlowStepUse", &usei))
    {
        gFlowStepUse = usei;
    }
    else
    {
        gFlowStepUse = true;
    }

// Get the pressure trips that go with the flow steps
    setting = config_lookup(&cfg,"Setting.FlowSteps");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *FS = config_setting_get_elem(setting, i);
            double Press, PumpV;
            
            if (!(config_setting_lookup_float(FS,"Press",&Press)
                && config_setting_lookup_float(FS,"PumpV",&PumpV)))
            {
                gFlowStep.Step[i].Press = 0;
                gFlowStep.Step[i].PumpV =  gAO_Data.ao[1].set_V ;
            }
            else
            {
                gFlowStep.Step[i].Press = Press;
                gFlowStep.Step[i].PumpV = PumpV;
            }
        }
        gFlowSteps = i+1;
    }
    
//Get the Serial settings: Port 1 is Status, Port 2 is full data.
    setting = config_lookup(&cfg, "Setting.Serial_Port");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *Ser = config_setting_get_elem(setting, i);
            const char *type;
            int use;
            int port, baud;

            if(!(config_setting_lookup_int(Ser, "port", &port)
                && config_setting_lookup_int(Ser, "baud", &baud)
                && config_setting_lookup_string(Ser, "type", &type)
                && config_setting_lookup_bool(Ser, "use", &use)))
            {
                gSerial_Ports.serial_port[i].port = i+1;
                gSerial_Ports.serial_port[i].baud = 9600;
                strcpy(gSerial_Ports.serial_port[i].type, "S");
                gSerial_Ports.serial_port[i].use = true;
            }
            else
            {
                gSerial_Ports.serial_port[i].port = port;
                gSerial_Ports.serial_port[i].baud = baud;
                strcpy(gSerial_Ports.serial_port[i].type, type);
                gSerial_Ports.serial_port[i].use = use;
            }
        }
    }

//Get Skip setting
    setting = config_lookup(&cfg, "Setting.Skip");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        int Skip_save;
        for (i = 0; i< count; ++i)
        {
            config_setting_t *value = config_setting_get_elem(setting, i);
            if(!(config_setting_lookup_int(value,"Skip_Save", &Skip_save)))
            {
                gSkip_Save = 0;
                strcat(gMessage,"Using default Skip_Save of 0.\n");
            }
            else
            {
                gSkip_Save = Skip_save;
            }
        }
    }

//Get Peak settings
    setting = config_lookup(&cfg, "Setting.Peak");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *value = config_setting_get_elem(setting, i);
// Only output the settings if all of the expected fields are present.
            int MinPeakPts, MaxPeakPts;
            if(!(config_setting_lookup_int(value,"MinPeakPts", &MinPeakPts)
                && config_setting_lookup_int(value,"MaxPeakPts", &MaxPeakPts)))
            {
                gMinPeakPts = 5;
                gMaxPeakPts = 255;
                strcat(gMessage,"Using default min and max peak points.\n");
            }
            else
            {
                gMinPeakPts = MinPeakPts;
                gMaxPeakPts = MaxPeakPts;
            }
        }
    }

//Get Baseline settings
    setting = config_lookup(&cfg, "Setting.Baseline");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *value = config_setting_get_elem(setting, i);
// Only output the settings if all of the expected fields are present.
            int BL_Start;
            double TH_Mult;
            if(!(config_setting_lookup_int(value,"BL_Start", &BL_Start)
                && config_setting_lookup_float(value,"TH_Mult", &TH_Mult)))
            {
                gBL_Start = 2300;
                gTH_Mult = 2.0;
                strcat(gMessage,"Using default min and max peak points.\n");
            }
            else
            {
                gBL_Start = BL_Start;
                gTH_Mult = TH_Mult;
            }
        }
    }

//Get Status setting
    setting = config_lookup(&cfg, "Setting.Status");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i=0; i < count; ++i )
        {
            config_setting_t *value = config_setting_get_elem(setting, i);
            const char *status_type;
            if(!(config_setting_lookup_string(value,"Status_Type", &status_type)))
            {
                strcpy(gStatus_Type, "UAV");
                strcat(gMessage,"Using default Status_Type of UAV.\n");
            }
            else
            {
                strcpy(gStatus_Type, status_type);
            }
        }
    }

    if(!strcmp(gStatus_Type, "iMet")) gBins.nbins = 16;		// must be 16 for this to work

//Get Raw Data settings
    setting = config_lookup(&cfg, "Setting.Raw");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *raw = config_setting_get_elem(setting, i);
            int view;
            int save;
            int pts, blpts, ct;
// Only output the settings if all of the expected fields are present.
            if(!(config_setting_lookup_bool(raw,"view", &view)
                && config_setting_lookup_bool(raw,"save", &save)
                && config_setting_lookup_int(raw,"pts", &pts)
                && config_setting_lookup_int(raw,"blpts", &blpts)
                && config_setting_lookup_int(raw,"ct", &ct)))
            {
                gRaw.view = true;
                gRaw.save = true;
                gRaw.pts = 512;
                gRaw.blpts = 512;
                gRaw.ct = 0;
                strcat(gMessage,"Using default raw points points.\n");
            }
            else
            {
                gRaw.view = view;
                gRaw.save = save;
                gRaw.pts = pts;
                gRaw.blpts = blpts;
                gRaw.ct = ct;
            }
        }
    }

//Get the UDP settings
    setting = config_lookup(&cfg, "Setting.UDP");
    if(setting != NULL)
    {
        count = config_setting_length(setting);
        for(i = 0; i < count; ++i)
        {
            config_setting_t *net = config_setting_get_elem(setting, i);
            const char *type;
            const char *IP;
            int use;
            int port;

            if(!(config_setting_lookup_string(net, "IP", &IP)
                && config_setting_lookup_int(net, "port", &port)
                && config_setting_lookup_string(net, "type", &type)
                && config_setting_lookup_bool(net, "use", &use)))
            {
                strcpy(gUDP.udp[i].IP, "10.1.1.255");
                gUDP.udp[i].port = 5100;
                strcpy(gUDP.udp[i].type, "S");
                gUDP.udp[i].use = true;
            }
            else
            {
                strcpy(gUDP.udp[i].IP, IP);
                gUDP.udp[i].port = port;
                strcpy(gUDP.udp[i].type, type);
                gUDP.udp[i].use = use;
            }
        }
    }

    config_destroy(&cfg);

    return(0);

Defaults:
    strcat(gMessage, "No 'address' setting in configuration file.\n");
    strcpy(gMedia,"uSD");
    strcpy(gBaseAddr,"/media/uSD/");
    strcat(gMessage, "No 'POPS serial number' setting in configuration file.\n");
    strcpy(gPOPS_SN, "POPS#");
    strcat(gMessage, "No 'Daughter Board Number' setting in configuration file.\n");
    strcpy(gDaughter_Board, "Rev2");
    strcat(gMessage, "No 'Daughter Board Number' setting in configuration file.\n");
    strcpy(gDaughter_Board, "Rev2");
    strcat(gMessage,"No 'Code Version' setting in configuration file.\n");
    strcpy(gCode_Version, "CodeVer_3.0");
    gFlow_Offset = 0;
    gFlow_Divisor = 1;
    strcat(gMessage,"Using default flow offset and divisor.\n");
    gBins.nbins = 8;
    gBins.logmin = 1.4;
    gBins.logmax = 4.817;
    strcat(gMessage,"Using default nbins, logmin and logmax.\n");
    for(i=0;i<7;i++)
    {
        sprintf(gAI_Data.ai[i].name,"AI[%d]",i);
        gAI_Data.ai[i].conv=V;	// default is V
        strcat(gMessage,"Using default AI setup.\n");
    }
    gSerial_Ports.serial_port[i].port = 1;
    gSerial_Ports.serial_port[i].baud = 9600;
    strcpy(gSerial_Ports.serial_port[i].type, "S");
    gSerial_Ports.serial_port[i].use = true;
    gSerial_Ports.serial_port[i].port = 2;
    gSerial_Ports.serial_port[i].baud = 115200;
    strcpy(gSerial_Ports.serial_port[i].type, "F");
    gSerial_Ports.serial_port[i].use = true;
    gSkip_Save = 0;
    strcat(gMessage,"Using default Skip_Save of 0.\n");
    gMinPeakPts = 5;
    gMaxPeakPts = 255;
    strcat(gMessage,"Using default min and max peak points.\n");
    gBL_Start = 2000;
    gTH_Mult = 2.;
    strcat(gMessage,"Using default min and max peak points.\n");
    strcpy(gStatus_Type, "UAV");
    strcat(gMessage,"Using default Status_Type of UAV.\n");
    gRaw.view = true;
    gRaw.save = false;
    gRaw.pts = 256;
    gRaw.blpts = 256;
    gRaw.ct = 0;
    strcat(gMessage,"Using default raw points points.\n");

    return(0);
}

//******************************************************************************
//
//  makeFileNames
//
//  Make the filenames for saving data and the message log.
//
//******************************************************************************

void makeFileNames(void)
{

    FILE *fp, *source, *target;
    char FileAddr[100] = {""};              // Start file address
    char FileVersion[100] = {""};           // file for holding version information
    char ver[] = "x000";                    // Default Version
    int n=0, i=0, j;                        // version number as int
    char num[] = "000";                     // used to conver to integer
    char blk[] = {""};                      // blank string for initialization
    char HK_Header[2000] ={""};             // HK header, comma del
    char ch;                                // for file copy
    char binNames[1000] = {""};             // Histogram Bin names (max = 883)
    char str[] = {""};                      // string dummy

    char config[] ="/media/uSD/POPS_BBB.cfg";
    char cp_cmd[] = {""};


    strcat(FileAddr, gBaseAddr);
    strcat(FileAddr, "/Data/F");
    strcat(FileAddr, gDatestamp);
    mkdir(FileAddr,0666);

// Save the configuration file to today's directory.
    strcat(cp_cmd,"cp /media/uSD/POPS_BBB.cfg ");
    strcat(cp_cmd, FileAddr);
    system (cp_cmd);

    strcpy(gHK_File, blk);                  // initialize the files to blank
    strcpy(gPeakFile, blk);
    strcpy(gLogFile, blk);
    strcpy(gPOPS_BBB_cfg, blk);
    strcpy(gRawFile, blk);
    strcpy(gPeakFShort, blk);

    strcat(FileAddr, "/");
    strcat(FileVersion, FileAddr);
    strcat(FileVersion, "Version");

    fp = fopen(FileVersion, "a+");
    fseek(fp, 0, SEEK_SET);
    
    while(fscanf(fp, "%s", ver) == 1) //1 = read a ver
    {
        ;
    }
    if (feof(fp))
	{
        num[0] = ver[1];
        num[1] = ver[2];
        num[2] = ver[3];
        num[3] = '\0';
        sscanf(num, "%d", &n);
        n += 1;
        sprintf(ver, "x%03d", n);
        fprintf(fp,"%s\n", ver);
    }
    fclose (fp);

    strcat(gHK_File, FileAddr);
    strcat(gHK_File, "HK_");
    strcat(gHK_File, gDatestamp);
    strcat(gHK_File, ver);
    strcat(gHK_File, ".csv");
    
    strcpy(binNames,blk);
    for (i=0;i<gBins.nbins;i++)
    {
        strcat(binNames,",b");
        sprintf(str,"%d",i);
        strcat(binNames,str);
    }

    fp = fopen(gHK_File, "w");
    strcpy(HK_Header,"DateTime,Status,PartCt,PartCon,BL,BLTH,STD,P,TofP,PumpLife_hrs,WidthSTD,AveWidth");
    for (i=0; i<7; i++)
    {
        strcat(HK_Header, ", ");
        strcat(HK_Header, gAI_Data.ai[i].name);
    }
    for (i=0; i<2; i++)
    {
        strcat(HK_Header, ", ");
        strcat(HK_Header, gAO_Data.ao[i].name);
    }
    strcat(HK_Header,",BL_Start,TH_Mult,nbins,logmin,logmax,Skip_Save,");
    strcat(HK_Header,"MinPeakPts,MaxPeakPts,RawPts,");
    if(gUDP.udp[3].use)     // add the aircraft header if data is used
    {
    strcat(HK_Header,"ACDateTime,Lat,Lon,GPS_MSL_Alt,WGS_84_Alt,Press_Alt,Radar_Alt,");
    strcat(HK_Header,"Grnd_Spd,True_Airspd,Ind_Airspd,Mach,Vert_Vel,True_Hdg,Track,Drift,Pitch,");
    strcat(HK_Header,"Roll,SideSlip,AngleOfAttack,Ambient_T,DewPoint,Total_T,");
    strcat(HK_Header,"Static_P,Dynamic_P,Cabin_P,WindSpd,WindDir,VertWindSpd,");
    strcat(HK_Header,"SolarZenith,SunElevAC,SunAzGrnd,SunAz_AC");
    }
    strcat(HK_Header,binNames);
    strcat(HK_Header,"\r\n");
    fprintf( fp, HK_Header);

    fclose (fp);

    strcat(gPeakFile, FileAddr);
    strcat(gPeakFile, "Peak_");
    strcat(gPeakFShort,"Peak_");
    strcat(gPeakFile, gDatestamp);
    strcat(gPeakFShort, gDatestamp);
    strcat(gPeakFile, ver);
    strcat(gPeakFile, ".b");
    strcat(gPeakFShort, ver);
    strcat(gPeakFShort, ".b");

    strcat(gLogFile, FileAddr);
    strcat(gLogFile, "Log_");
    strcat(gLogFile, gDatestamp);
    strcat(gLogFile, ver);
    strcat(gLogFile, ".txt");

    strcat(gRawFile, FileAddr);
    strcat(gRawFile, "RawPK_");
    strcat(gRawFile, gDatestamp);
    strcat(gRawFile, ver);
    strcat(gRawFile, ".b");

}

//******************************************************************************
//
//  UpdatePumpTime
//
//  Increment the pump time and save as hours.
//
//******************************************************************************

void UpdatePumpTime (void)
{
    FILE *fp;
    int num;
    fp =fopen(gPumpFile,"r+" );

    gPumpLife+= 0.000277778;          //increment the pump life
    
    fseek(fp,0,SEEK_SET);
    fprintf(fp,"%4.8f\r\n",gPumpLife);
    fclose(fp);
}

//******************************************************************************
//
//  ReadAI
//
//  Read Analog in using files and convert the reading to engineering units.
//
//******************************************************************************

void ReadAI( void )
{
    FILE *aifile0,*aifile1,*aifile2,*aifile3,*aifile4,*aifile5,*aifile6;
    static int ain[7];
    int i, start;
    double A, B, C, D, int1, int2, int3;
	
    A = 1.13206975726444E-03;
    B = 2.33431080526447E-04;
    C = 9.43470416157594E-08;
    D = -2.63722384777803E-11;
    
    if (strcmp(gStatus_Type, "Manta")!=0)  // Don't update POPS Flow if Manta
    {
    aifile0 = fopen( "/sys/devices/ocp.3/helper.12/AIN0","r");
    fseek(aifile0,0,SEEK_SET);
    fscanf(aifile0,"%d",&ain[0]);
    fclose(aifile0);
    start = 0;
    }
    else start = 1;

    aifile1 = fopen("/sys/devices/ocp.3/helper.12/AIN1", "r");
    fseek(aifile1,0,SEEK_SET);
    fscanf(aifile1,"%d",&ain[1]);
    fclose(aifile1);

    aifile2 = fopen("/sys/devices/ocp.3/helper.12/AIN2", "r");
    fseek(aifile2,0,SEEK_SET);
    fscanf(aifile2,"%d",&ain[2]);
    fclose(aifile2);

    aifile3 = fopen("/sys/devices/ocp.3/helper.12/AIN3", "r");
    fseek(aifile3,0,SEEK_SET);
    fscanf(aifile3,"%d",&ain[3]);
    fclose(aifile3);

    aifile4 = fopen("/sys/devices/ocp.3/helper.12/AIN4", "r");
    fseek(aifile4,0,SEEK_SET);
    fscanf(aifile4,"%d",&ain[4]);
    fclose(aifile4);

    aifile5 = fopen("/sys/devices/ocp.3/helper.12/AIN5", "r");
    fseek(aifile5,0,SEEK_SET);
    fscanf(aifile5,"%d",&ain[5]);
    fclose(aifile5);

    aifile6 = fopen("/sys/devices/ocp.3/helper.12/AIN6", "r");
    fseek(aifile6,0,SEEK_SET);
    fscanf(aifile6,"%d",&ain[6]);
    fclose(aifile6);


    for (i = start; i < 7; i++)
    {
        if (ain[i]>=1780) pin_high(8,14);
        switch(gAI_Data.ai[i].conv)
        {
            case rawai:                     //raw mv (0-1800)
                gAI_Data.ai[i].value=ain[i];
                break;
            case mV:                        //mVoltage (0 - 5036.17)
                gAI_Data.ai[i].value=ain[i]*2.79787;
                break;
            case V:                         //Voltage (0 - 5.03617)
                gAI_Data.ai[i].value=ain[i]*2.79787/1000.;
                break;
            case Pres:                      //Pressure
                gAI_Data.ai[i].value=(ain[i]*2.79787/1000.)*1013.25;
                break;
            case BatV:                      //Battery voltage
                gAI_Data.ai[i].value=(ain[i]*0.01117699115);
                break;
            case Flow:                      //Flow cc/s
                gAI_Data.ai[i].value=((ain[i]*2.79787/1000.)-gFlow_Offset)/gFlow_Divisor;
                break;
            case Therm:                     //Thermistor T
            {
                double LR = log(1.24*((3601.6/ain[i])-1)*1000.);
                int1 = B*LR;
                int2 = C*LR*LR*LR;
                int3 = D*LR*LR*LR*LR*LR;

                gAI_Data.ai[i].value=(double)(1./(A + int1 + int2 + int3))-273.15;
            }
                break;
            case RH_pct:                    //Relative Humidity %
                gAI_Data.ai[i].value=(ain[i]*2.79787/1000.)*100./5.03617;
                break;
            default:                        //Voltage (0 - 5.03617)
                gAI_Data.ai[i].value=(ain[i]*2.79787/1000.);
        }
    }
}

//******************************************************************************
//
//  POPS_Output
//
//  Gather the Status and Full Data to send to send to the display or ground
//
//******************************************************************************

void POPS_Output (void)
{
    char inst[]={"POPS,"};
    char full[] = {""};
    char str[4094] = {""};
    char fullstr[4094]={""};
    char *pch;
    unsigned int i, Bins, temp_out;
    int nantest;

    Bins=gBins.nbins;
    if(!strcmp(gStatus_Type,"iMet")) Bins=8;
    if(!strcmp(gStatus_Type,"iMet_TRM")) Bins=8;
    if(!strcmp(gStatus_Type,"WB57")) Bins=8;

// Full dat and HK

    strcpy(gFull, inst);                    //Header
	
    strcat(gFull, gStatus_Type);            //Status Type
    strcat(gFull,",");
	
    strcat(gFull, gPeakFile);               //Peak File
    strcat(gFull,",");
	
    strcat(gFull, gTimestamp);              //Date_Time
    sprintf(str,"%.3f",gFullSec);
    strcpy(gHK, str);
	
    sprintf(str,",%d,%u,%.2f",gIntStatus,gPart_Num,gPartCon_num_cc);    //Status
    strcat(fullstr, str);
	
    sprintf(str, ",%u,%u,%.2f",gBaseline,gBLTH,gSTD);   //Baseline info
    strcat(fullstr, str);
	
    sprintf(str,",%.2f,%.2f", P, T);        //P and T
    strcat(fullstr,str);
    
    sprintf(str, ",%.2f,%.2f,%.2f",gPumpLife, gWidthSTD, gAW);     //Pump and widthSTD
    strcat(fullstr, str);
	
    for (i=0; i<7; i++)                     //AI
    {
        sprintf(str,",%.2f" ,gAI_Data.ai[i].value);
        strcat(fullstr,str);
    }
    
    for (i=0; i<2; i++)                     //AO
    {
        sprintf(str,",%.2f", gAO_Data.ao[i].set_V);
        strcat(fullstr,str);
    }
    
    // control values
    sprintf(str, ",%u,%.1f,%u,%.2f,%.2f",gBL_Start,gTH_Mult,gBins.nbins,
        gBins.logmin,gBins.logmax);
    strcat(fullstr, str);
	
    sprintf(str, ",%i,%u,%u,%i",gSkip_Save,gMinPeakPts,gMaxPeakPts,gRaw.pts);
    strcat(fullstr, str);

    strcat(gFull,fullstr);
    strcat(gHK, fullstr);
    strcat(gHK, ",");
    
    if (gAC[0]!='\0')  // append aircraft data without \n if available
    {
        pch = strtok(gAC,"\n");
        strcat(gHK,pch);
        strcat(gHK,",");
    }
    
    for (i=0; i<gBins.nbins; i++)           //Histogram
    {	
        sprintf(str, ",%d", gHist[i]);
        strcat(gFull, str);
        strcat(gHK, str);
    }
    sprintf(str,"\r\n");
  
    strcat(gFull, str);                     // add cr lf
    strcat(gHK, str);                     
    
// Raw Data
    if(gRaw.save | gRaw.view)
    {
        strcpy(gRaw_Out,"RawData" );        //Headers

        for(i=0; i<gRaw.pts; i++)
        {
            sprintf(str,",%x",gRaw_Data[i]);
            strcat(gRaw_Out,str);
        }
    }
	
// Status Packet
    if(!strcmp(gStatus_Type, "iMet"))
    {
        strcpy(gStatus,"xdata=3801");
        sprintf(str, "%04X",(int)gPartCon_num_cc);	//Particle Concentration
        strcat(gStatus,str);
        if ((gAI_Data.ai[0].value >= 0.) && (gAI_Data.ai[0].value < 10.))
        {
            sprintf(str,"%02X", (int)(10*gAI_Data.ai[0].value));	//POPS Flow
        }
        else sprintf(str,"%02X", 0);
        strcat(gStatus,str);

        temp_out = (int)(100+gAI_Data.ai[5].value);	
            //  Temperature selection:
            //      T = T of P ambient chip
            //      gAI_Data.ai[2].value = LDTemp
            //      gAI_Data.ai[4].value = LD_Mon
            //      gAI_Data.ai[5].value = external thermistor
        nantest =isnan(temp_out);
        if (nantest != 0) temp_out = 0.;    // remove NaN math problem
        sprintf(str,"%02X",temp_out);

        strcat(gStatus,str);

        for (i=0; i<Bins; i++)              //Histogram
        {
            sprintf(str, "%04X", gHist[i]);
            strcat(gStatus, str);
        }
            sprintf(str,"\r\n");            //add cr lf
            strcat(gStatus, str);
	}
		
    else if(!strcmp(gStatus_Type, "iMet_TRM"))
    {
        strcpy(gStatus,"xdata=3801");

        if ((gAI_Data.ai[0].value >= 0.) && (gAI_Data.ai[0].value < 10.))
        {
            sprintf(str,"%02X", (int)(10*gAI_Data.ai[0].value));	//POPS Flow
        }
        else sprintf(str,"%02X", 0);
        strcat(gStatus,str);

        for (i=0; i<Bins; i++)          //Histogram
        {
            sprintf(str, "%04X", gHist[i]);
            strcat(gStatus, str);
        }
            sprintf(str,"\r\n");            //add cr lf
            strcat(gStatus, str);
    }
		
    else if(!strcmp(gStatus_Type, "iMet_ANG"))
    {
        strcpy(gStatus,"xdata=3801");
        sprintf(str, "%04X", gPart_Num);                //Particle Number
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)gPartCon_num_cc);      //Particle Concentration
        strcat(gStatus,str);
        sprintf(str, "%04X",gBaseline);                 //Baseline
        strcat(gStatus,str);
        sprintf(str, "%04X",gBLTH);                     //Baseline + Threshold
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)gSTD);                 //Baseline STD
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(P*10));               //Pressure
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(100+T));              //T of P
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(10*gAI_Data.ai[0].value));    //POPS Flow
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(gAI_Data.ai[1].value));       //PumpFB
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(100+gAI_Data.ai[2].value));   //LDTemp
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(10*gAI_Data.ai[3].value));    //LaserFB
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(10*gAI_Data.ai[4].value));    //LD_Mon
        strcat(gStatus,str);
        nantest =isnan(gAI_Data.ai[5].value);
        if (nantest != 0) sprintf(str, "%04x",0);       //remove NaN math problem
        else sprintf(str, "%04X",(int)(100+gAI_Data.ai[5].value));  //Temp
        strcat(gStatus,str);
        sprintf(str, "%04X",(int)(10*gAI_Data.ai[6].value));    //BatV
        strcat(gStatus,str);
			
        for (i=0; i<Bins; i++)                              //Histogram
        {
            sprintf(str, "%04X", gHist[i]);
            strcat(gStatus, str);
        }
        sprintf(str,"\r\n");                                //add cr lf
        strcat(gStatus, str);
    }	
		
    else if (!strcmp(gStatus_Type, "UAV"))
    {
        strcpy(gStatus, inst );		
        strcat(gStatus, gTimestamp);
        sprintf(str,",%u,%.2f",gPart_Num,gPartCon_num_cc);  //Status
        strcat(gStatus, str);
        sprintf(str, ",%u,%.2f",gBaseline,gSTD);            //Baseline info
        strcat(gStatus, str);
        sprintf(str,",%.2f", P);                            //P 
        strcat(gStatus,str);
        sprintf(str,",%.2f",gAI_Data.ai[0].value);          //AI Flow
        strcat(gStatus,str);
        sprintf(str,",%.2f",gAI_Data.ai[2].value);          //AI LDTemp
        strcat(gStatus,str);
        sprintf(str,",%.2f",gAI_Data.ai[4].value);          //AI LD_Mon
        strcat(gStatus,str);
        sprintf(str,",%.2f",gAI_Data.ai[5].value);          //AI Temp
        strcat(gStatus,str);
        // for (i=0; i<Bins; i++)                              //Histogram
        // {	
        //     sprintf(str, ",%d", gHist[i]);
        //     strcat(gStatus, str);
        // }
        sprintf(str,"\r\n");                                //add cr lf
        strcat(gStatus, str);
    }	
	    else if (!strcmp(gStatus_Type, "Manta"))
    {
        strcpy(gStatus, inst );		
        strcat(gStatus, gTimestamp);
        sprintf(str,",%u,%u,%.2f",gIntStatus,gPart_Num,gPartCon_num_cc);  //Status
        strcat(gStatus, str);
        sprintf(str,",%.2f",gAI_Data.ai[0].value);          //POPS Flow (from Manta)
        strcat(gStatus,str);
        sprintf(str, ",%u,%.2f",gBaseline,gSTD);            //Baseline info
        strcat(gStatus, str);

        
        for (i=0; i<Bins; i++)                              //Histogram
        {	
            sprintf(str, ",%d", gHist[i]);
            strcat(gStatus, str);
        }
        sprintf(str,"\r\n");                                //add cr lf
        strcat(gStatus, str);
    }		
    else if (!strcmp(gStatus_Type, "WB57"))
    {
        strcpy(gStatus, inst );		
        //strcat(gStatus, gTimestamp); //optional timestamp
        sprintf(str,"%.2f",gPartCon_num_cc);                //Status
        strcat(gStatus, str);

        for (i=0; i<Bins; i++)                              //Histogram
        {	
            sprintf(str, ",%d", gHist[i]);
            strcat(gStatus, str);
        }
        sprintf(str,"\r\n");                                //add cr lf
        strcat(gStatus, str);
    }
    
    else if (!strcmp(gStatus_Type, "Display"))
    {
        strcpy(gStatus, gDispTime);                         //Time
        strcat(gStatus, gPeakFShort);                       //File (name only)
        sprintf(str,"  %.2f Pump hrs",gPumpLife);                    //PumpLife
        strcat(gStatus,str); 
        sprintf(str,",%.2f",gPartCon_num_cc);               //PartCon
        strcat(gStatus, str);
        sprintf(str,",%.2f",gAI_Data.ai[0].value);          //AI Flow
        strcat(gStatus,str);
        sprintf(str,",%.2f", P);                            //P 
        strcat(gStatus,str);
        sprintf(str,",%.2f",gAI_Data.ai[5].value);          //AI Temp
        strcat(gStatus,str);
        sprintf(str,",%d",gSkip_Save);                       //SkipSave
        strcat(gStatus,str);
        sprintf(str,",%d",gBins.nbins);                     //Bin info
        strcat(gStatus,str);
        sprintf(str,",%.3f",gBins.logmin);
        strcat(gStatus,str);                
        sprintf(str,",%.3f",gBins.logmax);
        strcat(gStatus,str);   
        sprintf(str,",%.2f",gPumpLife);
        strcat(gStatus,str); 
        
        for (i=0; i<Bins; i++)                              //Histogram
        {	
            sprintf(str, ",%d", gHist[i]);
            strcat(gStatus, str);
        }
        sprintf(str,"\r\n");                                //add cr lf
        strcat(gStatus, str);
    }
    else                                                    //default to iMet
    {
        strcpy(gStatus,"xdata=3801");
        sprintf(str, "%04X",(int)gPartCon_num_cc);          //Particle Concentration
        strcat(gStatus,str);
        if ((gAI_Data.ai[0].value >= 0.) && (gAI_Data.ai[0].value < 10.))
        {
            sprintf(str,"%02X", (int)(10*gAI_Data.ai[0].value));    //POPS Flow
        }
        else sprintf(str,"%02X", 0);
        strcat(gStatus,str);

        temp_out = (int)(100+gAI_Data.ai[5].value);				
            //  Temperature selection:
            //     T = T of P ambient chip
            //     gAI_Data.ai[2].value = LDTemp
            //     gAI_Data.ai[4].value = LD_Mon
            //     gAI_Data.ai[5].value = external thermistor
			
        nantest =isnan(temp_out);
        if (nantest != 0) temp_out = 0.;                // remove NaN math problem

        strcat(gStatus,str);

        for (i=0; i<Bins; i++)                          //Histogram
        {
             sprintf(str, "%04X", gHist[i]);
             strcat(gStatus, str);
        }
        sprintf(str,"\r\n");                            // add cr lf
        strcat(gStatus, str);
    }
}

//******************************************************************************
//
//  Open_Serial
//
//  Open serial UART 1 or 2, RS232.
//
//  Parameters: int port (1 or 2)
//  			int baud (baud rate)
//
//  Returns: int fd (UART reference)
//
//******************************************************************************

int Open_Serial(int port, int baud)
{
    int file;
    struct termios options;		 //The termios structure is vital

    switch(port)
    {
        case 1:
            if ((file = open("/dev/ttyO1", O_RDWR | O_NOCTTY ))<0)
            {
                strcat(gMessage,"\tUART1: Failed to open the file.\n");
                gSerial_Ports.serial_port[0].open = false;
                return -1;
            }
            gSerial_Ports.serial_port[0].open = true;
            break;
        case 2:
            if ((file = open("/dev/ttyO2", O_RDWR | O_NOCTTY ))<0)
            {
                strcat(gMessage,"\tUART2: Failed to open the file.\n");
                gSerial_Ports.serial_port[1].open = false;
                return -1;
            }
            gSerial_Ports.serial_port[1].open = true;
            break;
        default:
            if ((file = open("/dev/ttyO1", O_RDWR | O_NOCTTY ))<0)
            {
                strcat(gMessage,"\tUART-default: Failed to open the file.\n");
                gSerial_Ports.serial_port[port].open = false;
                return -1;
            }
            gSerial_Ports.serial_port[0].open = true;
            break;
    }
    tcgetattr(file, &options);		 //Sets the parameters associated with file
// Set up the communications options:
//   8-bit, enable receiver, no modem control lines
   
    switch (baud)
    {
        case 9600:
            options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
            break;
        case 19200:
            options.c_cflag = B19200 | CS8 | CREAD | CLOCAL;
            break;
        case 115200:
            options.c_cflag = B115200 | CS8 | CREAD | CLOCAL;
            break;
        case 1200:
            options.c_cflag = B1200 | CS8 | CREAD | CLOCAL;
            break;
        case 2400:
            options.c_cflag = B2400 | CS8 | CREAD | CLOCAL;
            break;
        case 4800:
            options.c_cflag = B4800 | CS8 | CREAD | CLOCAL;
            break;
        case 38400:
            options.c_cflag = B38400 | CS8 | CREAD | CLOCAL;
            break;
        case 57600:
            options.c_cflag = B57600 | CS8 | CREAD | CLOCAL;
            break;
        default:
            options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
            break;
    }

    options.c_iflag = IGNPAR ;          //ignore partity errors, CR -> newline
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VTIME] = 0;            // Don't wait for time
    options.c_cc[VMIN] = 0;             // minimum characters is 0
    tcflush(file, TCIFLUSH);            //discard file information not transmitted
    tcsetattr(file, TCSANOW, &options); //changes occur immmediately

    return (file);
}

//******************************************************************************
//
//  Close_Serial
//
//  Close serial UART 1 or 2, RS232.
//
//  Parameters:  int UART (UART reference)
//
//******************************************************************************

void Close_Serial(int UART)
{
    close(UART);
}

//******************************************************************************
//
//  Send_Serial
//
//  Send serial to UART 1 or 2, RS232.
//
//  Parameters: int UART (UART reference)
//              char msg[] (text message to send)
//
//  Returms: int error (0: OK, -1 error)
//
//******************************************************************************

int Send_Serial(int UART, char msg[])
{
    int count, len;

    if ((count = write(UART, msg, (strlen(msg))))<0)    //send the string without
                                                        //null termination
    {
        strcat(gMessage,"Failed to write to the output - UART\n");
        return -1;
    }

    return 0;
}

//******************************************************************************
//
//  Read_Serial
//
//  Read serial from UART 1 or 2, RS232.
//
//  Parameters: int UART (UART reference)
//
//	ReturnsL int count (-1 if empty)
//
//******************************************************************************

int Read_Serial(int UART)
{

    int count;
// Buffer for receiving data = gCMD[512] global variable;
    if ((count = read(UART, (void*)gCMD, 512)) > 0)	//read the string
    {
        gCMD[count]=0;              //There is no null character sent
        return count;
    }
    else return -1;
}

//******************************************************************************
//
//  Calc_Baseline
//
//  Calculate the baseline for the high speed analog in,
//  and load baseline + threshold for use by PRU1.
//  Use 16 bit packed data into 1 KB. (512 points).
//
//******************************************************************************

void Calc_Baseline( void )
{
    double bl = 0.;                         // calculation of Baseline
    double var = 0.;                        // varience of Baseline
    int i;                                  // Step for baseline address

/* Read PRU1 RAM memory. */

    for (i=0; i<256; i++)                   // Read 0..255
    {
        gRawBL_Data[2*i] = (double)((pru1DRAM_int[i] & 0xFFFF0000) >> 16);
        gRawBL_Data[2*i+1] = (double)(pru1DRAM_int[i] & 0x0000FFFF);
        bl = bl + (gRawBL_Data[2*i] + gRawBL_Data[2*i+1]);
    }

    bl = bl/512.0;

    for (i=0;i<512; i++)
    {
        var += (gRawBL_Data[i]-bl)*(gRawBL_Data[i]-bl);
    }
    gSTD = sqrt(var/511.);
    gBLTH = (unsigned int) (bl+gTH_Mult*gSTD);
    gBaseline = (unsigned int)bl;

//Write the new baseline to the PRU
    pru1DRAM_int[256] = gBLTH;
    pru1DRAM_int[257] = gBaseline;
}


//******************************************************************************
//
//  Read_RawData
//
//  Read the raw data stored in PRU0 memory and store in the global gRaw_Data[].
//  Use 16 bit packed data into 1 KB. (512 points).
//
//******************************************************************************

void Read_RawData( void )
{
    int i;                                  // Step for address
    int count = 0;                          // Peak counter
    int flag = 0;                           // peak found flag
    
/* Read PRU0 RAM memory. */

    for (i=0; i<256; i++)                   // Read 0..255
    {
        gRaw_Read[2*i+1] = (double)((pru0DRAM_int[i] & 0xFFFF0000) >> 16);
        gRaw_Read[2*i] = (double)(pru0DRAM_int[i] & 0x0000FFFF);
    }
    for(i=0; i<512; i++)
    {
        if (gRaw_Read[i] > gBLTH)
        {
            count +=1;
            if(count > 10) flag = 1;
        }    
        else count = 0;
    }
    if (flag > 0)
    {
        for(i=0; i<512; i++)
        {
            gRaw_Data[i] = gRaw_Read[i];
        }
    } 
}


//******************************************************************************
//
//  CalcBins
//
//  Calculate the Histogram BINS.  The are binned in raw log10(16 bit AI value).
//
//******************************************************************************

void CalcBins(void)
{
    double logdelta, dbin;
    int i, bin;

//	clear the Bins
    for(i=0; i<gBins.nbins;i++)
    {
    gHist[i]=0;
    }
    if(gArray_Size == 0) return;        // Don't try binning if there are no particles
//	make the bins in log space
    logdelta=(gBins.logmax - gBins.logmin)/(double)gBins.nbins;

//	process all of the data into the bins
    for (i=0; i<gArray_Size; i++)
    {
        dbin = (log10(gData.peak[i].max)-gBins.logmin)/logdelta;
        bin = (int)dbin;
        if((dbin>=0) && (bin<gBins.nbins)) gHist[bin]++;
    }
    return;
}

//******************************************************************************
//
//  CompressBins
//
//  Compress 16 BINS to 8.  Used on some balloon or aircraft Status outputs.
//
//  gHist 0..5 are 16 bin data
//  gHist 6 is 6+7+8+9
//  gHist 7 is 10+11+12+13+14+15
//
//******************************************************************************

void CompressBins(void)
{
    gHist[6] = gHist[6] + gHist[7] + gHist[8] + gHist[9];
    gHist[7] = gHist[10] + gHist[11] + gHist[12] + gHist[13] + gHist[14] + gHist[15];

    return;
}

//******************************************************************************
//
//  Calc_WidthSTD
//
//  Calculate the standard deviation of the width of the particles.
//
//******************************************************************************
void Calc_WidthSTD(void)
{
    double var;
    int i;
    if (gArray_Size > 5) 
    {
        for ( i =0; i< gArray_Size; i++)
        {
            gAW += gData.peak[i].w;
        }
        gAW =gAW/gArray_Size;
    
        for (i = 0; i < gArray_Size; i++)
        {
            var += (gData.peak[i].w-gAW)*(gData.peak[i].w-gAW);
        }
        gWidthSTD = sqrt(var/(gArray_Size-1));
    }
    else 
    {
        gAW = 0;
        gWidthSTD = 0;
    }
    
    return;
}

//******************************************************************************
//
//  Write Files
//
//  Write the data to files.
//
//******************************************************************************

void Write_Files(void)
{
//	Get the Peak File size in bytes, and start new files if it is too big.
//	Set at 50 MB for now.

    struct stat st;
    int status;
    size_t M_length;

    FILE *fpb;          // binary file pointer
    FILE *fph;          // housekeeping file pointer
    FILE *fpl;          // log file pointer
    FILE *fpr;          // raw data file pointer

    status = stat(gPeakFile, &st);
    if(status == 0)
    {
        if (st.st_size >= 51200000)
        {
            makeFileNames();
        }
    }

    fph = fopen (gHK_File, "a+");
    if (fph == NULL) strcat(gMessage, "HK file could not be opened.\n");
    else
    {
        fprintf(fph,"%s",&gHK);
    }
    fclose (fph);

    if (strlen(gMessage) > 0)
    {
        fpl = fopen(gLogFile, "a+");
        if (fpl == NULL) strcat(gMessage,"Could not opem message file");
        else
        {
            fprintf(fpl, "%s",&gMessage);
        }
        fclose (fpl);
    }
    strcpy(gMessage,"");                    // Clear the messages

// Write binary peak data file (size of array, then data)

    fpb = fopen( gPeakFile, "a+");
    if (fpb == NULL)
    {
        strcat(gMessage,"Binary file could not be opened.\n");
    }
    else
    {
        size_t gdatasize, size_array, size_time;
        size_time = sizeof(double);
        size_array = sizeof(unsigned int);

        gdatasize = (gArray_Size)*(3*sizeof(unsigned int));

        fwrite(&gArray_Size, size_array,1,fpb);
        fwrite(&gFullSec,size_time,1,fpb);
        fwrite(&gData, gdatasize, 1, fpb);
    }

    fclose (fpb);

// Write raw data file if save is true
    if(gRaw.save)
    {
        fpr = fopen(gRawFile, "a+");
        if (fpr == NULL) strcat(gMessage,"Raw Data file could not be opened.\n");
        else
        {
            fwrite(&gRaw_Data, gRaw.pts*sizeof(unsigned int),1,fpr);
        }
        fclose (fpr);
    }

    gPart_Num = gArray_Size;                // Pass the value for in-lineing
    gArray_Size = 0;                        // Clear these for the next counts
    gRaw.ct = 0;

}


//******************************************************************************
//
//  Implement_CMD
//
//  Implement the serial commands received and then clear it again.
//
//  Parameters: int source (1, integer commands: 2, text commands)
//
//******************************************************************************

void Implement_CMD(int source)
{
    char CMD[20], sValue[20];
    char *pch;
    int Eq_pos;
    float value;
    if(source == 1)		// UART1, 0-9 CMDS
    {
        if (!strncmp(gCMD, "0", 1)) makeFileNames();    //NewFile
        if (!strncmp(gCMD, "1", 1)) gSkip_Save = 0;     //No Skip Save
        if (!strncmp(gCMD, "2", 1)) gSkip_Save = 1;     //Skip 1 (save 1of 2)
        if (!strncmp(gCMD, "3", 1)) gSkip_Save = 4;     //Skip 4 (save 1 of 5)
        if (!strncmp(gCMD, "4", 1)) gSkip_Save = 9;     //Skip 9 (save 1 of 10)
        if (!strncmp(gCMD, "5", 1)) ;                   //Available
        if (!strncmp(gCMD, "6", 1)) ;                   //Available
        if (!strncmp(gCMD, "7", 1)) ;                   //Available
        if (!strncmp(gCMD, "8", 1)) gStop = true;       //Shutdown
        if (!strncmp(gCMD, "9", 1)) gReboot = true;     //Reboot
        if (!strncmp(gCMD, "LFE_",4))
        {
            pch = strtok(gCMD,"_");
            strcpy(CMD,pch);
            pch = strtok(NULL,"_");
            value = atof(pch);
            gAI_Data.ai[0].value=value/60.0;
        }
        memset(&gCMD[0],0,sizeof(gCMD));                //Clear the command
    }
    if(source == 2)
         if (!strncmp(gCMD, "LFE_",4))    // Set the POPS Flow from the Manta
        {
            pch = strtok(gCMD,"_");
            strcpy(CMD,pch);
            pch = strtok(NULL,"_");
            value = atof(pch);
            gAI_Data.ai[0].value=value/60.0;
        }
    {
        pch = strtok(gCMD,"=");
        strcpy(CMD,pch);
        pch = strtok(NULL,"=");
        value = atof(pch);

        if (!strcmp(CMD, "NewFile"))    makeFileNames();
        if (!strcmp(CMD, "Skip"))       gSkip_Save = (int) value;
        if (!strcmp(CMD, "nbins"))      gBins.nbins = (int) value;
        if (!strcmp(CMD, "logmin"))     gBins.logmin = value;
        if (!strcmp(CMD, "logmax"))     gBins.logmax = value;
        if (!strcmp(CMD, "TH_mult"))    gTH_Mult =  value;
        if (!strcmp(CMD, "MaxPts"))     gMaxPeakPts = (int) value;
        if (!strcmp(CMD, "MinPts"))     gMinPeakPts = (int) value;
        if (!strcmp(CMD, "AO0"))
        {
            gAO_Data.ao[0].set_V = value;
            Set_AO(0,value);
        }
        if (!strcmp(CMD, "AO1"))
        {
            gAO_Data.ao[1].set_V = value;
            Set_AO(1, value);
        }
        if (!strcmp(CMD, "BLStart"))	
        {
            gBL_Start = (int) (value);
        }	
        if (!strcmp(CMD, "ViewRaw"))
        {
            if (value>=0) gRaw.view = true;
            else gRaw.view = false;
        }
        if (!strcmp(CMD, "RawPts"))     gRaw.pts = (int) value;
        if (!strcmp(CMD, "Shutdown"))   gStop = true;
        if (!strcmp(CMD, "Reboot"))
        {
            gReboot = true;
            gStop = true;
        }
        memset(&gCMD[0],0,sizeof(gCMD));    //Clear the command
    }
}

//******************************************************************************
//
//  Check_stop
//
//  Checks the STOP condition by looking at the stop bit on PRU1.
//
//******************************************************************************

void Check_Stop( void)
{
//	Check the stop bit on PRU1 (r31.b11)

    if (pru1DRAM_int[259] > 0) 
   {
      usleep(1);
     if(pru1DRAM_int[259] > 0) gStop = 1;
     else gStop = 0;
    }

}

//******************************************************************************
//
//  MIN
//
//  Get minimum of two integer numbers.
//
//  Parameters: int a (number a)
//	            int b (number b)
//
//	Returns: int minimum
//
//******************************************************************************

int MIN( int a, int b)
{

    if(a < b) return(a);
    return(b);
}

//******************************************************************************
//
//  MAX
// 
//  Get maximum of two integer numbers.
//
//  Parameters: int a (number a)
//	            int b (number b)
//
//  Results: int maximum
//
//******************************************************************************

int MAX( int a, int b)
{

    if(a > b) return(a);
    return(b);
}

//******************************************************************************
//
//  timeval_subtract
//
//  Subrtact times with roll over
//
//  Parameters: struct timeval *result
//	            struct timeval *x
//              struct timeval *y
//
//	Results: int (flag for time up)
//
//******************************************************************************

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
/* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) 
    {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) 
    {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

/* Compute the time remaining to wait. tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

/* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

//******************************************************************************
//
//  CheckFlowStep
//
//  Check the pressure and adjust the flow voltage if necessary
//
//******************************************************************************
void CheckFlowStep(void)
{
    int i, oldFlag, newFlag;
    double OldPumpV, NewPumpV;
    OldPumpV = gAO_Data.ao[1].set_V;
    oldFlag = gFlowFlag;
    NewPumpV = OldPumpV;
    if (P >= 10.)
    {
        if (P > gFlowStep.Step[0].Press)            // Start voltage
        {
            NewPumpV = gStartFlowV;
            newFlag = 0;
        }
        for(i=0; i< (gFlowSteps-2); i++)            // intermediate pressure/voltage
        {
            if((P <= gFlowStep.Step[i].Press) && (P > gFlowStep.Step[i+1].Press))
            {
                NewPumpV = gFlowStep.Step[i].PumpV;
                newFlag = i+1;
            }
        }
        if (P<= gFlowStep.Step[i].Press)            // last pressure/voltage
        {
            NewPumpV = gFlowStep.Step[i].PumpV;
            newFlag = i+1;
        }
    }
    
    if(oldFlag != newFlag)
    {
        gFlowFlag = newFlag;
        gAO_Data.ao[1].set_V = NewPumpV;
        Set_AO(1,0);
    }
    
    return;
}
//******************************************************************************
//
//  Set_AO
//
//  Set an AO using the MAX5802 on the i2c2 bus (/dev/i2c1)
//
//  Parameters: unsigned int n (channel number)
//              double Read_V (reference used for PID if used)
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status Set_AO(unsigned int n, double Read_V)
{
    double epsilon = 0.01, dt =0.1, error, V_set_in, integral=0, output;
    MAX5802_status nReturnValue;

    V_set_in = gAO_Data.ao[n].set_V;


    if(gAO_Data.ao[n].use_pid == true)
    {
        error = V_set_in - Read_V;
        if (abs(error)>epsilon)
        {
            integral = gAO_Data.ao[n].Ki*error*dt;
        }

        gAO_Data.ao[n].set_V = V_set_in + integral;
    }
	
    if (gAO_Data.ao[n].set_V > gAO_Data.ao[n].maxV) gAO_Data.ao[n].set_V = \
        gAO_Data.ao[n].maxV;

    if (gAO_Data.ao[n].set_V < gAO_Data.ao[n].minV) gAO_Data.ao[n].set_V = \
        gAO_Data.ao[n].minV;

    gAO_Data.ao[n].set_num = (unsigned int) (gAO_Data.ao[n].set_V * \
        4096./gAO_Data.ao[n].maxV);

    nReturnValue =MAX5802_set_DAC_12_bit_value(n, gAO_Data.ao[n].set_num);
    return(nReturnValue);
}

//******************************************************************************
//
//  MAX5802_set_internal_reference
//
//  Initialize the internal voltage reference in the MAX5802
// 
//		  Only the 3 LSBs of the uchReference command are used (sent in
//		  bit[18:16] of command), uchReferenceCommand[2] = 0: Ref power off,
//		  1: Ref power on (normal operating mode) uchReferenceCommand[1:0] =
//		  00: ext ref, 01: internal 2.500V, 10: internal 2.048V, 11: internal
//		  4.096V. Note that int ref of 4.096V requires VCC = 4.5V min
//
//	Parameters: short unsigned int uchReferenceCommand (specifies voltage 
//	            reference setting desired)
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_set_internal_reference( short unsigned int uchReferenceCommand)
{
    unsigned char uchTxBuffer[3];
    short unsigned int uchCommandToSend=0;
    int file;

    uchCommandToSend = uchReferenceCommand;
    uchCommandToSend &= 0x07;   // mask off all but 3 LSBs
    uchCommandToSend |= 0x70;   // set the REF command bit

// REF command is 0x7-  where bit[3]=0 and bit[2:0] is 0-7
    uchTxBuffer[0] = uchCommandToSend;
    uchTxBuffer[1] = 0x00;
    uchTxBuffer[2] = 0x00;

    if((file=open("/dev/i2c-1", O_RDWR)) < 0)
    {
        strcat(gMessage,"Failed to open the i2c-1 bus.\n");
        return MAX5802_status_i2c_transfer_error;
    }
    if(ioctl(file, I2C_SLAVE, 0x0F) < 0)
    {
        strcat(gMessage,"Failed to connect to the MAX5802.\n");
        return MAX5802_status_i2c_transfer_error;
    }

// Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage,"Failed to set MAX5802 internal reference.\n");
        return MAX5802_status_i2c_transfer_error;
    }

    close(file);

    return MAX5802_status_ok;
}

//******************************************************************************
//
//  MAX5802_send_sw_clear
//
// 		Send a software clear (SW_CLEAR) command to the MAX5802.
// 
//		All CODE and DAC registers are cleared to their default values.
// 		Sends the three byte command = 0x349630
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_send_sw_clear(void)
{
    unsigned char uchTxBuffer[3];
    int file;

// Send SW CLEAR (0x34 0x96 0x30)
    uchTxBuffer[0] = 0x50;
    uchTxBuffer[1] = 0x00;
    uchTxBuffer[2] = 0x00;

    if((file=open("/dev/i2c-1", O_RDWR)) < 0)
    {
        strcat(gMessage,"Failed to open the i2c-1 bus.\n");
        return MAX5802_status_i2c_transfer_error;
    }
    if(ioctl(file, I2C_SLAVE, 0x0F) < 0)
    {
        strcat(gMessage,"Failed to connect to the MAX5802.\n");
        return MAX5802_status_i2c_transfer_error;
    }

// Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage, "Failed to send MAX5802 sw clear.\n");
        return MAX5802_status_i2c_transfer_error;
    }

    close(file);

    return MAX5802_status_ok ;
}

//******************************************************************************
//
//  MAX5802_send_sw_reset
//
// 	Send a software reset (SW_RESET) command to the MAX5802.
//
//	All CODE, DAC and control registers are cleared to their power-on values.
//	Sends the three byte command = 0x359630
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_send_sw_reset(void)
{
    unsigned char uchTxBuffer[3];
    int file;

// Send SW RESET (0x35 96 30)
    uchTxBuffer[0] = 0x51;                  // CODE command + All 8 DACs address
    uchTxBuffer[1] = 0x00;
    uchTxBuffer[2] = 0x00;

    if((file=open("/dev/i2c-1", O_RDWR)) < 0)
    {
        strcat(gMessage, "Failed to open the i2c-1 bus.\n");
        return MAX5802_status_i2c_transfer_error;
    }
    if(ioctl(file, I2C_SLAVE, 0x0F) < 0)
    {
        strcat(gMessage, "Failed to connect to the MAX5802.\n");
        return MAX5802_status_i2c_transfer_error;
    }

// Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage, "Failed to send MAX5802 sw reset.\n");
        return MAX5802_status_i2c_transfer_error;
    }

    close(file);

    return MAX5802_status_ok;
}

//******************************************************************************
//
//  MAX5802_set_default_DAC_settings
//
// 	Send a software clear (SW_CLEAR) command to the MAX5802.
//
//	    All CODE registers are set to 1 of 5 possible default states, based on
//          uchDefaultCodeSetting:
// 	    MAX5802_DEFAULT_M_ZNOT: set dac's CODE register based on M/Z pin setting
//	    MAX5802_DEFAULT_ZERO: set dac's CODE register to zero
//	    MAX5802_DEFAULT_MID: set dac's CODE register to mid scale
//	    MAX5802_DEFAULT_FULL: set dac's CODE register to full scale
//	    MAX5802_DEFAULT_RETURN: set dac's code register = RETURN register
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_set_default_DAC_settings(void)
{
    unsigned char uchTxBuffer[3];
    int file;

    if((file=open("/dev/i2c-1", O_RDWR)) < 0)
    {
        strcat(gMessage, "Failed to open the i2c-1 bus.\n");
        return MAX5802_status_i2c_transfer_error;
    }
    if(ioctl(file, I2C_SLAVE, 0x0F) < 0)
    {
        strcat(gMessage, "Failed to connect to the MAX5802r.\n");
        return MAX5802_status_i2c_transfer_error;
    }
// Send config (0x60 03 00)
    uchTxBuffer[0] = 0x60;
    uchTxBuffer[1] = 0x03;
    uchTxBuffer[2] = 0x00;

// Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage, "Failed to set default DAC settings.\n");
        return MAX5802_status_i2c_transfer_error;
    }

// Send Power (0x40 03 00)
    uchTxBuffer[0] = 0x40;
    uchTxBuffer[1] = 0x03;
    uchTxBuffer[2] = 0x00;

// Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage, "Failed to set default DAC settings.\n");
        return MAX5802_status_i2c_transfer_error;
    }

    close(file);

    return MAX5802_status_ok;
}

//******************************************************************************
//
//  MAX5802_initialize
//
// 	Set the MAX5802 dac CODE registers equal to their respective				
//	RETURN registers and set the voltage reference = internal 4.096V
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_initialize(void)
{
    MAX5802_status nReturnValue = MAX5802_status_ok;
    nReturnValue = MAX5802_send_sw_clear();

    delay(100);

    nReturnValue = MAX5802_send_sw_reset();

    delay(100);

// Set all dac CODE registers = respective RETURN register settings
    nReturnValue = MAX5802_set_default_DAC_settings();

    delay(100);

// Set internal reference to 4.096V
    nReturnValue = MAX5802_set_internal_reference(MAX5802_4096_MV_REF);

    delay(100);

    return (nReturnValue);
}

//******************************************************************************
//
//  MAX5802_set_CODE_register
//
// 	Set the CODE register for selected dac channel(s) within the MAX5802.
//
//  Parameters: short unsigned int uchChannelToSet (channel to set)
// 	            unsigned int unRegisterSetting (setting)
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_set_CODE_register(unsigned short int uchChannelToSet,
    unsigned int unRegisterSetting)
{
    unsigned char uchTxBuffer[3];
    unsigned int unDACValue=0;
    short unsigned int uchChannel=0x00;
    int file;

    if(unRegisterSetting >= 4096)
        unDACValue = 4095;
    else
        unDACValue = unRegisterSetting;

    if(uchChannelToSet > 0x08)
        uchChannel = 0x08;
    else
        uchChannel = uchChannelToSet;

// Send the CODE command
    uchTxBuffer[0] = 0x00;                          // The base CODEn command is 0x80
    uchTxBuffer[0] |= uchChannel;                   // Set channel #0-1
    uchTxBuffer[1] = ((unDACValue & 0x0FF0) >> 4);  // MSB data
    uchTxBuffer[2] = ((unDACValue & 0x000F) << 4);  // LSB DATA (aligned to upper 4 bits)

    if((file=open("/dev/i2c-1", O_RDWR)) < 0)
    {
        strcat(gMessage, "Failed to open the i2c-1 bus.\n");
        return MAX5802_status_i2c_transfer_error;
    }
    if(ioctl(file, I2C_SLAVE, 0x0F) < 0)
    {
        strcat(gMessage, "Failed to connect to the MAX5802.\n");
        return MAX5802_status_i2c_transfer_error;
    }

//Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage, "Failed to send MAX5802 set CODE register.\n");
        return MAX5802_status_i2c_transfer_error;
    }

    close(file);

    return MAX5802_status_ok;

}

//******************************************************************************
//
//  MAX5802_LOAD_DAC_from_CODE_register
//
//  Set the DAC register from the CODE register for selected dac channel(s)
//	    within the MAX5802.
//
//  Parameters: short unsigned int uchChannelToSet (channel to set)
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_LOAD_DAC_from_CODE_register(short unsigned int uchChannelToSet)
{
    unsigned char uchTxBuffer[3];
    short unsigned int uchChannel=0x00;
    int file;

    if(uchChannelToSet > 0x08)
        uchChannel = 0x08;
    else
        uchChannel = uchChannelToSet;

// Send the LOAD command
    uchTxBuffer[0] = 0x10;          // The base LOADn command is 0x10
    uchTxBuffer[0] |= uchChannel;   // Set channel #0-1
    uchTxBuffer[1] = 0x00;          // NULL - doesn't matter, data is ignored.
    uchTxBuffer[2] = 0x00;          // NULL - doesn't matter, data is ignored.

    if((file=open("/dev/i2c-1", O_RDWR)) < 0)
    {
        strcat(gMessage, "Failed to open the i2c-1 bus.\n");
        return MAX5802_status_i2c_transfer_error;
    }
    if(ioctl(file, I2C_SLAVE, 0x0F) < 0)
    {
        strcat(gMessage, "Failed to connect to the MAX5802.\n");
        return MAX5802_status_i2c_transfer_error;
    }

// Send I2C reference command
    if(write(file, uchTxBuffer, 3)!=3)
    {
        strcat(gMessage, "Failed to send MAX5802 LOAD DAC from CODE register.\n");
        return MAX5802_status_i2c_transfer_error;
    }

    close(file);

    return MAX5802_status_ok;
}

//******************************************************************************
//
//  MAX5802_set_DAC_12_bit_value
//
//  Set the CODE register for the specified channel within the MAX5802 to the
//	value unRegisterSetting, then move that value into the DAC register.
//
//  Parameters: short unsigned int uchChannelToSet (channel to set)
//      unsigned int unRegisterSetting (setting)
//
//  Returns: MAX5802_status (ok or error)
//
//******************************************************************************

MAX5802_status MAX5802_set_DAC_12_bit_value(short unsigned int uchChannelToSet,
    unsigned int unRegisterSetting)
{
    MAX5802_status nReturnValue = MAX5802_status_ok;

    if(MAX5802_set_CODE_register(uchChannelToSet, unRegisterSetting)!=MAX5802_status_ok)
        nReturnValue=MAX5802_status_set_CODE_reg_error;

    if(MAX5802_LOAD_DAC_from_CODE_register(uchChannelToSet)!=MAX5802_status_ok)
        nReturnValue=MAX5802_status_LOAD_DAC_error;

    return nReturnValue;
}

//******************************************************************************
//
//  delay
//
// 	Loop for nStopValue iterations to provide a delay.
//
//  Parameters: int nStopValue (number of cycles)
//
//******************************************************************************

void delay(int nStopValue)
{
    int i=0;
    int a=0;

    for(i=0;i<nStopValue;i++)
    {
        a=i;
    }
}

//******************************************************************************
//
//  Initialize_GPIO
//
//  Initialize the GPIO memory pointers for general access and for:
//
//  Bit-banged SPI interface to the ms5607 P&T chip
//	    p9.13 gpio0_30 output pulldown  SCLK (input on ship)
//	    p9.14 gpio1_28 output pullup	CSB  (chip select when 0)
//	    p9.15 gpio0_31 input  pulldown  SDO on chip
//	    p9.16 gpio1_18 output pulldown  SDI on ship
//
//  Generally defined:  p8.9    gpio2_5     input   pulldown
//                      p8.10   gpio2_4     input   pulldown
//                      p8.11   gpio1_13    input   pullup
//                      p8.12   gpio1_12    output  pulldown
//                      p8.13   gpio0_23    output  pulldown
//                      p8.14   gpio0_26    output  pullup
//
//******************************************************************************

void Initialize_GPIO(void)
{
    iolib_init();                   // initialize the library
    iolib_setdir(9,13, DIR_OUT);    // SCLK
    iolib_setdir(9,14, DIR_OUT);    // CBS
    iolib_setdir(9,15, DIR_IN);	    // SDO
    iolib_setdir(9,16, DIR_OUT);    // SDI
    iolib_setdir(8,12, DIR_OUT);    // ms timing test pin
    iolib_setdir(8,13, DIR_OUT);    // 1 sec timing test pin
    iolib_setdir(8,14, DIR_OUT);    // AI out of range test
    pin_high(9,14);                 // initial value CSB (not selected)
    pin_low(9,13);                  // intial value SCLK
    pin_low(9,16);                  // initial value SDI
    pin_low(8,12);
    pin_low(8,13);
    pin_low(8,14);

}

//******************************************************************************
//
//  Send_TP_CMD
//
//  Clock in command to the ms5607.  8 bit command, sent low bit first.
//  Assumes chip is selected before use.
//
//  Parameters: unsigned int CMD (8 bit command)
//              unsigned int del (delay)
//
//******************************************************************************

void Send_TP_CMD(unsigned int CMD, unsigned int del)
{
    int i;
    unsigned int OUTBIT;

    for (i=0; i<8; i++)
    {
        OUTBIT = (CMD & (1<<(7-i))) >> (7- i);
        if(OUTBIT>0) pin_high(9,16);    // Set the data bit
        else pin_low(9,16);             // Clear the data bit
        delay(del);
        pin_high(9,13);                 // SCLK in the bit
        delay(del);
        pin_low(9,13);                  // SCLK low
    }

}

//******************************************************************************
//
//  Read_TP_Reply
//
//  Read reply from ms5607. Assumes chip is selected before use.
//
//  Parameters: unsigned int del (delay for bit banged SPI)
//
//  Returns: unsigned int reply (to previous command)
//
//******************************************************************************

unsigned int Read_TP_Reply(unsigned int del)
{
    int i;
    unsigned int bit;
    unsigned int reply =0;
    for (i=0; i<8; i++)
    {
        delay(del);
        pin_high(9,13);                     // SCLK high
        delay(del);
        if (is_high(9,15))
        {
            reply |= (1 << (7- i));        // Read and store data
        }
        if (is_low(9,15))
        {
            reply |= (0 << (7-i));         // Read and store data
        }
        pin_low(9,13);                     // SCLK low
    }
    return reply;
}

//******************************************************************************
// 
//  ms5607_reset
// 
//  Send Reset command to MS5607 and wait 10ms each
//
//  Returns: ms5607_status (completion with or without error)
// 
//******************************************************************************

ms5607_status ms5607_reset(void)
{
    unsigned int del=1000;
    pin_low(9,14);                      // CSB chip selected
    pin_low(9,13);                      // SCLK low to be sure
    delay(del);
    Send_TP_CMD(MS5607_CMD_RESET, del);
    iolib_delay_ms(3);                  // delay for reset
    pin_high(9,14);                     // de-select chip
    return ms5607_status_ok;
}

//******************************************************************************
//
//  ms5607_read_prom
//
//  Read factory calibrated coefficients from PROM
//
//  Returns: ms5607_status (completion with or without error)
//			 ms5607_prom_coeffs[i] as globals
//
//******************************************************************************

ms5607_status ms5607_read_prom(void)
{
    int i, j;
    unsigned int del=1000;
    unsigned int part[2];
    unsigned int test_crc;				    // test value of the crc

    for (i=0; i<8; i++)					    // 8 values to read
    {
        pin_low(9,14);					    // CSB chip selected
        delay(del);
        Send_TP_CMD(MS5607_CMD_READ_PROM_BASE+2*i,del);
        for (j=0; j<2; j++)
        {
            part[j]=Read_TP_Reply(del);
        }
        pin_high(9,14);					    // CSB deselected
        delay(del);
        ms5607_prom_coeffs[i] = part[0]*0x100 + part[1];
    }

    test_crc = CRC4(ms5607_prom_coeffs);
    if (test_crc == ms5607_prom_coeffs[7]) return ms5607_status_ok;
    else return ms5607_status_crc_error;

}

//******************************************************************************
//
//  ms5607_read_TP
//
//  Send commands to start a temperature conversion, wait for completion, 
//    read the temperature value, start a pressure conversion, wait for 
//    completion and read the pressure value
//
//  Returns: ms5607_status (completion with or without error)
//	         P and T (temperature of pressure chip) as globals
//
//******************************************************************************

ms5607_status ms5607_read_TP(void)
{
    int i;
    unsigned int del=1000;
    short unsigned int	d1;                 // Pressure
    short unsigned int	d2;                 // Temperature
    short unsigned int read;                // Read command
    unsigned int buf[3];
    long unsigned int pres_raw;
    long unsigned int temp_raw;
    int dT;
    int TEMP;
    long long int OFF;
    long long int SENS;
    int Pr;

    d1 = MS5607_CMD_CONVERT_D1_P;
    d2 = MS5607_CMD_CONVERT_D2_T;
    read = MS5607_CMD_READ_ADC;

    pin_low(9,14);                          // CSB select chip
    delay(del);
    Send_TP_CMD(d1,del);                    // convert P

// Wait for the pressure conversion
    iolib_delay_ms(3);                      // delay for reset

    pin_high(9,14);                         // CSB, deselect chip

    delay(del);
    pin_low(9,14);                          // CSB select chip
    Send_TP_CMD(read,del);
    for (i=0; i<3; i++)
    {
        buf[i]=Read_TP_Reply(del);
    }
    pres_raw =(long unsigned int) (buf[0]*0x10000 + buf[1]*0x100 + buf[2]);

    pin_high(9,14);                         // CSB, deselect chip
    delay(del);

    pin_low(9,14);                          // CSB select chip
    delay(del);
    Send_TP_CMD(d2,del);                    // convert T

// Wait for the temperature conversion
    iolib_delay_ms(3);                      // delay for reset

    pin_high(9,14);                         // CSB, deselect chip

    delay(del);
    pin_low(9,14);                          // CSB select chip
    Send_TP_CMD(read,del);
    for (i=0; i<3; i++)
    {
        buf[i]=Read_TP_Reply(del);
    }

    pin_high(9,14);                         // CSB, deselect chip

    temp_raw =(long unsigned int) (buf[0]*0x10000) + (buf[1]*0x100) + (buf[2]);

    dT = temp_raw - (((unsigned int)ms5607_prom_coeffs[5])<<8);

    TEMP = 2000 + (unsigned int)((dT * ((long long int)ms5607_prom_coeffs[6]))>>23);

    OFF = (((long long int)ms5607_prom_coeffs[2])<<17) +
        ((((long long int)ms5607_prom_coeffs[4])*dT)>>6);

    SENS = (((long long int)ms5607_prom_coeffs[1])<<16) +
        ((((long long int)ms5607_prom_coeffs[3])*dT)>>7);

    Pr = (int)((((((long long int)pres_raw)*SENS)>>21) - OFF)>>15);

    P = (double)Pr/100.0f;
    T = (double)TEMP/100.0f;
    return ms5607_status_ok;

}

//******************************************************************************
//
//  ms5607_CRC4
//
//  Does a CRC4 check on the 7 prom coefficients
//
//  Parameters: long unsigned int n_prom[] (coefficients)
//
//	Returns: unsigned char crc4 (checksum)
//
//******************************************************************************

unsigned char CRC4(long unsigned int n_prom[])
{
    int cnt;                                //counter
    long unsigned int crc_read;             //original value of crc
    unsigned int  n_rem;                    //crc remainder
    unsigned char n_bit;

    n_rem = 0x00;
    crc_read = n_prom[7];                   //save original coefficients

    n_prom[7] = (0xFF00 & (n_prom[7]));     //replace crc bit with 0

    for (cnt = 0; cnt < 16; cnt++)          //LSB and MSB
    {
        if (cnt%2 == 1) n_rem ^= (unsigned short) ((n_prom[cnt>>1])) & 0x00FF;
        else n_rem ^= (unsigned short) (n_prom[cnt>>1]>>8);

        for (n_bit = 8; n_bit > 0; n_bit--)
        {
            if (n_rem & (0x8000))
            {
                n_rem = (n_rem << 1) ^ 0x3000;
            }
            else
            {
                n_rem = (n_rem << 1);
            }
        }
    }

    n_rem = (0x000F & (n_rem >> 12));     //final 4 is CRC code

    n_prom[7] = crc_read;           //restore the crc_read to its original value
    return (n_rem ^ 0x0);           //crc4
}

//******************************************************************************
//
//  Make a watchdog timer
//
//  Parameters: int interval (seconds before reboot)
//
//	Returns: int fd (watchdog reference)
//
//******************************************************************************

int Make_Watchdog( int interval)
{
    int fd, state;
    if ((fd = open(WATCHDOG, O_RDWR))<0)
    {
        strcat(gMessage,"Watchdog: Failed to open watchdog device\n");
        return -1;
    }
// set the timing interval to 30 seconds
    if (ioctl(fd, WDIOC_SETTIMEOUT, &interval)!=0)
    {
        strcat(gMessage,"Watchdog: Failed to set the watchdog interval\n");
        return -1;
    }
    return fd;
}

//******************************************************************************
//
//  Initialize the PRU memory space
//
//  This subroutine is used for testing the memory space.  Specified numbers can
//  be loaded, or the memory can be cleared.
//
//******************************************************************************

void InitPRU_Mem(void)
{
    int i;

// Initial value of the Baseline + Threshold

    pru1DRAM_int[256] = (gBL_Start+0x30);    // BLTH
    pru1DRAM_int[257] = gBL_Start;           // BL
//	 pru1DRAM_int[258] = 0x00010000;         // start of points

    for(i=0; i<256; i++)
    {
        pru1DRAM_int[i] = 0x75007550;
    }

//  for (i=0; i<3072; i++)
//  {
//	    pruSharedMem_int[i] = 0x00000000;
//  }

}

//******************************************************************************
//
// Read PRU Peak Data dt version
//
// Read the current m_tail of the Shared Memory buffer m_head is the next 
// buffer point to read and is carried over from the last read.
// The base address is 0x00010000 and needs to be subtracted.
// 4 bytes read at a time. Do not read the point at M_tail, it may have only
// 2 bytes of data. Read it next time.
//
// There is a 30,000 particle/second maximum.  Any more than this and the data
// is invalid, the particles would be overlapping.
//
//******************************************************************************
void Read_PRU_Data (void)
{
    unsigned int i, j, jmax, m, mmax = 3072, n;
    unsigned int y_all[3072];

// Read the current m_tail of the Shared Memory buffer
// m_head is the next buffer point to read and is
// carried over from the last read.
// The base address is 0x00010000 and needs to be subtracted.
// 4 bytes read at a time. Do not read the point at M_tail, it may have only
// 2 bytes of data. Read it next time.

    m = pru1DRAM_int[258];

    m_tail = (unsigned int)(((double)(m - 0x00010000))/4.0);

// y_all is the new y data, 16 bytes at a time


    if (m_head == m_tail) return;                          // no new data

    if (m_tail > m_head)                                   // read buffer continuous
    {
        i_tail = m_tail - m_head;
        for(i = m_head; i < m_tail; i++)                   // m_head..m_tail-1
        {
            y_all[i-m_head] = pruSharedMem_int[i];
        }
    }
    else                                                   // to end of buffer and start of next
    {
        i_tail = mmax - m_head + m_tail - 1;               // the -1 is because the buffer
                                                           // is 0..3071
        for (i = m_head; i < mmax; i++)                    // m_head..3071
        {
            y_all[i-m_head] = pruSharedMem_int[i];
        }

        for (i = 0; i < m_tail; i++)                       // 0..m_tail-1
        {
            y_all[i+mmax-m_head] = pruSharedMem_int[i];
        }
    }
    m_head=m_tail;                                         // Start of next time through
    for (j=0; j< i_tail; j+=2)
    {
        if(gArray_Size < 29999) // ignore any data over 30,000 particles in one second
        {
        gData.peak[gArray_Size].w =(unsigned int) ((y_all[j] &  0xFFFF0000) >>16) ;	    
        //lower 2 bytes
        gData.peak[gArray_Size].max = (unsigned int) (y_all[j] &  0x0000FFFF) ;			
        //top 2 bytes
        gData.peak[gArray_Size].dt =(unsigned int) (((double)(y_all[j+1] + 16))/200.) ;		
        //4 bytes to dt in uS. 16 added to make up for lost cycles. Divide by 200 MHz.
        gArray_Size+=1;
        }
    }
}

//******************************************************************************
//
//  Open_Socket_Write
//
//  Opens a UDP socket for write
//			
//  Parameters: int i (UDP index)
//
//  Returns: int fd (UDP reference)
//				
//******************************************************************************
 
int Open_Socket_Write(int i)
{
    int fd, slen=sizeof(gUDP.udp[i].remaddr);
    char *server = gUDP.udp[i].IP;	/* change this to use a different server */

/* create a socket */

if ((fd=socket(AF_INET, SOCK_DGRAM, 0)) <0)
        strcat(gMessage,"UDP write socket not created\n");

/* bind it to all local addresses and pick any port number */

    memset((char *)&gUDP.udp[i].myaddr, 0, sizeof(gUDP.udp[i].myaddr));
    gUDP.udp[i].myaddr.sin_family = AF_INET;
    gUDP.udp[i].myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    gUDP.udp[i].myaddr.sin_port = htons(0);

    if (bind(fd, (struct sockaddr *)&gUDP.udp[i].myaddr, sizeof(gUDP.udp[i].myaddr)) < 0) {
        strcat(gMessage, "Write socket bind failed");
        return 0;
    }	   

/* now define remaddr, the address to whom we want to send messages */
/* For convenience, the host address is expressed as a numeric IP address */
/* that we will convert to a binary format via inet_aton */

    memset((char *) &gUDP.udp[i].remaddr, 0, sizeof(gUDP.udp[i].remaddr));
    gUDP.udp[i].remaddr.sin_family = AF_INET;
    gUDP.udp[i].remaddr.sin_port = htons(gUDP.udp[i].port);
    if (inet_aton(server, &gUDP.udp[i].remaddr.sin_addr)==0) {
        strcat(gMessage, "Write socket inet_aton() failed\n");
        return 0;
    }
    return fd;
}
//******************************************************************************
//
//  Open_Socket_Broadcast
//
//  Opens a UDP socket for broadcast
//				
//  Parameters: int i (UDP index)
//
//  Returns: int fd (UDP refernece)
//				
//******************************************************************************
 
int Open_Socket_Broadcast(int i)
{
    int fd, slen=sizeof(gUDP.udp[i].remaddr);
    char *server = gUDP.udp[i].IP;	/* change this to use a different server */


/* create a socket */

    if ((fd=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        strcat(gMessage,"UDP Broadcast socket not created\n");
		
    int broadcastEnable=1;
    int ret=setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

/* bind it to all local addresses and pick any port number */

    memset((char *)&gUDP.udp[i].myaddr, 0, sizeof(gUDP.udp[i].myaddr));
    gUDP.udp[i].myaddr.sin_family = AF_INET;
    gUDP.udp[i].myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    gUDP.udp[i].myaddr.sin_port = htons(0);

    if (bind(fd, (struct sockaddr *)&gUDP.udp[i].myaddr, sizeof(gUDP.udp[i].myaddr)) < 0) {
        strcat(gMessage, "Failed to bind broadcast socket.");
        return 0;
    }	   

/* now define remaddr, the address to whom we want to send messages */
/* For convenience, the host address is expressed as a numeric IP address */
/* that we will convert to a binary format via inet_aton */

    memset((char *) &gUDP.udp[i].remaddr, 0, sizeof(gUDP.udp[i].remaddr));
    gUDP.udp[i].remaddr.sin_family = AF_INET;
    gUDP.udp[i].remaddr.sin_port = htons(gUDP.udp[i].port);
    if (inet_aton(server, &gUDP.udp[i].remaddr.sin_addr)==0) {
        strcat(gMessage, " Broadcast socket inet_aton() failed\n");
        return 0;
    }
    return fd;
}

//******************************************************************************
//
//  Write_UDP
//
//  Writes message to a UDP socket
//			
//  Parameters: int fd (UDP reference)
//				int i (udp index)
//				char msg[] (message to write)
//
//  Returns: int slen (string length)
//				
//******************************************************************************

int Write_UDP(int fd, int i, char msg[])
{
    int slen=sizeof(gUDP.udp[i].remaddr);
    char *server = gUDP.udp[i].IP;	/* change this to use a different server */

    if (sendto(fd, msg, strlen(msg), 0, (struct sockaddr *)&gUDP.udp[i].remaddr, slen)==-1)
        strcat(gMessage,"Error with UDP sendto");
    return slen;
}

//******************************************************************************
//
//  Open_Socket_Read
//
//  Opens a UDP socket for read
//
//  Parameters: int i (UDP index)
//
//	Returns: int fd (UDP reference)
//				
// *****************************************************************************

int Open_Socket_Read(int i)
{
    int fd; 
    struct timeval timeout;	    
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

/* create a socket */

    if ((fd=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        strcat(gMessage,"Read socket not created\n");
    }

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
    sizeof(timeout)) < 0)
        strcat(gMessage," rec setsockopt failed\n");
/* bind it to all local addresses and pick the port number */

    memset((char *)&gUDP.udp[i].myaddr, 0, sizeof(gUDP.udp[i].myaddr));
    gUDP.udp[i].myaddr.sin_family = AF_INET;
    gUDP.udp[i].myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    gUDP.udp[i].myaddr.sin_port = htons(gUDP.udp[i].port);

    if (bind(fd, (struct sockaddr *)&gUDP.udp[i].myaddr, sizeof(gUDP.udp[i].myaddr)) < 0) 
    {
        strcat(gMessage,"Cannot bind rec socket.");
        return 0;
    }	   

    return fd;
}

//******************************************************************************
//
//  UDP_Read_Data
//
//  Read data from a UDP socket
//
//  Parameters: int fd (UDP ID )
//				int i (buffersize)
//
//  Returns: int reclen (-1, no data, >0 received length)
//				
// *****************************************************************************

int UDP_Read_Data(int fd, int i)
{
    int reclen;
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);     /* length of addresses */
	
    reclen = recvfrom(fd, gCMD, 512, 0, (struct sockaddr *)&remaddr, &addrlen);
    if (reclen > 0)
    {
        gCMD[reclen] = 0;                    // add C zero term
    }
	
    return reclen;
}

//******************************************************************************
//
//  Close_UDP_Socket
//
//  Closes a UDP socket
//				
//  Parameters: int (reference to the UDP socket)
//
//  Returns:	void
//				
//******************************************************************************
 
void Close_UDP_Socket(int UDPID)
{
    close(UDPID);
}

