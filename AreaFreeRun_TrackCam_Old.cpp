////////////////////////////////////////////////////////////////////////////
// ME4 Frame grabber example
//
//
//
// File:	AreaFreeRun.cpp
//
// Copyrights by Silicon Software 2002-2010
////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <conio.h>
#include "pfConfig.h"

#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <time.h>

#include "board_and_dll_chooser.h"

#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <math.h>


#define NUMTESTS	5				// Number of tests to run and record
#define NUMIMAGES	1000			// Number of images to grab in one sequence
#define WIDTH		104				// 4-8192 in increments of 4 -> TrackCam max of 1024
#define HEIGHT		175				// 1-8192 in increments of 1 -> TrackCam max of 1024
#define CAMTYP		108				// 8: 8-bit Single Tap, 10: 10-bit Single Tap, 12: 12-bit Single Tap, 14: 14-bit Single Tap, 16: 16-bit Single Tap, 
									// 108: 8-bit Dual Tap, 110: 10-bit Dual Tap, 112: 12-bit Dual Tap

#define EXPOSURE	0.199			// Exposure of TrackCam (ms)
#define FRAMETIME	0.57			// Frame time of TrackCam (ms)

#define TRIGMODE	ASYNC_TRIGGER	// 0: Free Run, 1: Grabber Controlled, 2: Extern Trigger, 4: Software Trigger
#define EXPOSUREMS	20				// Exposure used for frame grabber (not sure what it really does - made as small as possible)
#define TRIGSOURCE	0				// 0-3: Trigger Source 0-3 (respectively)

#define BUFLEN		45000			// Buffer length allowable on computer for frame grabber storage
#define nBoard		0				// Board number
#define NCAMPORT	PORT_A			// Port (PORT_A / PORT_B)

#define FPS			1750			// VideoWriter FPS
#define ISCOLOR		0				// VideoWriter color [0:grayscale, 1:color]

#define PULSETIME	690				// Pulsetime for piezoelectric disk, [us]

#define PHASE 		0				// droplet contact phase relationship to vertical bouncing, 0-100
#define AMPL 		11				// shaking amplitude, [m/s^2]
#define FREQ 		32				// shaking frequency, [Hz]
#define FIXEDDIST	4.95			// distance from bottom of nozzle to top of fluid bath without shaking, [mm]
#define GRAVITY		9.81			// gravitational acceleration, [m/s^2]
#define ADJUST		(-5)				// frames to adjust for droplet trajectory
									// positive: drop earlier


using namespace cv;
using namespace std;

#using <System.dll>
using namespace System;
using namespace System::IO::Ports;


int ErrorMessage(Fg_Struct *fg)
{
	int			error	= Fg_getLastErrorNumber(fg);
	const char*	err_str = Fg_getLastErrorDescription(fg);
	fprintf(stderr,"Error: %d : %s\n",error,err_str);
	return error;
}

int ErrorMessageWait(Fg_Struct *fg)
{
	int			error	= ErrorMessage(fg);
	printf (" ... press ENTER to continue\n");
	getchar();
	return error;
}

const char *dll_list_me3[] = {
#ifdef MEUNIX
		"libSingleAreaGray.so",
		"libDualAreaRGB.so",
		"libMediumAreaGray.so",
		"libMediumAreaRGB.so",
#else
		"SingleAreaGray.dll",
		"DualAreaRGB.dll",
		"MediumAreaGray.dll",
		"MediumAreaRGB.dll",
#endif
		""
};

const char *dll_list_me3xxl[] = {
#ifdef MEUNIX
		"libDualAreaGray12XXL.so",
		"libDualAreaRGB36XXL.so",
		"libMediumAreaGray12XXL.so",
		"libMediumAreaRGBXXL.so",
#else
		"DualAreaGray12XXL.dll",
		"DualAreaRGB36XXL.dll",
		"MediumAreaGray12XXL.dll",
		"MediumAreaRGBXXL.dll",
#endif
		""
};


const char *dll_list_me4_dual[] = {
#ifdef MEUNIX
	"libDualAreaGray16.so",
		"libDualAreaRGB48.so",
		"libMediumAreaGray16.so",
		"libMediumAreaRGB48.so",
#else
		"DualAreaGray16.dll",
		"DualAreaRGB48.dll",
		"MediumAreaGray16.dll",
		"MediumAreaRGB48.dll",
#endif
		""
};

const char *dll_list_me4_single[] = {
#ifdef MEUNIX
	"libSingleAreaGray16.so",
	"libSingleAreaRGB48.so",
#else
	"SingleAreaGray16.dll",
	"SingleAreaRGB48.dll",
#endif
	""
};


