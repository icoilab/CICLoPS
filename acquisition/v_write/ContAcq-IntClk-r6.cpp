/*********************************************************************

*********************************************************************/
#define ALLEGRO_USE_CONSOLE
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <allegro.h>
#include <NIDAQmx.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

bool inCallBack = false;
bool rescaleRequested = false;
const int NumChannels = 1; 
#define ROW 640
#define COL 692
#define MAX 5
#define MIN -5
#define SR 100
#define SAMPS_PER_CHAN_PER_CALLBACK SR
#define	BUFFER_SIZE 10*SAMPS_PER_CHAN_PER_CALLBACK*NumChannels
#define	EXP_LENGTH 30
	//in minutes		

int displayUpdateNumber = 0;
int screenCycles = 0;
int spacingV = 32;
int spacingH = 48;
int c='\0';
int chan = 0;
int ccol = 0;

BITMAP		*bmp;
PALETTE		pal;

float 		scaleV[8];
//float		scaleH = (COL-2*spacingH)/SR;
float		graphWidth = (ROW-spacingV*(NumChannels+1))/NumChannels;
float		displayedValues[NumChannels][2*EXP_LENGTH*60*SR/SAMPS_PER_CHAN_PER_CALLBACK];
int			col = spacingH+1;
int			zeroPoint[8] = {0,0,0,0,0,0,0,0};
float		cumulativeSum[8] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
float		cumulativeAvg[8] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
double		graphMarkers[NumChannels][5];

void initAllegro();
void initScreen(int channelNumber);
void updateDisplay(int channelNumber);
void redrawTrace(int channelNumber, int ccol);
void centerTrace(int channelNumber);

//void fill(int NumberOfChannels, int value, int &array); // figure out how to do this some other time

using namespace std;
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

	ofstream file;
	string filename;
	FILE * pFile;

