/*********************************************************************
*
* ANSI C Example program:
*    ContAcq-IntClk.c
*
* Example Category:
*    AI
*
* Description:
*    This example demonstrates how to acquire a continuous amount of
*    data using the DAQ device's internal clock.
*
* Instructions for Running:
*    1. Select the physical channel to correspond to where your
*       signal is input on the DAQ device.
*    2. Enter the minimum and maximum voltage range.
*    Note: For better accuracy try to match the input range to the
*          expected voltage level of the measured signal.
*    3. Set the rate of the acquisition. Also set the Samples per
*       Channel control. This will determine how many samples are
*       read at a time. This also determines how many points are
*       plotted on the graph each time.
*    Note: The rate should be at least twice as fast as the maximum
*          frequency component of the signal being acquired.
*
* Steps:
*    1. Create a task.
*    2. Create an analog input voltage channel.
*    3. Set the rate for the sample clock. Additionally, define the
*       sample mode to be continuous.
*    4. Call the Start function to start the acquistion.
*    5. Read the data in the EveryNCallback function until the stop
*       button is pressed or an error occurs.
*    6. Call the Clear Task function to clear the task.
*    7. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal input terminal matches the Physical
*    Channel I/O control. For further connection information, refer
*    to your hardware reference manual.
*
*********************************************************************/
#define ALLEGRO_USE_CONSOLE
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <allegro.h>
#include <NIDAQmx.h>
#include <iostream>
#include <fstream>
#include <string>

#define ROW 1024 
#define COL 1280
#define max 10
#define min -10
#define SR 32000
#define NumChannels 2

void initAllegro();

BITMAP *bmp;
PALLETE pal;

using namespace std;

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

	ofstream file;
	string filename;
	FILE * pFile;
	int col=0, rowOld=0;

int main(void)
{
	initAllegro();

	cout <<"Please input a file name"<<endl;
    getline (cin, filename);
	filename += ".bin";
	
	int32       error=0;
	TaskHandle  taskHandle=0;
	char        errBuff[2048]={'\0'};

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle,"Dev1/ai0","",DAQmx_Val_RSE,min,max,DAQmx_Val_Volts,NULL));

	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"",SR,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(taskHandle,DAQmx_Val_Acquired_Into_Buffer,1000,0,EveryNCallback,NULL));
	DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle,0,DoneCallback,NULL));

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	printf("Acquiring samples continuously. Press Enter to interrupt\n");
	getchar();

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
	}
	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);
	printf("End of program, press Enter key to quit\n");
	getchar();
	return 0;
}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	int32       error=0;
	char        errBuff[2048]={'\0'};
	static int  totalRead=0;
	int32       read=0;
	int16		data[2000];
	int row = 512;

	int16 n = 0;
	float avg = 0;
	float disp = 0;
	float scale = (row-64)/max;

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/

	DAQmxErrChk (DAQmxReadBinaryI16(taskHandle,2000,10.0,DAQmx_Val_GroupByScanNumber,data,2000,&read,NULL));

	if( read>0 ) {

		pFile = fopen(filename.c_str(), "ab");
		fwrite (data, sizeof(char), 2000, pFile); 
		fclose (pFile);
		fflush(stdout);

		n = data[1000];
		for(double i=0; n > 0; i++) {
        if(n % 2 == 1) {
            avg += pow(2, i);
        }
        n /= 2;
		}

		disp = avg*(max-min)/65536;

		printf("Acquired %d samples. Current average %4.3f V. Total %d\r",read,disp,totalRead+=read);

		row=512-(int) (disp*scale); //row and col must be integers within screen size definition
		fastline(bmp,col-1,rowOld,col,row,makecol(0,0,0));
		blit(bmp,screen,0,0,0,0,COL,ROW);
		rowOld=row;

		if(col<COL){
			col=col+2;
		} else {
		clear_to_color(bmp,makecol(200,200,200));
		clear_to_color(screen,makecol(200,200,200));
		col = 0;
		}
	}

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n",errBuff);

	}
	return 0;
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	int32   error=0;
	char    errBuff[2048]={'\0'};

	// Check to see if an error stopped the task.
	DAQmxErrChk (status);

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n",errBuff);
	}
	return 0;
}

void initAllegro(){
	allegro_init();
	install_keyboard();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED,COL,ROW,0,0);
	set_display_switch_mode(SWITCH_BACKGROUND);
	get_palette(pal);
	bmp=create_bitmap(COL,ROW);
}