int main(int argc, char* argv[], char* envp[]){

	// =================================================================================================================
	// Initialization of serial port: COM7 (Lower USB port on front panel of computer) to talk to PIC
	// =================================================================================================================
	static bool _continue;
    SerialPort^ serialPort7;

    String^ message;
    StringComparer^ stringComparer = StringComparer::OrdinalIgnoreCase;

    // Create a new SerialPort object with default settings.
    serialPort7 = gcnew SerialPort("COM7");

    // Allow the user to set the appropriate properties.
    serialPort7->BaudRate = 230400;
    serialPort7->Parity = Parity::None;
    serialPort7->DataBits = 8;
    serialPort7->StopBits = StopBits::One;
    serialPort7->Handshake = Handshake::RequestToSend;

    // Set the read/write timeouts
    serialPort7->ReadTimeout = 500;
    serialPort7->WriteTimeout = 500;

	// Open the serial port
	serialPort7->Open();


	/*
	// =================================================================================================================
	// Initialization of serial port: COMX to talk to PPOD
	// =================================================================================================================
	static bool _continue;
    SerialPort^ serialPortX;

    String^ message;
    StringComparer^ stringComparer = StringComparer::OrdinalIgnoreCase;

    // Create a new SerialPort object with default settings.
    serialPortX = gcnew SerialPort("COMX");

    // Allow the user to set the appropriate properties.
    serialPortX->BaudRate = 9600;
    serialPortX->Parity = Parity::None;
    serialPortX->DataBits = 8;
    serialPortX->StopBits = StopBits::One;
    serialPortX->Handshake = Handshake::None;

    // Set the read/write timeouts
    serialPortX->ReadTimeout = 500;
    serialPortX->WriteTimeout = 500;

	// Open the serial port
	serialPortX->Open();
	*/



	/*
	// =================================================================================================================
	// Camera configuration
	// =================================================================================================================
	int numOfPorts, error, i, numProperties;
	const char *pErr;
	//long width, height;
	float fVal;

	//port of camera
	//camPort = 0;

	
	// ************ Camera configuration **************************** //

	// Init of pfConfig
	numOfPorts = pfGetNumPort();


	// Open Camera on port camPort (Port0)
	printf("Open camera on port %d...\n", nBoard);
	error = pfOpenPort(nBoard, true);
	if(error != 0){
		pErr = pfGetErrorMsg(nBoard);
		printf("Opening camera port failed: %s \nSorry we must quit.\n", pErr);
		return 0;
	}

	printf("Camera configuration...");


	// Set ROI and offset
	//width = 104;
	//height = 175;

	error = pfSetCameraPropertyI(nBoard, "Window.W", WIDTH);
	error = pfSetCameraPropertyI(nBoard, "Window.X", 0);			//no offset
	error = pfSetCameraPropertyI(nBoard, "Window.H", HEIGHT);
	error = pfSetCameraPropertyI(nBoard, "Window.Y", 0);			// no offset


	// Attempt to set Free Running Trigger
	int pp = 1;
	error = pfSetCameraPropertyI(nBoard, "Trigger.Source.Free", pp);	


	// Set exposure time
	fVal = EXPOSURE; //ms
	error = pfSetCameraPropertyF(nBoard, "ExposureTime", fVal);

	// Set frame time for 1,750 fps
	fVal = FRAMETIME; //ms
	error = pfSetCameraPropertyF(nBoard, "FrameTime", fVal);

	
	// Close camera
	printf("\nClose camera on port %d...\n", nBoard);
	error = pfClosePort(nBoard);
	*/

	

	// =================================================================================================================
	// Initialization of frame grabber and camera properties
	// =================================================================================================================
	int nr_of_buffer	=	BUFLEN;						// Number of memory buffer
	//int nBoard			=	selectBoardDialog();	// Board number
	int nCamPort		=	NCAMPORT;					// Port (PORT_A / PORT_B)
	int status = 0;
	Fg_Struct *fg;
	
	const char *dllName = NULL;

	int boardType = 0;

	boardType = Fg_getBoardType(nBoard);

	printf("Board ID  %d: MicroEnable III (a3)\r\n", nBoard);
	printf("\r\n=====================================\r\n\r\n");
	
	//dllName = selectDll(boardType, dll_list_me3, dll_list_me3xxl, dll_list_me4_dual, dll_list_me4_single);
	dllName = "SingleAreaGray.dll";		//Uncomment to selectDLL



	// =================================================================================================================
	// Initialization of the microEnable frame grabber
	// =================================================================================================================
	if((fg = Fg_Init(dllName,nBoard)) == NULL) {
		status = ErrorMessageWait(fg);
		return status;
	}
	fprintf(stdout,"Init Grabber: ok\n");



	// =================================================================================================================
	// Setting the image size and camera format
	// =================================================================================================================
	int width	= WIDTH;
	int height	= HEIGHT;
	int CamTyp	= CAMTYP;

	if (Fg_setParameter(fg,FG_WIDTH,&width,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}
	if (Fg_setParameter(fg,FG_HEIGHT,&height,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}
	if (Fg_setParameter(fg,FG_CAMERA_LINK_CAMTYP,&CamTyp,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}

	fprintf(stdout,"Set Image Size on port %d (w: %d,h: %d): ok\n",nCamPort,width,height);
	fprintf(stdout,"Set CameraLink Format on port %d to %d (108 == 8-bit Dual Tap): ok\n",nCamPort,CamTyp);



	// =================================================================================================================
	// Memory allocation
	// =================================================================================================================
	int format;
	Fg_getParameter(fg,FG_FORMAT,&format,nCamPort);
	size_t bytesPerPixel = 1;
	switch(format){
	case FG_GRAY:	bytesPerPixel = 1; break;
	case FG_GRAY16:	bytesPerPixel = 2; break;
	case FG_COL24:	bytesPerPixel = 3; break;
	case FG_COL32:	bytesPerPixel = 4; break;
	case FG_COL30:	bytesPerPixel = 5; break;
	case FG_COL48:	bytesPerPixel = 6; break;
	}
	size_t totalBufSize = width*height*nr_of_buffer*bytesPerPixel;
	dma_mem *pMem0;
	if((pMem0 = Fg_AllocMemEx(fg,totalBufSize,nr_of_buffer)) == NULL){
		status = ErrorMessageWait(fg);
		return status;
	} else {
		fprintf(stdout,"%d framebuffer allocated for port %d: ok\n",nr_of_buffer,nCamPort);
	}

	unsigned char * buf[NUMIMAGES];
	for(int i=0;i<NUMIMAGES;i++)
	{
		buf[i] = (unsigned char *) malloc(width*height);
	}



	/*
	// =================================================================================================================
	// Setting the trigger and grabber mode for Free Run
	// =================================================================================================================
	int	nTriggerMode		= FREE_RUN;

	if(Fg_setParameter(fg,FG_TRIGGERMODE,&nTriggerMode,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	*/
	
	// =================================================================================================================
	// Setting the trigger and grabber mode for Extern Trigger
	// =================================================================================================================
	int		nTriggerMode		= TRIGMODE;	
	int	    nExposureInMicroSec	= EXPOSUREMS;
	int		nTrgSource			= TRIGSOURCE;				//0-3: Trigger Source 0-3 (respectively)
	int		nExSyncEnable		= 0;						//0: OFF, 1: ON

	if(Fg_setParameter(fg,FG_TRIGGERMODE,&nTriggerMode,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	if(Fg_setParameter(fg,FG_EXPOSURE,&nExposureInMicroSec,nCamPort)<0)	{ status = ErrorMessageWait(fg); return status;}
	if(Fg_setParameter(fg,FG_TRIGGERINSRC,&nTrgSource,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	

	if((Fg_AcquireEx(fg,nCamPort,GRAB_INFINITE,ACQ_STANDARD,pMem0)) < 0){
		status = ErrorMessageWait(fg);
		return status;
	}

	
	/*
	// =============================================================================================
	// Calculate DELAYTIME to delay droplet creation to achieve phase relationship
	// =============================================================================================

	// Determine the vertical sine wave amplitude based on the shaking amplitude/frequency
	//		rho = A*w^2/g
	//		Sine_Amplitude = rho*g/w^2
	//		Sine_Amplitude = rho*g/(2*pi*f)^2
	double SineAmplitude = (AMPL) / (4.0*M_PI*M_PI*FREQ*FREQ);
	//printf("\nSine Amplitude (mm): %f\n", SineAmplitude*1000.0);

	// Calculate the height the droplet will fall before contacting the bath
	double DropHeight = (FIXEDDIST/1000.0) - SineAmplitude*sin( 2.0*M_PI*(PHASE/100.0) );
	//printf("\nDrop Height (mm): %f\n", DropHeight*1000.0);

	// Once know the drop height, determine the drop time
	//		H = (1/2)*g*t^2
	double DropTime, Temp;
	Temp = (2.0*DropHeight)/GRAVITY;
	DropTime = sqrt( Temp );
	//printf("\nDrop Time (sec): %f\n", DropTime);
	
	// Calculate the corresponding number of frames droplet will be falling before contacting sine wave
	int FramesFalling = FPS * DropTime;
	//printf("\nFrames Falling: %d\n", FramesFalling);

	// With number of frames droplet will be falling, determine how many frames from the beginning to create the droplet
	DelayFrames = (PHASE/100.0)*(FPS/FREQ) - FramesFalling - 0.25*(FPS/FREQ) - ADJUST;

	while (DelayFrames < 0) {
		DelayFrames = DelayFrames + (FPS/FREQ);
	}
	//printf("\nDelay Frames: %d\n", DelayFrames);

	// Convert DelayFrames to DELAYTIME based on fps of camera
	float DELAYTIME = DelayFrames/FPS;
	*/



	// =================================================================================================================
	// Start of automated testing - Increment DELAYTIME by 5 ms each run, for one period of shaking
	// ================================================================================================================
	int currentTest = 0;
	//while( currentTest < NUMTESTS ) {
	float DELAYTIME = 0;
	while( DELAYTIME < (FPS/FREQ) ) {

		// =================================================================================================================
		// Send serial command to PPOD to adjust shaking parameters
		// =================================================================================================================



		// =================================================================================================================
		// Send serial command for [NUMIMAGES, FPS, PULSETIME, DELAYTIME]
		// =================================================================================================================
		serialPort7->WriteLine(String::Format("{0}, {1}, {2}, {3}", NUMIMAGES, FPS, PULSETIME, DELAYTIME) );
		Console::WriteLine(String::Format("[NUMIMAGES, FPS, PULSETIME, DELAYTIME]: {0}, {1}, {2}, {3}", NUMIMAGES, FPS, PULSETIME, DELAYTIME) );



		// =================================================================================================================
		// When PIC receives serial command, will create ExSync Trigger and delay droplet production
		// =================================================================================================================

	

		// =================================================================================================================
		// Obtain frames from TrackCam using ExSycn Trigger and store pointer to image in buf
		// =================================================================================================================
		int lastPicNr = 0, tempCounter = 0;

		// Turning ExSync Enable ON to start receiving ExSync Trigger
		nExSyncEnable		= 1;
		if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}

		// Obtain frame number and store pointer to image in buf[]
		while((lastPicNr<NUMIMAGES)) {
			lastPicNr = Fg_getLastPicNumberBlockingEx(fg,lastPicNr+1,nCamPort,10,pMem0); //Waits a maximum of 10 seconds

			if(lastPicNr == 1){ 
				printf("\r\nFrames starting to be grabbed from TrackCam.\r\n");
			}

			if(lastPicNr <0){
				status = ErrorMessageWait(fg);
				Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
				Fg_FreeMemEx(fg,pMem0);
				Fg_FreeGrabber(fg);
				return status;

			}

			buf[tempCounter] = (unsigned char *) Fg_getImagePtrEx(fg,lastPicNr,0,pMem0);
			tempCounter++;
		}
		printf("Frames finished grabbing from TrackCam.\r\n");
		printf("\r\nSuccessful Frames Grabbed: %d/%d\r\n", tempCounter, NUMIMAGES);

		// Turning ExSync Enable OFF since finished receiving triggers
		nExSyncEnable		= 0;
		if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}



		// =================================================================================================================
		// Writing avi file using each frame
		// =================================================================================================================
		long lSize;
		char * buffer;
		size_t result;

		// Define properties for VideoWrite
		int isColor = ISCOLOR;
		int fps     = FPS;
		int frameW  = WIDTH;
		int frameH  = HEIGHT;
		Size size2 = Size(frameW, frameH);
		int codec = CV_FOURCC('H', 'F', 'Y', 'U'); 
		//Usable: HFYU, MJPG(missing some information) 
		//Not: MPEG, MPG4, YUV8, YUV9, _RGB, 8BPS, DUCK, MRLE, PIMJ, PVW2, RGBT, RLE4, RLE8, RPZA

		// Create unique name for each AVI object
		char filename[64];
		sprintf(filename, "%fDelayTime_%dFPS_%dPulseTime.txt", DELAYTIME, FPS, PULSETIME);

		VideoWriter writer2(filename, codec, fps, size2, isColor);
		writer2.open(filename, codec, fps, size2, isColor);

		// Check to see if VideoWriter properties are usable
		if (writer2.isOpened()) 
		{
			printf("\r\nStarting to write frames to AVI file.\r\n");
			for(int i=0;i<NUMIMAGES;i++) {

				// Load frame from buf[i] to TempImg for writing to video
				Mat TempImg(height, width, CV_8UC1, buf[i]);

				// Flip image vertically to compensate for camera upside down
				Mat FlippedImg;
				flip(TempImg, FlippedImg, 0);

				// Write the frame to the video
				writer2.write(FlippedImg);

			}
		}
		else {
			cout << "ERROR while opening" << endl;
		}
		printf("AVI file written.\r\n");

		// Increment DELAYTIME by 5 ms
		DELAYTIME = DELAYTIME + 0.005;

		// Increment current test number
		currentTest++;
	}

	

	// =================================================================================================================
	// Freeing the grabber resource and closing the serial ports
	// =================================================================================================================
	Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
	Fg_FreeMemEx(fg,pMem0);
	Fg_FreeGrabber(fg);

	serialPort7->Close();
	//serialPortX->Close();



	return FG_OK;
}
