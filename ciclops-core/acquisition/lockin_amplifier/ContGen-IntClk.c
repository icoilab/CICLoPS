/*********************************************************************
*
* ANSI C Example program:
*    ContGen-IntClk.c
*
* Example Category:
*    AO
*
* Description:
*    This example demonstrates how to output a continuous periodic
*    waveform using an internal sample clock.
*
* Instructions for Running:
*    1. Select the Physical Channel to correspond to where your
*       signal is output on the DAQ device.
*    2. Enter the Minimum and Maximum Voltage Ranges.
*    3. Enter the desired rate for the generation. The onboard sample
*       clock will operate at this rate.
*    4. Select the desired waveform type.
*    5. The rest of the parameters in the Waveform Information
*       section will affect the way the waveform is created, before
*       it's sent to the analog output of the board. Select the
*       number of samples per cycle and the total number of cycles to
*       be used as waveform data.
*
* Steps:
*    1. Create a task.
*    2. Create an Analog Output Voltage channel.
*    3. Define the update Rate for the Voltage generation.
*       Additionally, define the sample mode to be continuous.
*    4. Write the waveform to the output buffer.
*    5. Call the Start function.
*    6. Wait until the user presses the Stop button.
*    7. Call the Clear Task function to clear the Task.
*    8. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal output terminal matches the Physical
*    Channel I/O Control. For further connection information, refer
*    to your hardware reference manual.
*
*********************************************************************/

#include <NIDAQmx.h>
#include <stdio.h>
#include <math.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else


//#define	buff_size freq*640
//#define SR 1

#define	freq 211
#define SR 42200

//#define freq 209
//#define SR 41800
#define PI	3.1415926535

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

int main(void)
{
	int32       error=0;
	TaskHandle  taskHandle=0;
	float64     data[SR];
	char        errBuff[2048]={'\0'};
	int         i=0;

	float64		zero[1]={0};
	for(;i<SR;i++)
		data[i] = 9.5*sin((double)i*2*freq*PI/SR);

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle,"Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"",SR,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SR));

	DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle,0,DoneCallback,NULL));

	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle,SR,0,10.0,DAQmx_Val_GroupByChannel,data,NULL,NULL));

	/************************			*********************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	printf("Generating voltage continuously. Press Enter to interrupt\n");
	getchar();

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,zero,NULL,NULL));
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
	}
	if( DAQmxFailed(error) )
	printf("DAQmx Error: %s\n",errBuff);
	printf("Tasks stopped. Press Enter key to close the program.\n");
	getchar();

	printf("End of program, press Enter key to quit\n");
	getchar();
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
