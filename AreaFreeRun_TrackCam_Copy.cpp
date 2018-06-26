////////////////////////////////////////////////////////////////////////////
// ME4 Frame grabber example
//
//
//
// File:	AreaFreeRun.cpp
//
// Copyrights by Silicon Software 2002-2010
////////////////////////////////////////////////////////////////////////////


// UDP communication includes - must be before include windows.h
#include <winsock2.h>
#include <Ws2tcpip.h>

// Standard includes
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>       // std::string
#include <iostream>     // std::cout, std::ostream, std::hex
#include <sstream>      // std::stringbuf
#include <time.h>		// Sleep(milliseconds)
#include <math.h>

// Camera settings includes
//#include <stdlib.h>
//#include <conio.h>
//#include "pfConfig.h"

// Frame grabber includes
#include "board_and_dll_chooser.h"
#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>

// OpenCV includes
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


 




#define WIDTH		800				// 4-8192 in increments of 4 -> TrackCam max of 1024
#define HEIGHT		150				// 1-8192 in increments of 1 -> TrackCam max of 1024
#define CAMTYP		108				// 8: 8-bit Single Tap, 10: 10-bit Single Tap, 12: 12-bit Single Tap, 14: 14-bit Single Tap, 16: 16-bit Single Tap, 
									// 108: 8-bit Dual Tap, 110: 10-bit Dual Tap, 112: 12-bit Dual Tap

#define TRIGMODE	ASYNC_TRIGGER	// 0: Free Run, 1: Grabber Controlled, 2: Extern Trigger, 4: Software Trigger
#define EXPOSUREMS	20				// Exposure used for frame grabber (not sure what it really does - made as small as possible)
#define TRIGSOURCE	0				// 0-3: Trigger Source 0-3 (respectively)

#define BUFLEN		200				// Buffer length allowable on computer for frame grabber storage
									// Frame Buffer Memory Size: 819 million pixels [Width*Height*BUFLEN]

#define nBoard		0				// Board number
#define NCAMPORT	PORT_A			// Port (PORT_A / PORT_B)

#define ERROR_THRESHOLD		0.3		// Error threshold needed from PPOD after changing signal before will create droplet and start video capturing

#define VIDEOFPS	5				// VideoWriter FPS
#define ISCOLOR		0				// VideoWriter color [0:grayscale, 1:color]

#define ComPortPIC		L"COM7"			// Serial communication port - neccessary to add L in front for wide character
#define ComPortParams	L"COM6"			// Serial communication port - neccessary to add L in front for wide character

#define SERVERA   "129.105.69.140"  // server IP address (Machine A - this one)
#define PORTA      9090             // port on which to listen for incoming data (Machine A)
#define SERVERB   "129.105.69.220"  // server IP address (Machine B - Linux/Mikrotron)
#define PORTB      51717            // port on which to send data (Machine B - capture_sequence_avi.c)
#define PORTD      51718            // port on which to send data (Machine B - Matlab)
#define SERVERC   "129.105.69.253"  // server IP address (Machine C - PPOD)
#define PORTC      9091             // port on which to send data (Machine C)
#define UDPBUFLEN  512              // max length of buffer



#ifndef UNICODE
#define UNICODE
#endif

// Necessary for UDP - WinSock
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "Ws2_32.lib")   // Link with ws2_32.lib



using namespace cv;
using namespace std;


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


/* HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const wchar_t* m_pszPortName)

Before starting any communication on a serial port, we must first open a connection, in this case in non-overlapped mode. 
This is achieved by using CreateFile function in Win32.
	The CreateFile function takes in seven parameters. 
		1) Specifies the port name. In our case, this is usually COM, COM2, COM3 or COM4.
		2) GENERIC_READ | GENERIC_WRITE to support both read and write access.
		3) Must always be 0 for serial port communication because unlike files, serial port access cannot be shared.
		4) Used to set security attributes. If no security attribute needs to be specified, just use NULL.
		5) Must always be set to OPEN_EXISTING.
		6) Used to specify flags and attributes (either 0 or FILE_ATTRIBUTE_NORMAL can be used).
		7) must always be NULL as we only support non-overlapped communication.
The HANDLE m_hSerialComm that is returned by the CreateFile function can now be used for performing operations like Configure, Read and Write.

After opening connection to a serial port, the next step is usually to configure the serial port connect settings like Baud Rate, Parity Checking, Byte Size, Error Character, EOF Character etc. 
Win32 provides a DCB struct that encapsulates these settings. 
Configuration of the serial port connection settings is performed in the following three steps:
	1) First, we have to access the present settings of the serial port using the GetCommState function. The function takes in two parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Output parameter, which returns the DCB structure containing the present settings.
	2) Next, using the DCB structure that we obtained from the previous step, we can modify the necessary settings according to the application needs.
	3) Finally, we update the changes by using the SetCommState method.

Another important part of configuration of serial port connection settings is setting timeouts. 
Again, Win32 provides a COMMTIMEOUTS struct for setting Read and Write Timeouts. 
We are also provided with two functions GetCommTimeouts and SetCommTimeouts to access, modify, and update the timeout settings. 

*/
HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const wchar_t* m_pszPortName) {

	// Attempt to open a serial port connection in non-overlapped mode
	m_hSerialComm = CreateFile(m_pszPortName, 
            GENERIC_READ | GENERIC_WRITE, 
            0, 
            NULL, 
            OPEN_EXISTING, 
            0, 
            NULL);

	// Check to see if connection was established
	if (m_hSerialComm == INVALID_HANDLE_VALUE) {  
		printf("Handle error connection.\r\n");
	}
	else {
		printf("Connection established.\r\n");
	}
	

	// DCB (device control block) structure containing the current settings of the port
	DCB dcbConfig;
	
	// Fill all fields of DCB with zeros
	FillMemory(&dcbConfig, sizeof(dcbConfig), 0);

	// Get current settings of the port
	if(!GetCommState(m_hSerialComm, &dcbConfig))
	{
		printf("Error in GetCommState.\r\n");
	}

	// Configuration of serial port
	dcbConfig.BaudRate = 230400;						// 230400 baud rate
	dcbConfig.Parity = NOPARITY;						// No parity
	dcbConfig.ByteSize = 8;								// 8 data bits
	dcbConfig.StopBits = ONESTOPBIT;					// 1 stop bit
	dcbConfig.fRtsControl = RTS_CONTROL_HANDSHAKE;		// RequestToSend handshake
	dcbConfig.fBinary = TRUE;							// Windows API does not support nonbinary mode transfers, so this member should be TRUE
	dcbConfig.fParity = TRUE;							// Specifies whether parity checking is enabled.
	
	// Set new state of port
	if(!SetCommState(m_hSerialComm, &dcbConfig)) {
		// Error in SetCommState
		printf("Error in SetCommState. Possibly a problem with the communications port handle or a problem with the DCB structure itself.\r\n");
	}
	printf("Serial port configured.\r\n");

	
	// COMMTIMEOUTS struct for setting Read and Write Timeouts
	COMMTIMEOUTS commTimeout;

	// Get current settings of the Timeout structure
	if(!GetCommTimeouts(m_hSerialComm, &commTimeout))
	{
		printf("Error in GetCommTimeouts.\r\n");
	}
	
	// Set Read and Write Timeouts
	commTimeout.ReadIntervalTimeout				= 20;
	commTimeout.ReadTotalTimeoutConstant		= 10;
	commTimeout.ReadTotalTimeoutMultiplier		= 100;
	commTimeout.WriteTotalTimeoutConstant		= 10;
	commTimeout.WriteTotalTimeoutMultiplier		= 100;

	// Set new state of CommTimeouts
	if(!SetCommTimeouts(m_hSerialComm, &commTimeout)) {
		// Error in SetCommTimeouts
		printf("Error in SetCommTimeouts. Possibly a problem with the communications port handle or a problem with the COMMTIMEOUTS structure itself.\r\n");
	}
	printf("Serial port set for Read and Write Timeouts.\r\n");

	return m_hSerialComm;

}


/* std::string ReadSerialPort (HANDLE m_hSerialComm)

First, we setup a Read Event using the SetCommMask function. This event is fired when a character is read and buffered internally by Windows Operating System. 
	The SetCommMask function takes in two parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Specify the event type. To specify a Read Event, we use EV_RXCHAR flag.
After calling the SetCommMask function, we call the WaitCommEvent function to wait for the event to occur. 
	This function takes in three parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Output parameter, which reports the event type that was fired.
		3) NULL for our purpose since we do not deal with non-overlapped mode
Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally. 
	The ReadFile function takes in five parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Buffer that would receive the bytes if the ReadFile function returns successfully.
		3) Specifies the size of our buffer we passed in as the second parameter.
		4) Output parameter that will notify the user about the number of bytes that were read.
		5) NULL for our purpose since we do not deal with non-overlapped mode.

In our example, we read one byte at a time and store it in a temporary buffer. 
This continues until the case when ReadFile function returns successfully and the fourth parameter has a value of 0. 
This indicates that the internal buffer used by Windows OS is empty and so we stopping reading. 

*/
std::string ReadSerialPort (HANDLE m_hSerialComm) {

	// String buffer to store bytes read and dwEventMask is an output parameter, which reports the event type that was fired
	std::stringbuf sb;
	DWORD dwEventMask;

	// Setup a Read Event using the SetCommMask function using event EV_RXCHAR
	if(!SetCommMask(m_hSerialComm, EV_RXCHAR)) {
		printf("Error in SetCommMask to read an event.\r\n");
	}

	// Call the WaitCommEvent function to wait for the event to occur
	if(WaitCommEvent(m_hSerialComm, &dwEventMask, NULL))
	{
		//printf("WaitCommEvent successfully called.\r\n");
		char szBuf;
		DWORD dwIncommingReadSize;
		DWORD dwSize = 0;

		// Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally
		do
		{
			if(ReadFile(m_hSerialComm, &szBuf, 1, &dwIncommingReadSize, NULL) != 0) {
				if(dwIncommingReadSize > 0)	{
					dwSize += dwIncommingReadSize;
					sb.sputn(&szBuf, dwIncommingReadSize);
				}
			}
			else {
				//Handle Error Condition
				printf("Error in ReadFile.\r\n");
			}
		} while(dwIncommingReadSize > 0);
	}
	else {
		//Handle Error Condition
		printf("Could not call the WaitCommEvent.\r\n");
	}

	return sb.str();

}


