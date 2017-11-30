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
#define SR 16000
#define SAMPS_PER_CHAN_PER_CALLBACK SR/16
#define	BUFFER_SIZE 2*SAMPS_PER_CHAN_PER_CALLBACK*NumChannels
#define	EXP_LENGTH 20	//in minutes		

const int NumChannels = 2;
int displayUpdateNumber = 0;
int spacingV = 32;
int spacingH = 32;

BITMAP		*bmp;
PALLETE		pal;
float 		scaleV = (ROW-2*spacingV)/(NumChannels*(max-min));
float		scaleH = (COL-2*spacingH)/SR;
float		graphWidth = (ROW-spacingV*(NumChannels+1))/NumChannels;
int			rowOld[8] = {0,0,0,0,0,0,0,0};
int			row[8] = {0,0,0,0,0,0,0,0};
int			col = spacingH;
int			zeroPoint[8] = {0,0,0,0,0,0,0,0};
int			cumulativeDisplaySum[8] = {0,0,0,0,0,0,0,0}; 
int			displayedValues[NumChannels][2*EXP_LENGTH*60*SR];

void initAllegro();
void initScreen(int channelNumber);
void updateDisplay(int currentDisplay, int channelNumber);

using namespace std;
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

	ofstream file;
	string filename;
	FILE * pFile;

int main(void)

{
	if( __cplusplus == 201103L ) std::cout << "C++11\n" ;
	else if( __cplusplus == 199711L ) std::cout << "C++98\n" ;
	else std::cout << "pre-standard C++\n" ;
	
	int32       error=0;
	char        errBuff[2048]={'\0'};
	TaskHandle  AItaskHandle=0;
	TaskHandle	AOtaskHandle=0;
	float64		out[SR];
	float64		zero[1]={0};

	int keyval;

	int j = 0;
	for(;j<SR;j++){
		if (j<SR/10)
		out[j] = 2;
		else if (j<SR)
		out[j] = 0;
	}

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	//Analog Out
	DAQmxErrChk (DAQmxCreateTask("",&AOtaskHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOtaskHandle,"Dev1/ao0","",-10,10,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOtaskHandle,"",SR,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SR));

	//Analog In
	DAQmxErrChk (DAQmxCreateTask("",&AItaskHandle));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(AItaskHandle,"Dev1/ai0:1","",DAQmx_Val_RSE,min,max,DAQmx_Val_Volts,NULL));

	DAQmxErrChk (DAQmxCfgSampClkTiming(AItaskHandle,"",SR,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SAMPS_PER_CHAN_PER_CALLBACK));
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(AItaskHandle,DAQmx_Val_Acquired_Into_Buffer,SAMPS_PER_CHAN_PER_CALLBACK,0,EveryNCallback,NULL));
	DAQmxErrChk (DAQmxRegisterDoneEvent(AItaskHandle,0,DoneCallback,NULL));
	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteAnalogF64(AOtaskHandle,SR,0,10.0,DAQmx_Val_GroupByChannel,out,NULL,NULL));

	printf("Press enter to begin experiment.\n");
	getchar();

	initAllegro();
	for(int nc=0;nc<NumChannels;nc++){
	initScreen(nc);
	}

	cout <<"Please input a file name"<<endl;
    getline (cin, filename);
	filename += ".bin";

	printf("Press any key to begin data acquisition.\n");
	getchar();
	clear_keybuf();

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(AOtaskHandle));
	DAQmxErrChk (DAQmxStartTask(AItaskHandle));

	printf("Acquiring samples continuously. Press q to quit \n");
	
	int c='\0';
	while(c != 'q'){

		if (c=='z') {
			printf("Zooming in on Y-axis. \r");
			scaleV = scaleV*0.5;
		}
		if (c=='o') {
			printf("Zooming out of Y-axis. \r");
			scaleV = scaleV/0.5;
		}
		c = getchar();
		//putchar(c);
	}

	printf("End of program, press Enter key to quit.\n");
	getchar();

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( AItaskHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(AItaskHandle);
		DAQmxClearTask(AItaskHandle);
	}

	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);

	if( AOtaskHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxErrChk (DAQmxWriteAnalogF64(AOtaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,zero,NULL,NULL));
		DAQmxStopTask(AOtaskHandle);
		DAQmxClearTask(AOtaskHandle);
	}

	printf("Tasks stopped. Press Enter key to close the program.\n");
	getchar();
	return 0;
}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	int32       error=0;
	char        errBuff[2048]={'\0'};
	static int  totalRead=0;
	int32       read=0;
	int16		data[BUFFER_SIZE];
	int16		n = 0;
	float		disp = 0;

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/
	DAQmxErrChk (DAQmxReadBinaryI16(taskHandle,SAMPS_PER_CHAN_PER_CALLBACK,10.0,DAQmx_Val_GroupByScanNumber,data,BUFFER_SIZE,&read,NULL));

	if(read>0) {
		pFile = fopen(filename.c_str(), "ab");
		fwrite (data, sizeof(char), BUFFER_SIZE, pFile); 
		fclose (pFile);
		fflush(stdout);

		for(int nc = 0;nc<NumChannels;nc++){
			n = data[nc];
			for(double i=0; n > 0; i++){
				if(n % 2 == 1) {
					disp += pow(2, i);
				}
				n /= 2;
			}

			disp = disp*(max-min)/65536;
			row[nc] = (int) (zeroPoint[nc]-disp*scaleV);
			displayedValues[nc][displayUpdateNumber] = disp;
			printf("Acquired %d samples. Total %d. Current Scale %f \r",read,totalRead+=read,scaleV);
			updateDisplay(disp,nc);
			cumulativeDisplaySum[nc] += disp;
		}
		displayUpdateNumber++;
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
void initScreen(int channelNumber){
	//name the channels, add arguments to account for screen changing to next 
	rect(bmp,spacingH,spacingV*(channelNumber+1)+graphWidth*(channelNumber),COL-spacingH,(spacingV+graphWidth)*(channelNumber+1),makecol(0,0,0));
	
	int width = text_length(font,"Real Time acquisition of Channel 0 Data");
	int height = text_height(font);
	textout_centre_ex(bmp, font, "Real Time acquisition of Channel 0 Data", COL/2, spacingV*(channelNumber+1)+graphWidth*channelNumber+height,makecol(0, 0, 255),-1);

	blit(bmp,screen,0,0,0,0,COL,ROW);
	zeroPoint[channelNumber] = ROW*(2*channelNumber+1)/(2*NumChannels)+spacingV*(NumChannels);

}
void updateDisplay(int currentDisplay, int channelNumber){

		fastline(bmp,col-1,rowOld[channelNumber],col,row[channelNumber],makecol(0,0,0));
		blit(bmp,screen,0,0,0,0,COL,ROW);
		if(col<COL-spacingH){
			col=col+1;
		} else {
		clear_to_color(bmp,makecol(200,200,200));
		clear_to_color(screen,makecol(200,200,200));
		col = spacingH;
		//need to init screen again
		}
		rowOld[channelNumber] = row[channelNumber];
}