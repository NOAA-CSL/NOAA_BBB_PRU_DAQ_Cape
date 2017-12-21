/*
// Filename: Read_Peak_Files.c
// Version: 1.0
//
// Project: NOAA - POPS
// Author: Laurel Watts
// Contact: laurel.a.watts@noaa.gov
// Date	: 18 Dec 2017
//
// Read either "old" data with timestamp, peak, position, width, and saturated
// flag, for read the "new" version with timestamp, peak, width, dt.
//
// Filename and "old" or "new" is specified at run time.
// The output filename is the input name with the .b replaced with .txt. It is
// a comma separated variable file.  There is no provision for concatenating 
// data.
//
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
not subject to copyright protection.*/

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
#include <unistd.h>
#include <float.h>
#include <errno.h>

//******************************************************************************
//
// Function prototypes:
//
//******************************************************************************
void Read_New( void );
void Read_Old( void );
//******************************************************************************
//
// Global variables:
//
//******************************************************************************
char gFN[50];                               // input peak file name
char gFNO[50];                              // output file name
char gType[4];                              // input type, old or new

double gFullSec;                            // Timestamp with partial sec

struct Peaks {                              // structure for peak data
    unsigned int max;
    unsigned int w;
    unsigned int dt;
};
struct gData {
    struct Peaks peak[30000];
} gData;                                    // global data structure

struct PeakO {                              // structure for peak data (old)
    unsigned int max;
    unsigned int pos;
    unsigned int w;
    unsigned int is;
};
struct gDataO {
    struct PeakO peako[30000];
} gDataO;                                   // global data structure (old)
unsigned int gArray_Size = 0;               // Size of the data array

//******************************************************************************
//
// Main program:
//
//******************************************************************************

void main()
{
    size_t len;

    printf("Enter the file to read with full path:\n");
    scanf("%s", gFN);
    printf("Enter the file type, new or old:\n");
    scanf("%s", gType);

// Make the output file name    
    len = strlen(gFN);
    strncpy(gFNO, gFN, len-2);          // remove the ".b"
    strcat(gFNO, ".txt");               // add ".txt"
    
    if(strcmp(gType, "new") == 0)
    {
        Read_New();
    }
   
    if(strcmp(gType, "old") == 0)
    {
        Read_Old();
    }
// end of main program    
    
}
//******************************************************************************
//
// Read_New()
//
// Uses global variables as I/O, handles reading and converting the new file 
// format.
//
//******************************************************************************
void Read_New( void)
{
    FILE *fp, *fpo;
    unsigned int i;
    int timeoi;
    char dataline[512] = {""};
    char str[512] = {""};
    char str1[512] ={""};
    double timeo, day = 86400.0;
    size_t gdatasize, size_array, size_time;
    size_time = sizeof(double);
    size_array = sizeof(unsigned int);

// Open the files for read and write.    
    if((fp = fopen(gFN, "rb")) == NULL) 
    printf("ERROR: File could not be opened for read.");
    
    if((fpo = fopen(gFNO, "a+")) == NULL) 
    printf("ERROR: File could not be opened for write.");
    
// write the header on the new file 
    strcpy(str1,"DateTime,Peak,Width,dT");
    sprintf(str,"\r\n");                    // add cr lf
    strcat(str1,str);
    fprintf(fpo,"%s",&str1);                // write to file

//Read 1 sec of data, and write it out one line at a time
    while(!feof(fp))
    {
        fread(&gArray_Size, size_array, 1, fp);
        fread(&gFullSec, size_time,1,fp);
        gdatasize = (gArray_Size)*(3*sizeof(unsigned int));
        fread(&gData, gdatasize, 1, fp);
        //Format and write 1 sec of data
 //       timeo = gFullSec + 2082844800;      // convert to windows time
        timeo = fmod(gFullSec, day);        // seconds since midnight at start
        for(i=0; i < gArray_Size; i++)
        {
            // convert to exact windows time
            timeo += ((double)gData.peak[i].dt)/1000000.;  // add dT
            sprintf(str,"%.6f",timeo);
            strcpy(dataline, str);
            sprintf(str, ",%u", gData.peak[i].max);
            strcat(dataline, str);
            sprintf(str, ",%u", gData.peak[i].w);
            strcat(dataline, str);
            sprintf(str, ",%u", gData.peak[i].dt);
            strcat(dataline, str);
            sprintf(str,"\r\n");            // add cr lf
            strcat(dataline, str);
            
            fprintf(fpo,"%s",&dataline);    // write to file
        }
    }
    
 // Close the files   
    fclose(fp);
    fclose(fpo);
}

//******************************************************************************
//
// Read_Old()
//
// Uses global variables as I/O, handles reading and converting the old file 
// format.
//
//******************************************************************************
void Read_Old( void)
{
    FILE *fp, *fpo;
    unsigned int i;
    char dataline[512] = {""};
    char str[512] = {""};
    char str1[512] ={""};
    double timeo, day = 86400.0;
    size_t gdatasize, size_array, size_time;
    size_time = sizeof(double);
    size_array = sizeof(unsigned int);

// Open the files for read and write.    
    if((fp = fopen(gFN, "rb")) == NULL)
    printf("ERROR: File could not be opened for read.");
    
    if((fpo = fopen(gFNO, "a+")) == NULL)
    printf("ERROR: File could not be opened for write.");
    
// write the header on the new file  
    strcpy(str1,"DateTime,Peak,Position,Width,IS");
    sprintf(str,"\r\n");                    // add cr lf
    strcat(str1,str);
    fprintf(fpo,"%s",&str1);                // write to file

//Read 1 sec of data, and write it out one line at a time
    while(!feof(fp))
    {
        fread(&gArray_Size, size_array, 1, fp);
        fread(&gFullSec, size_time,1,fp);
        gdatasize = (gArray_Size)*(4*sizeof(unsigned int));
        fread(&gDataO, gdatasize, 1, fp);
//Format and write 1 sec of data
//        timeo = gFullSec + 2082844800;      // convert to windows time
        timeo = fmod(gFullSec, day);        // seconds since midnight
        for(i=0; i < gArray_Size; i++)
        {
            sprintf(str,"%.3f",timeo);
            strcpy(dataline, str);
            sprintf(str, ",%u", gDataO.peako[i].max);
            strcat(dataline, str);
            sprintf(str, ",%u", gDataO.peako[i].pos);
            strcat(dataline, str);
            sprintf(str, ",%u", gDataO.peako[i].w);
            strcat(dataline, str);
            sprintf(str, ",%u", gDataO.peako[i].is);
            strcat(dataline, str);
            sprintf(str,"\r\n");            //add cr lf
            strcat(dataline, str);
            fprintf(fpo,"%s",&dataline);
        }
    }
    
 // Close the files   
    fclose(fp);
    fclose(fpo);    
}