/* void WriteSerialPort (HANDLE m_hSerialComm)

Write operation is easier to implement than Read. It involves using just one function, WriteFile. 
	It takes in five parameters similar to ReadFile function.
		1) HANDLE we received from the call to the CreateFile function.
		2) Specifies the buffer to be written to the serial port.
		3) Specifies the size of our buffer we passed in as the second parameter.
		4) Output parameter that will notify the user about the number of bytes that were written.
		5) NULL for our purpose since we do not deal with non-overlapped mode.
		
The following example shows implementation of Write using WriteFile. 
It writes one byte at a time until all the bytes in our buffer are written.

*/
void WriteSerialPort (HANDLE m_hSerialComm, char message[]) {

	unsigned long dwNumberOfBytesSent = 0;

	// Append new line to end of message.
	// Code adopted from: http://stackoverflow.com/questions/9955236/append-to-the-end-of-a-char-array-in-c
    char temp1[] = "\n";
    char * pszBuf = new char[std::strlen(message)+std::strlen(temp1)+1];
    std::strcpy(pszBuf,message);
    std::strcat(pszBuf,temp1);

	// Size of the buffer pszBuf ("message\r\n")
	DWORD dwSize = (unsigned)strlen(pszBuf);

	// Continue writing until all bytes are sent
	while(dwNumberOfBytesSent < dwSize)
	{
		unsigned long dwNumberOfBytesWritten;

		// Write to serial port whatever is in pszBuf[]
		if(WriteFile(m_hSerialComm, &pszBuf[dwNumberOfBytesSent], 1, &dwNumberOfBytesWritten, NULL) != 0)
		{
			// Increment number of bytes sent
			if(dwNumberOfBytesWritten > 0) {
				++dwNumberOfBytesSent;
			}
			else {
				//Handle Error Condition
				printf("Error in BytesWritten.\r\n");
			}
		}

		else {
			//Handle Error Condition
			printf("Error in WriteFile.\r\n");
		}
	}
	//std::cout << "\r\nLine written to serial port: " << pszBuf << "\r\n";

}


/* void CloseSerialPort (HANDLE m_hSerialComm)

After we finish all our communication with the serial port, we have to close the connection. 
This is achieved by using the CloseHandle function and passing in the serial port HANDLE we obtained from the CreateFile function call. 
Failure to do this results in hard to find handle leaks.

*/
void CloseSerialPort (HANDLE m_hSerialComm) {

	if(m_hSerialComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hSerialComm);
		m_hSerialComm = INVALID_HANDLE_VALUE;
	}
	printf("Serial port closed.\r\n");

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
Note: Frame grabber must be re-opened/closed for taking more than one AVI in the same script
	- Might be a work around if can allocate the correct memory
