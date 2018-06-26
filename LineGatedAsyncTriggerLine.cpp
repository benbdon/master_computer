////////////////////////////////////////////////////////////////////////////
// ME4 Frame grabber example
//
//
//
// File:	LineGatedAsyncTriggerLine.cpp
//
// Copyrights by Silicon Software 2002-2010
////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <time.h>

#include "board_and_dll_chooser.h"

#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>

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

///////////////////////////////////////////////////////////////////////
// Main function
//

const char *dll_list_me3[] = {
#ifdef MEUNIX
			"libDualLineGray.so",
			"libDualLineRGB.so",
			"libMediumLineGray.so",
			"libMediumLineRGB.so",
#else
			"DualLineGray.dll",
			"DualLineRGB.dll",
			"MediumLineGray.dll",
			"MediumLineRGB.dll",
#endif
			""
};

const char *dll_list_me3xxl[] = {
#ifdef MEUNIX
		"libDualLineGray12XXL.so",
			"libDualLineRGB36XXL.so",
			"libMediumLineGray12XXL.so",
			"libMediumLineRGBXXL.so",
#else
			"DualLineGray12XXL.dll",
			"DualLineRGB36XXL.dll",
			"MediumLineGray12XXL.dll",
			"MediumLineRGBXXL.dll",
#endif
			""
};


const char *dll_list_me4_dual[] = {
#ifdef MEUNIX
		"libDualLineGray16.so",
			"libDualLineRGB30.so",
			"libMediumLineGray16.so",
			"libMediumLineRGB48.so",
#else
			"DualLineGray16.dll",
			"DualLineRGB30.dll",
			"MediumLineGray16.dll",
			"MediumLineRGB48.dll",
#endif
			""
};

const char *dll_list_me4_single[] = {
#ifdef MEUNIX
	"libSingleLineGray16.so",
	"libSingleLineRGB48.so",
#else
	"SingleLineGray16.dll",
	"SingleLineRGB48.dll",
#endif
	""
};


int main(int argc, char* argv[], char* envp[]){

	int nr_of_buffer	=	8;			// Number of memory buffer
	int nBoard			=	selectBoardDialog();			// Board Number
	int nCamPort		=	PORT_A;		// Port (PORT_A / PORT_B)
	int MaxPics			=	100;		// Number of images to grab
	int status = 0;
	Fg_Struct *fg;

	const char *dllName = NULL;

	int boardType = 0;

	boardType = Fg_getBoardType(nBoard);

	dllName = selectDll(boardType, dll_list_me3, dll_list_me3xxl, dll_list_me4_dual, dll_list_me4_single);

	// Initialization of the microEnable frame grabber
	if((fg = Fg_Init(dllName,nBoard)) == NULL) {
		status = ErrorMessageWait(fg);
		return status;
	}
	fprintf(stdout,"Init Grabber ok\n");

	// Setting the image size
	int width	= 512;
	int height	= 512;
	if (Fg_setParameter(fg,FG_WIDTH,&width,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}
	if (Fg_setParameter(fg,FG_HEIGHT,&height,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}

	fprintf(stdout,"Set Image Size on port %d (w: %d,h: %d) ok\n",nCamPort,width,height);


	// Memory allocation
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
		fprintf(stdout,"%d framebuffer allocated for port %d ok\n",nr_of_buffer,nCamPort);
	}

	// Creating a display window for image output
	int Bits = 8;
	switch(format){
	case FG_GRAY:
	case FG_GRAY16:
		Bits = 8;
		break;
	case FG_COL24:
	case FG_COL48:
		Bits = 24;
		break;
	default:
		Bits = 8;
		break;
	};

	int nId = ::CreateDisplay(Bits,width,height);
	SetBufferWidth(nId,width,height);

	// Setting the trigger and grabber mode
	int		nImgTriggerMode		= FREE_RUN;

	if(Fg_setParameter(fg,FG_IMGTRIGGERMODE,&nImgTriggerMode,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}

	int		line_trigger_mode			= ASYNC_TRIGGER;
	double	exposure_time_in_microsec	= 250.0;
	double	periode_time_in_microsec	= 500.0;
	int		exsync_invert				= FG_NO;
	int		in_src						= 0;
	int		invert_input				= FG_NO;
	int		downscale					= 1;
	int		phase						= 1;

	if(Fg_setParameter(fg,FG_LINETRIGGERMODE,&line_trigger_mode,nCamPort)<0)	{ status = ErrorMessageWait(fg); return status;};
	if(Fg_setParameter(fg,FG_LINEEXPOSURE,&exposure_time_in_microsec,nCamPort)<0){ status = ErrorMessageWait(fg); return status;};
	if(Fg_setParameter(fg,FG_EXSYNCINVERT,&exsync_invert,nCamPort)<0)			{ status = ErrorMessageWait(fg); return status;};
	if(Fg_setParameter(fg,FG_LINETRIGGERINSRC,&in_src,nCamPort)	<0)				{ status = ErrorMessageWait(fg); return status;};
	if(Fg_setParameter(fg,FG_LINETRIGGERINPOLARITY,&invert_input,nCamPort)<0)	{ status = ErrorMessageWait(fg); return status;};
	if(Fg_setParameter(fg,FG_LINE_DOWNSCALE,&downscale,nCamPort)<0)				{ status = ErrorMessageWait(fg); return status;};
	if(Fg_setParameter(fg,FG_LINE_DOWNSCALEINIT,&phase,nCamPort)<0)				{ status = ErrorMessageWait(fg); return status;};

	fprintf(stdout,"Set Linetrigger and Grabber mode to Grabber Controlled: %f us exposure time and %f us\n",exposure_time_in_microsec,periode_time_in_microsec);

	if((Fg_AcquireEx(fg,nCamPort,GRAB_INFINITE,ACQ_STANDARD,pMem0)) < 0){
		status = ErrorMessageWait(fg);
		return status;
	}


	// ====================================================
	// MAIN LOOP
	int lastPicNr = 0;
	while((lastPicNr = Fg_getLastPicNumberBlockingEx(fg,lastPicNr+1,nCamPort,10,pMem0))< MaxPics) {
		if(lastPicNr <0){
			status = ErrorMessageWait(fg);
			Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
			Fg_FreeMemEx(fg,pMem0);
			Fg_FreeGrabber(fg);
			CloseDisplay(nId);
			return status;
		}
		::DrawBuffer(nId,Fg_getImagePtrEx(fg,lastPicNr,0,pMem0),lastPicNr,"");
	}

	// ====================================================
	// Freeing the grabber resource
	Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
	Fg_FreeMemEx(fg,pMem0);
	Fg_FreeGrabber(fg);

	CloseDisplay(nId);

	return FG_OK;
}