int main(){

	int32       error=0;
	char        errBuff[2048]={'\0'};
	TaskHandle  AItaskHandle=0;
	TaskHandle	AOtaskHandle=0;
	float64		out[SR];
	float64		zero[1]={0};
	
	for (int i=0;i<NumChannels;i++){
		scaleV[i] = (ROW-spacingV*(NumChannels+1))/(NumChannels*(MAX-MIN));
	}

	int keyval;

	int j = 0;
	for(;j<SR;j++){
		if (j<SR/10)
		out[j] = 2;
		else if (j<SR)
		out[j] = 0;
	}

	for(;j<NumChannels;j++){
		displayedValues[j][0]=0;
	}


	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	//Analog Out
	DAQmxErrChk (DAQmxCreateTask("",&AOtaskHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOtaskHandle,"Dev1/ao0","",-10,10,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOtaskHandle,"",SR,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SR));

	const char* physicalChannel = "Dev1/ai0";
		 if (NumChannels == 2) physicalChannel = "Dev1/ai0:1";
	else if (NumChannels == 3) physicalChannel = "Dev1/ai0:2";
	else if (NumChannels == 4) physicalChannel = "Dev1/ai0:3";
	else if (NumChannels == 5) physicalChannel = "Dev1/ai0:4";
	else if (NumChannels == 6) physicalChannel = "Dev1/ai0:5";
 	else if (NumChannels == 7) physicalChannel = "Dev1/ai0:6";
	else if (NumChannels == 8) physicalChannel = "Dev1/ai0:7";

	DAQmxErrChk (DAQmxCreateTask("",&AItaskHandle));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(AItaskHandle,physicalChannel,"",DAQmx_Val_RSE,MIN,MAX,DAQmx_Val_Volts,NULL));

	DAQmxErrChk (DAQmxCfgSampClkTiming(AItaskHandle,"",SR,DAQmx_Val_Rising,DAQmx_Val_ContSamps,BUFFER_SIZE));
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
	zeroPoint[nc] = (graphWidth/2)*(2*nc+1)+spacingV*(nc+1);
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

	printf("Acquiring samples continuously.\n \
		   c: center trace about cumulative average\n \
		   z: center and zoom in on y axis.\n \
		   o: center and zoom out of y axis\n \
		   q: quit data acquisition\n");
	
	while(c != 'q'){

		if (c=='c') {
			
			while ((std::cout << "Type channel number you want to center\n") && !(std::cin >> chan) && !inCallBack) {
					std::cin.clear();
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					std::cout << "That's not an active channel number";
			}
			
			if(!inCallBack){
				printf("Centering channel %d. \n",chan);
				rescaleRequested = true;
			}

		} else if (c=='z') {

			while ((std::cout << "Type channel number you want to center\n") && !(std::cin >> chan) && !inCallBack) {
					std::cin.clear();
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					std::cout << "That's not an active channel number";
			}

			if(!inCallBack){
				printf("Zooming in on Y-axis of chan %d. \n", chan);
				scaleV[chan] = scaleV[chan]/0.5;
				rescaleRequested = true;
			}
			
		} else if (c=='o') {

			while ((std::cout << "Type channel number you want to center\n") && !(std::cin >> chan) && !inCallBack) {
					std::cin.clear();
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					std::cout << "That's not an active channel number";
			}

			if(!inCallBack){
			printf("Zooming out of Y-axis of chan %d. \n", chan);
			scaleV[chan] = scaleV[chan]*0.5;
			rescaleRequested = true;
			}
			
		} else {
			c = '\0';
		}

		fflush(stdin);
		c = '\0';
	
		if (!inCallBack) c = getchar();
	}

	printf("End of program, wait for tasks to close.\n");
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
	//printf("in call back\n");
	inCallBack = true;
	int32       error=0;
	char        errBuff[2048]={'\0'};
	static int  totalRead=0;
	int32       read=0;
	int16		data[2*SAMPS_PER_CHAN_PER_CALLBACK*NumChannels];

	int16		n = 0;
	uint16_t	temp = 0;
	int			nc = 0;
	int			dec=0;
	bool		nIsNeg = false;
	int			nPointAvg = 20;
	int			j = 0;

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/
	DAQmxErrChk (DAQmxReadBinaryI16(taskHandle,SAMPS_PER_CHAN_PER_CALLBACK,10.0,DAQmx_Val_GroupByScanNumber,data,2*SAMPS_PER_CHAN_PER_CALLBACK*NumChannels,&read,NULL));

	if(read>0) {
		pFile = fopen(filename.c_str(), "ab");
		fwrite (data, sizeof(char), 2*SAMPS_PER_CHAN_PER_CALLBACK*NumChannels, pFile); 
		fclose (pFile);
		fflush(stdout);

		for(nc=0;nc<NumChannels;nc++){
			dec = 0;
			for(j=0;j<nPointAvg;j++){	
			n = data[nc+(NumChannels*SR/nPointAvg)*j];
			temp = (unsigned int) n>>15;
			if (temp > 1){
				n = ~n;
				n--;
				nIsNeg = true;
			}

			for(double i=0; n != 0; i++){
					if(n % 2 == 1) {
						dec += pow(2, i);
					}
					n /= 2;
				}
			}

			if (nIsNeg) dec = dec*(-1);
			nIsNeg = false;
			displayedValues[nc][displayUpdateNumber] = (float) dec*(MAX-MIN)/65536/nPointAvg;

			cumulativeSum[nc] += displayedValues[nc][displayUpdateNumber];
			printf("Acquired %d samples. Total %d. Current value on channel 0 - %f \r",read,totalRead+=read,displayedValues[0][displayUpdateNumber]);
			
			if (rescaleRequested){
				centerTrace(chan);
				initScreen(chan);
				ccol = col;
				redrawTrace(chan,ccol);
				rescaleRequested = false;										// clear rescale request flag
			}
			updateDisplay(nc);
		}
		displayUpdateNumber++;

		if(col<COL-spacingH){
			col++;
		} else {
		clear_to_color(bmp,makecol(200,200,200));
		clear_to_color(screen,makecol(200,200,200));
		for (int nc=0;nc<NumChannels;nc++) initScreen(nc);
		col = spacingH+1;
		screenCycles++;
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

	inCallBack = false;
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
	//Graph titles & bounding boxes
	//                        0123456789*123456789*123456789*12345678

	std::string traceTitle = "Real Time acquisition of Channel X Data";
	std::ostringstream s;
	s << channelNumber;
	std::string traceChan(s.str());
	
	traceTitle.replace(33,1,traceChan);


	//y-axis
	std::ostringstream t;
	rectfill(bmp,0,spacingV*(channelNumber+1)+graphWidth*(channelNumber),spacingH-1,(spacingV+graphWidth)*(channelNumber+1),makecol(200,200,200));
	
	for (int i = -2;i<3;i++){
	graphMarkers[channelNumber][i+2] = cumulativeAvg[channelNumber] - graphWidth/4*i/scaleV[channelNumber];
	t << graphMarkers[channelNumber][i+2];
	std::string graphMarker(t.str());
	if (graphMarker.length()>5) graphMarker.erase(5,graphMarker.end()-(graphMarker.begin()+4));
	textout_right_ex(bmp, font, graphMarker.c_str(), spacingH-4, spacingV*(channelNumber+1)+graphWidth*channelNumber+(graphWidth/4-2)*(i+2),makecol(0, 0, 255),-1);
	t.str(std::string());														//clear the string buffer
	}

	//graph bounding boxes
	rect(bmp,spacingH,spacingV*(channelNumber+1)+graphWidth*(channelNumber),COL-spacingH,(spacingV+graphWidth)*(channelNumber+1),makecol(0,0,0));
	rectfill(bmp,spacingH+1,spacingV*(channelNumber+1)+graphWidth*(channelNumber)+1,COL-spacingH-1,(spacingV+graphWidth)*(channelNumber+1)-1,makecol(255,255,255));
	int width = text_length(font,traceTitle.c_str());
	int height = text_height(font);
	textout_centre_ex(bmp, font, traceTitle.c_str(), COL/2, spacingV*(channelNumber+1)+graphWidth*channelNumber+height-16,makecol(0, 0, 255),-1);
	
	//grid
	
	//zeroPoint[channelNumber] = (ROW-2*spacingV)*(2*channelNumber+1)/(2*NumChannels)+spacingV*(NumChannels);
	for (int i = -4;i<5;i++){
	//fastline(bmp,spacingH,    (int) (zeroPoint[channelNumber]+(MAX-MIN)*scaleV[channelNumber]/10*i),\
	//		     COL-spacingH,(int) (zeroPoint[channelNumber]+(MAX-MIN)*scaleV[channelNumber]/10*i),\
	//			 makecol(0,255,0));
	fastline(bmp,spacingH+(COL-2*spacingH)/10*(i+5)+1,spacingV*(channelNumber+1)+graphWidth*channelNumber,\
				 spacingH+(COL-2*spacingH)/10*(i+5)+1,spacingV*(channelNumber+1)+graphWidth*(channelNumber+1),\
				 makecol(255,0,0));
	}

	if (displayUpdateNumber == 0)
	fastline(bmp,spacingH,(int) (zeroPoint[channelNumber]), COL-spacingH,(int) (zeroPoint[channelNumber]), makecol(0,0,255));
	else {fastline(bmp,spacingH,(int) (zeroPoint[channelNumber]),\
				   COL-spacingH,(int) (zeroPoint[channelNumber]),\
				 makecol(0,0,255));
	}
	blit(bmp,screen,0,0,0,0,COL,ROW);
}
void updateDisplay(int channelNumber){

		fastline(bmp,col-1,(int) (zeroPoint[channelNumber]-scaleV[channelNumber]*displayedValues[channelNumber][displayUpdateNumber-1]+scaleV[channelNumber]*cumulativeAvg[channelNumber]),\
					 col,  (int) (zeroPoint[channelNumber]-scaleV[channelNumber]*displayedValues[channelNumber][displayUpdateNumber]  +scaleV[channelNumber]*cumulativeAvg[channelNumber]),\
					 makecol(0,0,0));
		blit(bmp,screen,0,0,0,0,COL,ROW);
	
}
void redrawTrace(int channelNumber,int ccol){
	//printf("Redrawing\n");

	//rectfill(bmp,spacingH+1,spacingV*(channelNumber+1)+graphWidth*(channelNumber)+1,COL-spacingH-1,(spacingV+graphWidth)*(channelNumber+1)-1,makecol(255,255,255));

	//initScreen(channelNumber);
		int c;
		for (c=spacingH+1;c<col-1;c++){
				fastline(bmp,c-1,(int) (zeroPoint[channelNumber]-scaleV[channelNumber]*displayedValues[channelNumber][c-spacingH-1+screenCycles*(COL-2*spacingH)]+cumulativeAvg[channelNumber]*scaleV[channelNumber]),\
						     c,  (int) (zeroPoint[channelNumber]-scaleV[channelNumber]*displayedValues[channelNumber][c-spacingH  +screenCycles*(COL-2*spacingH)]+cumulativeAvg[channelNumber]*scaleV[channelNumber]  ),\
							 makecol(0,0,0));
			blit(bmp,screen,0,0,0,0,COL,ROW);
			//screenCycles is important for modifying displayUpdateNumber/c to use here. ie '-screenCycles*(COL-spacingH*2)
	}
		//printf("Made it to column %d",c);
}
void centerTrace(int channelNumber){

	cumulativeAvg[channelNumber] = cumulativeSum[channelNumber]/(displayUpdateNumber+1);//because of indexing, display update number 0 means one pixel has been displayed

}