*/


	// =====================================================================================================================
	// Configuration of serial port: COM7 (Lower USB port on front panel of computer) to talk to PIC
	// =====================================================================================================================

	// Create empty handle to serial port
	HANDLE m_hSerialCommPIC = {0};

	// Serial communication using port COM7
	const wchar_t* m_pszPortNamePIC = ComPortPIC;

	// Configure serial port
	m_hSerialCommPIC = ConfigureSerialPort(m_hSerialCommPIC, m_pszPortNamePIC);



	// =====================================================================================================================
	// Configuration of serial port: COM6 (virtual COM port pair 5/6 - com0com program) to talk with TestingParams (COM5)
	// =====================================================================================================================

	// Create empty handle to serial port
	HANDLE m_hSerialCommParams = {0};

	// Serial communication using port COM6
	const wchar_t* m_pszPortNameParams = ComPortParams;

	// Configure serial port
	m_hSerialCommParams = ConfigureSerialPort(m_hSerialCommParams, m_pszPortNameParams);



	// =====================================================================================================================
	// Frame Grabber board properties
	// =====================================================================================================================
	Fg_Struct *fg;

	int nr_of_buffer	=	BUFLEN;						// Number of memory buffer
	int nCamPort		=	NCAMPORT;					// Port (PORT_A / PORT_B)
	int status = 0;
	
	const char *dllName = NULL;

	int boardType = 0;

	boardType = Fg_getBoardType(nBoard);

	printf("\r\nBoard ID  %d: MicroEnable III (a3)\r\n", nBoard);
	printf("\r\n============================================================================\r\n\r\n");
	
	dllName = "SingleAreaGray.dll";
	


	// =====================================================================================================================
	// Initialize Winsock, create Receiver/Sender sockets, and bind the sockets
	// =====================================================================================================================
	WSADATA wsaData;
    SOCKET RecvSocket;
	SOCKET SendSocket = INVALID_SOCKET;
	
	// Create structures for Machine A, Machine B, and Machine C
    sockaddr_in AddrMachineA, AddrMachineB, AddrMachineBMatlab, AddrMachineC;
	sockaddr_in SenderAddr;
	
    unsigned short PortA = PORTA;		// This machine (TrackCam)
	unsigned short PortB = PORTB;		// Linux (Miktron) - capture_sequence_avi.c
	unsigned short PortD = PORTD;		// Linux (Miktron) - MATLAB
	unsigned short PortC = PORTC;		// PPOD Controller
    char RecvBuf[UDPBUFLEN];
	char SendBuf[UDPBUFLEN];
	int iResult = 0;
    int BufLen = UDPBUFLEN;    
    int SenderAddrSize = sizeof (AddrMachineB);
	

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error %d\n", iResult);
        return 1;
    }	


	// Create a sender socket for sending datagrams
    SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SendSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

	
    // Create a receiver socket to receive datagrams
    RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (RecvSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error %d\n", WSAGetLastError());
        return 1;
    }
	

	// Set up the AddrMachineB structure with the IP address of the receiver and the specified port
	memset((char* ) &AddrMachineB, 0, sizeof(AddrMachineB));

    AddrMachineB.sin_family = AF_INET;
    AddrMachineB.sin_port = htons(PortB);
    AddrMachineB.sin_addr.s_addr = inet_addr(SERVERB);

	// Set up the AddrMachineBMatlab structure with the IP address of the receiver and the specified port
	memset((char* ) &AddrMachineBMatlab, 0, sizeof(AddrMachineBMatlab));

    AddrMachineBMatlab.sin_family = AF_INET;
    AddrMachineBMatlab.sin_port = htons(PortD);
    AddrMachineBMatlab.sin_addr.s_addr = inet_addr(SERVERB);

		
	// Set up the AddrMachineC structure with the IP address of the receiver and the specified port
	memset((char* ) &AddrMachineC, 0, sizeof(AddrMachineC));

    AddrMachineC.sin_family = AF_INET;
    AddrMachineC.sin_port = htons(PortC);
    AddrMachineC.sin_addr.s_addr = inet_addr(SERVERC);
	
	
    // Bind the RecvSocket to any address and the specified port
	memset((char* ) &AddrMachineA, 0, sizeof(AddrMachineA));

    AddrMachineA.sin_family = AF_INET;
    AddrMachineA.sin_port = htons(PortA);
	AddrMachineA.sin_addr.s_addr = htonl(INADDR_ANY);

    iResult = bind(RecvSocket, (SOCKADDR *) & AddrMachineA, sizeof (AddrMachineA));
    if (iResult != 0) {
        wprintf(L"bind failed with error %d\n", WSAGetLastError());
        return 1;
    }

	

	// =====================================================================================================================
	// Local Variables
	// =====================================================================================================================

	// RunFlag variable to continue running tests or stop
	int RunFlag = 1;

	// Variables used for reading from frame grabber and storing pointer to images in buf[i]
	int lastPicNr, tempCounter;

	// Initialize serial and UDP commands
	char message[100];
	char UDPmessage[50];	
	std::string TestingParamsRead;

	// Define errorThreshold for PPOD after changed signal
	float errorThreshold = ERROR_THRESHOLD;

	// Define properties for VideoWriter
	bool isColor = ISCOLOR;
	int fps     = VIDEOFPS;
	int frameW  = WIDTH;
	int frameH  = HEIGHT;
	Size FrameSize = Size(frameW, frameH);
	int codec = CV_FOURCC('H','F','Y','U'); 	// Usable: HFYU, MJPG(missing some information) 
												// Not usable: MPEG, MPG4, YUV8, YUV9, _RGB, 8BPS, DUCK, MRLE, PIMJ, PVW2, RGBT, RLE4, RLE8, RPZA
	char filename[150];
	
	// Used for comparing which IP address and port recvfrom received
	char *CorrectIP;
	u_short CorrectPort;

	// Initialize PPOD_ERROR to some value
	float PPOD_ERROR = 10.0;




	// =====================================================================================================================
	// Start of automated testing - Increment DELAYTIME by 5 ms each run, for one period of shaking
	// =====================================================================================================================
	while( RunFlag == 1 ) {

		// =================================================================================================================
		// Send serial command to COM5 (TestingParams) to obtain next testing parameters
		// =================================================================================================================
		sprintf(message,"%d",8);
		WriteSerialPort(m_hSerialCommParams, message);

		printf("Ready to obtain set of testing parameters.\r\n");



		// =================================================================================================================
		// Receive serial command back from COM5 with current testing parameters
		// =================================================================================================================
		
		// Read from serial port - COM6 - [FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, IDENTIFIER, SAVEDSIGNAL, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIMESTART]
		printf("Waiting to read from TestingParameters - COM6.\r\n");
		TestingParamsRead = ReadSerialPort (m_hSerialCommParams);

		// Deal with incorrect read - don't know why it occurs though
		while( TestingParamsRead.length() == 0 ) {
			TestingParamsRead = ReadSerialPort (m_hSerialCommParams);
		}

		// Store message into variables
		int FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, SAVEDSIGNAL, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, PPOD_RESET;
		char IDENTIFIER;
		float DELAYTIME;

		sscanf(TestingParamsRead.c_str(), "%d%*c %d%*c %d%*c %d%*c %c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d", &FREQ, &VERT_AMPL, &HORIZ_AMPL, &PHASE_OFFSET, &IDENTIFIER, &SAVEDSIGNAL, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET);

		printf("[FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET: %d, %d, %d, %d\r\n", FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET);
		printf("IDENTIFIER, SAVEDSIGNAL: %c, %d\r\n", IDENTIFIER, SAVEDSIGNAL);
		printf("FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE: %d, %d, %d\r\n", FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE);
		printf("PULSETIME, DELAYTIME: %d, %f\r\n", PULSETIME, DELAYTIME);
		printf("PPOD_RESET]: %d\r\n", PPOD_RESET);
		
		
		
		// =================================================================================================================
		// Send datagram to Machine B for current test parameters
		// =================================================================================================================

		wprintf(L"\nSending a datagram to Machine B...Initialization...\n");

		// Create message to send: [FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME]
		memset(&SendBuf[0], 0, sizeof(SendBuf));
		sprintf(UDPmessage,"%d, %d, %d, %d, %d, %d, %d, %f", FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME);
		strcpy(SendBuf,UDPmessage);

		// Send message to Machine B
		iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *) & AddrMachineB, sizeof (AddrMachineB));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			closesocket(SendSocket);
			WSACleanup();
			return 1;
		}



		// =================================================================================================================
		// Initialization of the microEnable frame grabber
		// =================================================================================================================
		if((fg = Fg_Init(dllName,nBoard)) == NULL) {
			status = ErrorMessageWait(fg);
			return status;
		}
		fprintf(stdout,"\r\nInit Grabber: ok\n");
		


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
		if( (pMem0 = Fg_AllocMemEx(fg,totalBufSize,nr_of_buffer)) == NULL ) {
			status = ErrorMessageWait(fg);
			return status;
		}
		else {
			fprintf(stdout,"%d framebuffer allocated for port %d: ok\n\n",nr_of_buffer,nCamPort);
		}


		// Allocate memory for buf array to store pointers to images
		unsigned char * buf[5000];
		for(int i=0;i<5000;i++)
		{
			buf[i] = (unsigned char *) malloc(width*height);
		}


		// Create array for storing frame numbers
		int FrameNumbers[5000] = {0};



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



		// =================================================================================================================
		// Starts a continuous grabbing
		// =================================================================================================================
		if((Fg_AcquireEx(fg,nCamPort,GRAB_INFINITE,ACQ_STANDARD,pMem0)) < 0){
			status = ErrorMessageWait(fg);
			return status;
		}



		// =================================================================================================================
		// Enable ExSync Trigger mode
		// =================================================================================================================
		nExSyncEnable		= 1;
		if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}


			
		// =================================================================================================================
		// Receive datagram from Machine B to start capturing sequence AVI
		// =================================================================================================================

		// Call the recvfrom function to receive datagrams on the bound socket.
		wprintf(L"Receiving datagrams from Machine B...Start Capture...\n");

		memset(&RecvBuf[0], 0, sizeof(RecvBuf));
		iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
		if (iResult == SOCKET_ERROR) {
			wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
		}

		CorrectIP = "129.105.69.220";
		CorrectPort = 51717;

		while( (strcmp(inet_ntoa(SenderAddr.sin_addr),CorrectIP) != 0) && (ntohs(SenderAddr.sin_port) != CorrectPort) ) {
			iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
			if (iResult == SOCKET_ERROR) {
				wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			}
		}

		printf("Received packet from %s: %d\n", inet_ntoa(SenderAddr.sin_addr), ntohs(SenderAddr.sin_port));
		printf("Data: %s\r\n\n",RecvBuf);
			


		// =================================================================================================================
		// Send datagram to PPOD to adjust shaking parameters
		// =================================================================================================================
		wprintf(L"Sending a datagram to Machine C...PPOD shaking parameters...\n");
			
		// Create message to send
		memset(&SendBuf[0], 0, sizeof(SendBuf));
		if (IDENTIFIER == 'E') {
			sprintf(UDPmessage,"%c %d %d %d", IDENTIFIER, HORIZ_AMPL, PHASE_OFFSET, VERT_AMPL);
		}
		else if (IDENTIFIER == 'S') {
			sprintf(UDPmessage,"%c %d", IDENTIFIER, SAVEDSIGNAL);
		}
		strcpy(SendBuf,UDPmessage);

		// Send message to Machine C
		iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *) & AddrMachineC, sizeof (AddrMachineC));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			closesocket(SendSocket);
			WSACleanup();
			return 1;
		}

		Sleep(5000);

			
		// =================================================================================================================
		// Wait until PPOD_ERROR less than a threshold value to continue
		// =================================================================================================================
			
		wprintf(L"Receiving datagrams from Machine C...PPOD_Error...\n");

		// Reset PPOD_ERROR before first compare to errorThreshold
		PPOD_ERROR = 10.0;

		// Wait until PPOD_ERROR less than a threshold value to continue		
		while (PPOD_ERROR >= errorThreshold) {

			// Call the recvfrom function to receive datagrams on the bound socket.
			memset(&RecvBuf[0], 0, sizeof(RecvBuf));
			iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
			if (iResult == SOCKET_ERROR) {
				wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			}

			CorrectIP = "129.105.69.253";

			while( strcmp(inet_ntoa(SenderAddr.sin_addr),CorrectIP) != 0 ) {
				iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
				if (iResult == SOCKET_ERROR) {
					wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
				}
			}

			// Scan RecvBuf into PPOD_ERROR
			sscanf(RecvBuf, "%f", &PPOD_ERROR);
			printf("PPOD_Error: %f, Threshold: %f\r\n", PPOD_ERROR, errorThreshold);

		}


		// =================================================================================================================
		// Obtain frames from TrackCam using ExSync Trigger and store pointer to image in buf
		// =================================================================================================================
		lastPicNr = 0, tempCounter = 0;
			
		// Grab frames until NUMIMAGES frames
		while( lastPicNr < ((NUMIMAGES_Side/INTEGER_MULTIPLE)-1) ) {			// Subtracted 1 (everywhere else too) to avoid missing the last lastPicNr frequently - don't know why it does
			
			// =============================================================================================================
			// Send serial command to PIC for [NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME]
			// =============================================================================================================
			if( lastPicNr == 0) {
				sprintf(message,"%d, %d, %d, %d, %f", NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME);
				WriteSerialPort(m_hSerialCommPIC, message);
				printf("\n[NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME]: %d, %d, %d, %d, %f\r\n", NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME);
			}
			


			// =============================================================================================================
			// Capture frame buffer data from frame grabber and store pointer to image in buf[]
			// =============================================================================================================

			// Get last frame number - blocking function call
			lastPicNr = Fg_getLastPicNumberBlockingEx(fg,lastPicNr+1,nCamPort,15,pMem0); //Waits a maximum of 15 seconds

			// Handle lastPicNr error
			if( lastPicNr < 0 ){
				status = ErrorMessageWait(fg);
				Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
				Fg_FreeMemEx(fg,pMem0);
				Fg_FreeGrabber(fg);
				return status;
			}

			// Store pointer to image in buf[]
			FrameNumbers[tempCounter] = lastPicNr;
			buf[tempCounter] = (unsigned char *) Fg_getImagePtrEx(fg,lastPicNr,0,pMem0);
			tempCounter++;


			/*
			// =============================================================================================================
			// FUTURE WORK: Live Tracking of Droplet - Untested - copied from docs.opencv.org
			//              Can be performed using overhead TrackCam (C++) or side-view Mikrotron (C)
			// =============================================================================================================

			// Caution: Too high of a framerate and too much extra analysis being performed can lead to 
			//          missing the next frame(s) when calling Fg_getLastPicNumberBlockingEx.

			
			// Note: Will have to restructure code if used in order to stop the ExSync signals (PIC32 and blocking function calls).
			//       Also, the main while loops will need to be updated so can leave if something has gone wrong (i.e., drop coalesce),
			//       or if want to change the PPOD shaking signals relative to location of droplet.
			//       Not sure how this would be performed for the Mikrotron code since while( pxd_goneLive() ) stalls the program until
			//       the correct number of frames have been captured.

			// Load frame from buf[] to OpenCV Matrix (Mat)
			Mat TempImg33(height, width, CV_8UC1, buf[tempCounter]);
					
			// Flip image vertically to compensate for camera upside down
			Mat FlippedImg33;
			flip(TempImg33, FlippedImg33, 0);

			// Convert FlippedImg33 to grayscale
			Mat FlippedImg33_Gray;
			cvtColor( FlippedImg33, FlippedImg33_Gray, CV_BGR2GRAY);

			// Reduce noise to help avoiding false circle detection
			GausianBlur( FlippedImg33_Gray, FlippedImg33_Gray, FrameSize, 2, 2 );

			// Apply the Hough Transform to find the circles
			//	 --> docs.opencv.org/2.4/modules/imgproc/doc/feature_detection.html?highlight=houghcircles#houghcircles
			vector<Vec3f> circles;
			double dp=1;
			double minDist=FlippedImg33_Gray.rows/8;
			double param1=200;
			double param2=100;
			int minRadius=0;
			int maxRadius=0;
			HoughCircles( FlippedImg33_Gray, circles, CV_HOUGH_GRADIENT, dp, minDist, param1, param2, min_radius, max_radius );

			// Draw the circles detected
			for( size_t i=0; i<circles.size(); i++) {
				Point center(cvRound(circles[i][0]), cvRound(circles[i][1]))l
				int radius = cvRound(circles[i][2]);
				// Circle center
				circle( FlippedImg33, center, 3, Scalar(0,255,0), -1, 8, 0 );
				// Circle outline
				circle( FlippedImg33, center, radius, Scalar(0,0,255), 3, 8, 0 );
			}

			// Show your results
			namedWindow( "Hough Circle Transform Demo", CV_WINDOW_AUTOSIZE );
			imshow( "Hough Circle Transform Demo", FlippedImg33 );
			waitKey(0);

			// Analysis on location of droplet




			// If need to get out of the loop for grabbing frames until NUMIMAGES frames
			lastPicNr = ((NUMIMAGES_Side/INTEGER_MULTIPLE)-1) + 1;

			// Release OpenCV matrices and vector
			TempImg33.release();
			FlippedImg33.release();
			FlippedImg33_Gray.release();
			circles.clear();
			*/

		}
		printf("\r\nSuccessful Frames Grabbed: %d/%d\r\n", tempCounter, (NUMIMAGES_Side/INTEGER_MULTIPLE)-1);

		

		// =================================================================================================================
		// Writing avi file using each frame
		// =================================================================================================================

		// Create unique name for each AVI object
		sprintf(filename, "C:\\Documents and Settings\\lims\\Desktop\\Videos\\TrackCam_%dA_%dA_%dHz_%03dDPhase_%fDelayTime_%dFPS_%dPulseTime.avi", HORIZ_AMPL, VERT_AMPL, FREQ, PHASE_OFFSET, DELAYTIME, (FPS_Side/INTEGER_MULTIPLE), PULSETIME);
		
		VideoWriter writer2(filename, codec, fps, FrameSize, isColor);
		writer2.open(filename, codec, fps, FrameSize, isColor);

		// Create char array to hold frame number
		char a[4];

		// Check to see if VideoWriter properties are usable
		if (writer2.isOpened()) 
		{
			printf("\r\nStarting to write frames to AVI file.\r\n");
			for(int i=0;i<((NUMIMAGES_Side/INTEGER_MULTIPLE)-1);i++) {

				// Load frame from buf[i] to TempImg for writing to video
				Mat TempImg(height, width, CV_8UC1, buf[i]);
					
				// Flip image vertically to compensate for camera upside down
				Mat FlippedImg;
				flip(TempImg, FlippedImg, 0);
					
				// Add frame number to image, first 4 bytes
				memcpy(a, &FrameNumbers[i], sizeof(FrameNumbers[i]));

				FlippedImg.at<uchar>(0,0) = a[3];
				FlippedImg.at<uchar>(0,1) = a[2];
				FlippedImg.at<uchar>(0,2) = a[1];
				FlippedImg.at<uchar>(0,3) = a[0];

				// Write the frame to the video
				writer2.write(FlippedImg);

				TempImg.release();
				FlippedImg.release();
			}
				
			printf("AVI file written.\r\n\n");
		}
		else {
			cout << "ERROR while opening" << endl;
		}
			
		// Free buffer storing frame data, as well as releasing VideoWriter
		free(buf);
		free(a);
		writer2.release();



		// =================================================================================================================
		// Turning ExSync Enable OFF since finished receiving triggers
		// =================================================================================================================
		nExSyncEnable		= 0;
		if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}



		// =================================================================================================================
		// Freeing the grabber resource
		// =================================================================================================================
		Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
		Fg_FreeMemEx(fg,pMem0);
		Fg_FreeGrabber(fg);



		// =================================================================================================================
		// If PPOD_RESET is True (1), send datagram to PPOD to adjust shaking parameters to all 0's
		// =================================================================================================================
		if( PPOD_RESET == 1 ) {

			// =============================================================================================================
			// Send datagram to PPOD to adjust shaking parameters
			// =============================================================================================================
			wprintf(L"Sending a datagram to Machine C...PPOD shaking parameters...\n");
			
			// Create message to send
			memset(&SendBuf[0], 0, sizeof(SendBuf));		
			sprintf(UDPmessage,"%c %d %d %d", IDENTIFIER, 0, 0, 0);		
			strcpy(SendBuf,UDPmessage);

			// Send message to Machine C
			iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *) & AddrMachineC, sizeof (AddrMachineC));
			if (iResult == SOCKET_ERROR) {
				wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
				closesocket(SendSocket);
				WSACleanup();
				return 1;
			}

			// Once accelerations set to 0, error becomes 'Inf', so wait 20 seconds for it to stabilize to 0			
			printf("Waiting 20 seconds for PPOD accelerations to stabilize to {0}.\r\n");
			Sleep(20000);
		}


		// =================================================================================================================
		// Send serial command to COM5 (TestingParams) to check for another set of testing parameters
		// =================================================================================================================
		sprintf(message,"%d",9);
		WriteSerialPort(m_hSerialCommParams, message);

		printf("Checking for another set of testing parameters.\r\n");

		Sleep(10);



		// =================================================================================================================
		// Receive serial command back from COM5 indicating whether to continue running tests
		// =================================================================================================================

		// Read from serial port
		TestingParamsRead = ReadSerialPort (m_hSerialCommParams);

		// Deal with incorrect read - don't know why it occurs though
		while( TestingParamsRead.length() == 0 ) {
			TestingParamsRead = ReadSerialPort (m_hSerialCommParams);
		}


		// Scan message into RunFlag
		sscanf(TestingParamsRead.c_str(), "%d", &RunFlag);
		printf("RunFlag updated from TestingParams: %d\r\n\n", RunFlag);


		
		// =================================================================================================================
		// Send datagram to Machine B for whether to continue running tests
		// =================================================================================================================
		
		// Clear memory associated with SendBuf
		memset(&SendBuf[0], 0, sizeof(SendBuf));

						
		// Send message to Machine B
		sprintf(UDPmessage,"%d", RunFlag);
		strcpy(SendBuf,UDPmessage);

		wprintf(L"Sending a datagram to Machine B...RunFlag...\n");
		iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *) & AddrMachineB, sizeof (AddrMachineB));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			closesocket(SendSocket);
			WSACleanup();
			return 1;
		}



		// =================================================================================================================
		// Receive datagram from Machine B to continue 
		// =================================================================================================================

		// Call the recvfrom function to receive datagrams from Machine B
		wprintf(L"Receiving datagrams from Machine B...Continue...\n");

		memset(&RecvBuf[0], 0, sizeof(RecvBuf));
		iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
		if (iResult == SOCKET_ERROR) {
			wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
		}

		CorrectIP = "129.105.69.220";
		CorrectPort = 51717;

		while( (strcmp(inet_ntoa(SenderAddr.sin_addr),CorrectIP) != 0) && (ntohs(SenderAddr.sin_port) != CorrectPort) ) {
			iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
			if (iResult == SOCKET_ERROR) {
				wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			}
		}

		printf("Received packet from %s: %d\n", inet_ntoa(SenderAddr.sin_addr), ntohs(SenderAddr.sin_port));
		printf("Data: %s\r\n\n", RecvBuf);
		
		

		// =================================================================================================================
		// Send datagram to Machine B (MATLAB) to tell MATLAB to start the automated analysis
		// =================================================================================================================

		// Created a new AddressMachineB structure (AddrMachineBMatlab) with same IP address and new port so MATLAB and 
		// capture_avi_sequence.c can differentiate which one we are talking to.

		// On the MATLAB side, created a callback function using DatagramReceivedFcn property to continuously
		// be listening for a datagram. Once received, set something called appdata to the UDP message read.
		// Inside the main code, constantly check the appdata for either a filename or terminate statement.

		// Can either perform the analysis after each test (here), or after all sets of testing parameters have been
		// performed (outside the main while loop, but before close WinSock). If do the latter, will have to save the
		// current set of testing parameters (or .avi filenames) after each run in some array.
		
		// --> /home/maciver/Documents/MATLAB/CircleDetect_Tracking_Automated.m
		// MATLAB code is provided on the Linux computer called CircleDetect_Tracking_Automated.m that plots the x-position
		// of the droplet as a function of time. Circle detection parameters that might need to be messed around with
		// are radii_min, radii_max, and sensitivity. Feel free to add blurs or filtering before calling the function
		// imfindcircles() for better results. 
		//    *Included a link to the shared Windows folder under /home/maciver/Documents/MATLAB/


		wprintf(L"Sending a datagram to Machine B...MATLAB Automated Analysis...\n");
			
		// Create message to send - the filename of the movie
		memset(&SendBuf[0], 0, sizeof(SendBuf));
		sprintf(UDPmessage,"%s", filename);		
		strcpy(SendBuf,UDPmessage);

		// Send message to Machine B - Matlab
		iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *) & AddrMachineBMatlab, sizeof (AddrMachineBMatlab));
		if (iResult == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			closesocket(SendSocket);
			WSACleanup();
			return 1;
		}
		


		// =================================================================================================================
		// Receive datagram from Machine B - MATLAB to continue
		// =================================================================================================================

		
		// Call the recvfrom function to receive datagrams on the bound socket.
		wprintf(L"Receiving datagrams from Machine B...MATLAB Continue...\n");

		memset(&RecvBuf[0], 0, sizeof(RecvBuf));
		iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
		if (iResult == SOCKET_ERROR) {
			wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
		}

		CorrectIP = "129.105.69.220";
		CorrectPort = 51718;

		while( (strcmp(inet_ntoa(SenderAddr.sin_addr),CorrectIP) != 0) && (ntohs(SenderAddr.sin_port) != CorrectPort ) ) {
			iResult = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
			if (iResult == SOCKET_ERROR) {
				wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			}
		}

		printf("Received packet from %s: %d\n", inet_ntoa(SenderAddr.sin_addr), ntohs(SenderAddr.sin_port));
		printf("Data: %s\r\n\n",RecvBuf);
		

		
		printf("\r\n============================================================================\r\n\n");
	}




	// =================================================================================================================
	// Send datagram to Machine B (MATLAB) to tell MATLAB to stop the automated analysis
	// =================================================================================================================
	wprintf(L"Sending a datagram to Machine B...MATLAB Automated Analysis...\n");
			
	// Create message to send - the filename of the movie
	memset(&SendBuf[0], 0, sizeof(SendBuf));
	char temp123[] = "Finished";
	sprintf(UDPmessage,"%s", temp123);		
	strcpy(SendBuf,UDPmessage);

	// Send message to Machine B - Matlab
	iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *) & AddrMachineBMatlab, sizeof (AddrMachineBMatlab));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
		closesocket(SendSocket);
		WSACleanup();
		return 1;
	}





	// =====================================================================================================================
	// Close the Receiver/Sender sockets, serial ports, as well as WinSock
	// =====================================================================================================================

	// Close the serial ports
	CloseSerialPort(m_hSerialCommPIC);
	CloseSerialPort(m_hSerialCommParams);

	
	// Close the socket when finished receiving datagrams
    wprintf(L"Closing receiver socket.\n");
    iResult = closesocket(RecvSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket failed with error %d\n", WSAGetLastError());
        return 1;
    }	


	// When the application is finished sending, close the socket
    wprintf(L"Closing sender socket.\n");
    iResult = closesocket(SendSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }


    // Clean up and exit
	WSACleanup();
    wprintf(L"Cleaned up Machine A, Machine B, and Machine C.\n");



	return FG_OK;
}
