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

#define FPS			5				// VideoWriter FPS
#define ISCOLOR		0				// VideoWriter color [0:grayscale, 1:color]


using namespace cv;
using namespace std;

//#using <System.dll>
//using namespace System;
//using namespace System::IO::Ports;


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

	/*
	// =================================================================================================================
	// Initialization of serial port: COM7 (Lower USB port on front panel of computer)
	// =================================================================================================================
	static bool _continue;
    SerialPort^ serialPort;

    String^ message;
    StringComparer^ stringComparer = StringComparer::OrdinalIgnoreCase;

    // Create a new SerialPort object with default settings.
    serialPort = gcnew SerialPort("COM7");

    // Allow the user to set the appropriate properties.
    serialPort->BaudRate = 230400;
    serialPort->Parity = Parity::None;
    serialPort->DataBits = 8;
    serialPort->StopBits = StopBits::One;
    serialPort->Handshake = Handshake::None;

    // Set the read/write timeouts
    serialPort->ReadTimeout = 500;
    serialPort->WriteTimeout = 500;
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



	// =================================================================================================================
	// Obtain frames from TrackCam and store pointer to image in buf
	// =================================================================================================================
	int lastPicNr = 0, i = 0, tempCounter = 0;
	unsigned char * buf[NUMIMAGES];
	for(i=0;i<NUMIMAGES;i++)
	{
		buf[i] = (unsigned char *) malloc(width*height);
	}

	// Turning ExSync Enable ON to start receiving ExSync Trigger
	nExSyncEnable		= 1;
	if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	printf("\r\nPress USER Button to start sequence.\r\n");


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

    VideoWriter writer2("video.avi", codec, fps, size2, isColor);
	writer2.open("video.avi", codec, fps, size2, isColor);

	// Check to see if VideoWriter properties are usable
	if (writer2.isOpened()) 
	{
		printf("\r\nStarting to write frames to AVI file.\r\n");
        for(i=0;i<NUMIMAGES;i++) {

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



	// =================================================================================================================
	// Freeing the grabber resource
	// =================================================================================================================
	Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
	Fg_FreeMemEx(fg,pMem0);
	Fg_FreeGrabber(fg);

	return FG_OK;
}
