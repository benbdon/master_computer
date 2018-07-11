// Grab.cpp
/*
    Note: Before getting started, Basler recommends reading the Programmer's Guide topic
    in the pylon C++ API documentation that gets installed with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the Migration topic in the pylon C++ API documentation.

    This sample illustrates how to grab and process images using the CInstantCamera class.
    The images are grabbed and processed asynchronously, i.e.,
    while the application is processing a buffer, the acquisition of the next buffer is done
    in parallel.

    The CInstantCamera class uses a pool of buffers to retrieve image data
    from the camera device. Once a buffer is filled and ready,
    the buffer can be retrieved from the camera object for processing. The buffer
    and additional image data are collected in a grab result. The grab result is
    held by a smart pointer after retrieval. The buffer is automatically reused
    when explicitly released or when the smart pointer object is destroyed.
*/

// UDP communication includes - must be before include windows.h
#include <winsock2.h>
#include <Ws2tcpip.h>

// Standard includes

#include <iostream>     // std::cout, std::ostream, std::hex
#include <fstream>      // std::ifstream
#include <string>       // std::string
#include <sstream>      // std::stringbuf
#include <time.h>		// Sleep(milliseconds)
#include <windows.h>

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <pylon/usb/PylonUsbIncludes.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using usb camera 
using namespace Basler_UsbCameraParams;

//=======================
//MAY NEED OPENCV includes HERE
//=======================

//======================
//May need something about UNICODE HERE
/*#ifndef UNICODE
#define UNICODE
#endif */
//======================

#define ERROR_THRESHOLD		0.2		// Error threshold needed from PPOD after changing signal before will create droplet and start video capturing

#define ComPortPIC		"\\\\.\\COM4"			// Serial communication port - neccessary to add L in front for wide character

#define ComPortGRBL     "COM3"         // Serial communication port - maybe neccessary to add L in front for wide character

#define SERVERA   "129.105.69.211"  // server IP address (Machine A - this one)
#define PORTA      9090             // port on which to listen for incoming data (Machine A)
#define PORTE      4500             // port on which to listen for incoming data (PPOD)
#define SERVERB   "129.105.69.255"  // server IP address (Machine B - Linux/Mikrotron)
#define PORTB      51717            // port on which to send data (Machine B - capture_sequence_avi.c)
#define PORTD      51718            // port on which to send data (Machine B - Matlab)
#define SERVERC   "129.105.69.174"  // server IP address (Machine C - PPOD)
#define PORTC      9091             // port on which to send data (Machine C)
#define UDPBUFLEN  512              // max length of buffer

// Necessary for UDP - WinSock
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "Ws2_32.lib")   // Link with ws2_32.lib

// Namespace for using cout.
using namespace std;

//============may need the namespace thing below======
//using namespace cv;
//================================


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
HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const LPCTSTR m_pszPortName) {

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
	if (!GetCommState(m_hSerialComm, &dcbConfig))
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
	if (!SetCommState(m_hSerialComm, &dcbConfig)) {
		// Error in SetCommState
		printf("Error in SetCommState. Possibly a problem with the communications port handle or a problem with the DCB structure itself.\r\n");
	}
	printf("Serial port configured.\r\n");


	// COMMTIMEOUTS struct for setting Read and Write Timeouts
	COMMTIMEOUTS commTimeout;

	// Get current settings of the Timeout structure
	if (!GetCommTimeouts(m_hSerialComm, &commTimeout))
	{
		printf("Error in GetCommTimeouts.\r\n");
	}

	// Set Read and Write Timeouts
	commTimeout.ReadIntervalTimeout = 20;
	commTimeout.ReadTotalTimeoutConstant = 10;
	commTimeout.ReadTotalTimeoutMultiplier = 100;
	commTimeout.WriteTotalTimeoutConstant = 10;
	commTimeout.WriteTotalTimeoutMultiplier = 10;

	// Set new state of CommTimeouts
	if (!SetCommTimeouts(m_hSerialComm, &commTimeout)) {
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
std::string ReadSerialPort(HANDLE m_hSerialComm) {

	// String buffer to store bytes read and dwEventMask is an output parameter, which reports the event type that was fired
	std::stringbuf sb;
	DWORD dwEventMask;

	// Setup a Read Event using the SetCommMask function using event EV_RXCHAR
	if (!SetCommMask(m_hSerialComm, EV_RXCHAR)) {
		printf("Error in SetCommMask to read an event.\r\n");
	}
	else {
		//printf("A \r\n");
	}

	// Call the WaitCommEvent function to wait for the event to occur
	if (WaitCommEvent(m_hSerialComm, &dwEventMask, NULL))
	{
		printf("WaitCommEvent successfully called.\r\n");
		char szBuf;
		DWORD dwIncommingReadSize;
		DWORD dwSize = 0;

		// Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally
		do
		{
			if (ReadFile(m_hSerialComm, &szBuf, 1, &dwIncommingReadSize, NULL) != 0) {
				//printf("B \r\n");

				if (dwIncommingReadSize > 0) {
					dwSize += dwIncommingReadSize;
					sb.sputn(&szBuf, dwIncommingReadSize);

					//printf("C \r\n");
				}
			}
			else {
				//Handle Error Condition
				printf("Error in ReadFile.\r\n");
			}
		}

		while (dwIncommingReadSize > 0);
	}
	else {
		//Handle Error Condition
		printf("Could not call the WaitCommEvent.\r\n");
	}

	//printf("D \r\n");

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
void WriteSerialPort(HANDLE m_hSerialComm, char message[]) {

	unsigned long dwNumberOfBytesSent = 0;

	// Append new line to end of message.
	// Code adopted from: http://stackoverflow.com/questions/9955236/append-to-the-end-of-a-char-array-in-c
	char temp1[] = "\n";
	char * pszBuf = new char[std::strlen(message) + std::strlen(temp1) + 1];
	strcpy(pszBuf, message);
	strcat(pszBuf, temp1);

	// Size of the buffer pszBuf ("message\r\n")
	DWORD dwSize = (unsigned)strlen(pszBuf);

	// Continue writing until all bytes are sent
	while (dwNumberOfBytesSent < dwSize)
	{
		unsigned long dwNumberOfBytesWritten;

		// Write to serial port whatever is in pszBuf[]
		if (WriteFile(m_hSerialComm, &pszBuf[dwNumberOfBytesSent], 1, &dwNumberOfBytesWritten, NULL) != 0)
		{
			// Increment number of bytes sent
			if (dwNumberOfBytesWritten > 0) {
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
			//break;
		}
	}
	//std::cout << "\r\nLine written to serial port: " << pszBuf << "\r\n";

}


/* void CloseSerialPort (HANDLE m_hSerialComm)

After we finish all our communication with the serial port, we have to close the connection.
This is achieved by using the CloseHandle function and passing in the serial port HANDLE we obtained from the CreateFile function call.
Failure to do this results in hard to find handle leaks.

*/
void CloseSerialPort(HANDLE m_hSerialComm) {

	if (m_hSerialComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hSerialComm);
		m_hSerialComm = INVALID_HANDLE_VALUE;
	}
	printf("Serial port closed.\r\n");

}

//Separate serial com setup for gShield
HANDLE ConfigureSerialPortGRBL(HANDLE m_hSerialComm, const LPCTSTR m_pszPortName) {

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
		printf("GRBL Connection established.\r\n");
	}


	// DCB (device control block) structure containing the current settings of the port
	DCB dcbConfig;

	// Fill all fields of DCB with zeros
	FillMemory(&dcbConfig, sizeof(dcbConfig), 0);

	// Get current settings of the port
	if (!GetCommState(m_hSerialComm, &dcbConfig))
	{
		printf("Error in GRBL GetCommState.\r\n");
	}

	// Configuration of serial port
	dcbConfig.BaudRate = 115200;						// 230400 baud rate
	dcbConfig.Parity = NOPARITY;						// No parity
	dcbConfig.ByteSize = 8;								// 8 data bits
	dcbConfig.StopBits = ONESTOPBIT;					// 1 stop bit
	dcbConfig.fRtsControl = RTS_CONTROL_HANDSHAKE;		// RequestToSend handshake
	dcbConfig.fBinary = TRUE;							// Windows API does not support nonbinary mode transfers, so this member should be TRUE
	dcbConfig.fParity = TRUE;							// Specifies whether parity checking is enabled.

														// Set new state of port
	if (!SetCommState(m_hSerialComm, &dcbConfig)) {
		// Error in SetCommState
		printf("Error in GRBL SetCommState. Possibly a problem with the communications port handle or a problem with the DCB structure itself.\r\n");
	}
	printf("GRBL Serial port configured.\r\n");


	// COMMTIMEOUTS struct for setting Read and Write Timeouts
	COMMTIMEOUTS commTimeout;

	// Get current settings of the Timeout structure
	if (!GetCommTimeouts(m_hSerialComm, &commTimeout))
	{
		printf("Error in GRBL GetCommTimeouts.\r\n");
	}

	// Set Read and Write Timeouts
	commTimeout.ReadIntervalTimeout = 20;
	commTimeout.ReadTotalTimeoutConstant = 10;
	commTimeout.ReadTotalTimeoutMultiplier = 100;
	commTimeout.WriteTotalTimeoutConstant = 10;
	commTimeout.WriteTotalTimeoutMultiplier = 100;

	// Set new state of CommTimeouts
	if (!SetCommTimeouts(m_hSerialComm, &commTimeout)) {
		// Error in SetCommTimeouts
		printf("Error in GRBL SetCommTimeouts. Possibly a problem with the communications port handle or a problem with the COMMTIMEOUTS structure itself.\r\n");
	}
	printf("GRBL Serial port set for Read and Write Timeouts.\r\n");

	string returnedMessage = ReadSerialPort(m_hSerialComm);
	returnedMessage.append("\r\n");
	printf(returnedMessage.c_str());


	return m_hSerialComm;

}

//Returns DOD generator to home position, should be left corner against wall, if it isn't, use Universal Gcode Sender to reset home there
void Home(HANDLE m_hSerialCommGRBL, char GCODEmessage[]) {
	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G90 G0 X0 Y0");
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);


	//string test = ReadSerialPort(m_hSerialCommGRBL);
	//test.append("\r\n");
	//printf(test.c_str());
}

void HomeNoRead(HANDLE m_hSerialCommGRBL, char GCODEmessage[]) {
	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G90 G0 X0 Y0");
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);


	//string test = ReadSerialPort(m_hSerialCommGRBL);
	//test.append("\r\n");
	//printf(test.c_str());
}

//Sends DOD Generator to specified location. Coordinats are relative to machine home position, center of PPOD should be around X110 Y-210
void PositionAbsolute(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos) {
	//printf("debugging....");

	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G90 G0 X%d Y%d", Xpos, Ypos);
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);

	//printf("debugging....");

	//string test = ReadSerialPort(m_hSerialCommGRBL);
	//test.append("\r\n");
	//printf(test.c_str());
}

void PositionAbsoluteNoRead(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos) {
	//printf("debugging....");

	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G90 G0 X%d Y%d", Xpos, Ypos);
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);

	//printf("debugging....");

	//string test = ReadSerialPort(m_hSerialCommGRBL);
	//test.append("\r\n");
	//printf(test.c_str());
}

//Moves DOD Generator relative to it's current location
void PositionRelative(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos) {
	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G91 G0 X%d Y%d", Xpos, Ypos);
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);

	string test = ReadSerialPort(m_hSerialCommGRBL);
	//test.append("\r\n");
	//printf(test.c_str());
}

int main(int argc, char* argv[], char* envp[])
{

	// =====================================================================================================================
	// Configuration of serial port: COM13 (Lower USB port on front panel of computer) to talk to PIC
	// =====================================================================================================================

	
	// Create empty handle to serial port
	HANDLE m_hSerialCommPIC = { 0 };

	// Serial communication using port COM13
	const LPCTSTR m_pszPortNamePIC = ComPortPIC;

	// Configure serial port
	m_hSerialCommPIC = ConfigureSerialPort(m_hSerialCommPIC, m_pszPortNamePIC);
	

	// =====================================================================================================================
	// Configuration of serial port: COM9 (Lower USB port on back next to ethernet) to talk to Arduino
	// =====================================================================================================================

	// Create empty handle to serial port
	HANDLE m_hSerialCommGRBL = { 0 };

	// Serial communication using port COM13
	const LPCTSTR m_pszPortNameGRBL = ComPortGRBL;

	// Configure serial port
	m_hSerialCommGRBL = ConfigureSerialPortGRBL(m_hSerialCommGRBL, m_pszPortNameGRBL);

	// =====================================================================================================================
	// TrackCam (Basler) properties
	// =====================================================================================================================
	// Number of images to be grabbed.
	static const uint32_t c_countOfImagesToGrab = 5;

	// =================================================================================================================
	// Display each command-line argument
	// =================================================================================================================
	int count;

	cout << "Command-line arguments:\n";

	for (count = 0; count < argc; count++) {
		cout << " argv[" << count << "]   " << argv[count] << "\n";
	}
	cout << endl;
	
	/*
	// =====================================================================================================================
	// Initialize Winsock, create Receiver/Sender sockets, and bind the sockets
	// =====================================================================================================================
	WSADATA wsaData;
	SOCKET RecvSocket, RecvSocketPPOD;
	SOCKET SendSocket = INVALID_SOCKET;

	// Create structures for Machine A, Machine B, and Machine C
	sockaddr_in AddrMachineA, AddrMachineAPPOD, AddrMachineB, AddrMachineBMatlab, AddrMachineC;
	sockaddr_in SenderAddr;

	unsigned short PortA = PORTA;		// This machine (TrackCam) - Linux
	unsigned short PortE = PORTE;		// This machine (TrackCam) - PPOD
	unsigned short PortB = PORTB;		// Linux (Miktron) - capture_sequence_avi.c
	unsigned short PortD = PORTD;		// Linux (Miktron) - MATLAB
	unsigned short PortC = PORTC;		// PPOD Controller
	char RecvBuf[UDPBUFLEN], RecvBufPPOD[UDPBUFLEN];
	char SendBuf[UDPBUFLEN];
	int iResult = 0;
	int BufLen = UDPBUFLEN;
	int SenderAddrSize = sizeof(AddrMachineB);
	int SenderAddrSizeA = sizeof(AddrMachineA);


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

	struct timeval tv;

	tv.tv_sec = 120;
	tv.tv_usec = 0;

	setsockopt(RecvSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

	//17-06-02 By Mengjiao
	RecvSocketPPOD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (RecvSocketPPOD == INVALID_SOCKET) {
		wprintf(L"socketppod failed with error %d\n", WSAGetLastError());
		return 1;
	}

	struct timeval tvppod;

	tvppod.tv_sec = 120;
	tvppod.tv_usec = 0;

	setsockopt(RecvSocketPPOD, SOL_SOCKET, SO_RCVTIMEO, (char *)&tvppod, sizeof(struct timeval));


	// Set up the AddrMachineB structure with the IP address of the receiver and the specified port
	memset((char*)&AddrMachineB, 0, sizeof(AddrMachineB));

	AddrMachineB.sin_family = AF_INET;
	AddrMachineB.sin_port = htons(PortB);
	AddrMachineB.sin_addr.s_addr = inet_addr(SERVERB);

	// Set up the AddrMachineBMatlab structure with the IP address of the receiver and the specified port
	memset((char*)&AddrMachineBMatlab, 0, sizeof(AddrMachineBMatlab));

	AddrMachineBMatlab.sin_family = AF_INET;
	AddrMachineBMatlab.sin_port = htons(PortD);
	AddrMachineBMatlab.sin_addr.s_addr = inet_addr(SERVERB);


	// Set up the AddrMachineC structure with the IP address of the receiver and the specified port
	memset((char*)&AddrMachineC, 0, sizeof(AddrMachineC));

	AddrMachineC.sin_family = AF_INET;
	AddrMachineC.sin_port = htons(PortC);
	AddrMachineC.sin_addr.s_addr = inet_addr(SERVERC);


	// Bind the RecvSocket to any address and the specified port
	memset((char*)&AddrMachineA, 0, sizeof(AddrMachineA));

	AddrMachineA.sin_family = AF_INET;
	AddrMachineA.sin_port = htons(PortA);
	AddrMachineA.sin_addr.s_addr = htonl(INADDR_ANY);

	iResult = bind(RecvSocket, (SOCKADDR *)& AddrMachineA, sizeof(AddrMachineA));
	if (iResult != 0) {
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 1;
	}

	//17-06-02 By Mengjiao
	// Bind the RecvSocket to any address and the specified port
	memset((char*)&AddrMachineAPPOD, 0, sizeof(AddrMachineAPPOD));

	AddrMachineAPPOD.sin_family = AF_INET;
	AddrMachineAPPOD.sin_port = htons(PortE);
	AddrMachineAPPOD.sin_addr.s_addr = htonl(INADDR_ANY);

	iResult = bind(RecvSocketPPOD, (SOCKADDR *)& AddrMachineAPPOD, sizeof(AddrMachineAPPOD));
	if (iResult != 0) {
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 1;
	}
*/

	// =====================================================================================================================
	// Local Variables
	// =====================================================================================================================

	// RunFlag variable to continue running tests or stop
	int RunFlag = 1;

	int iLast = 0;
	bool MemSet = false;

	// Initialize serial and UDP commands
	char GCODEmessage[150];
	//char message[150];
	//char UDPmessage[150];
	string TestingParamsRead;

	//Default DOD position
	int defX = -112;
	int defY = -185;

	int camX = defX - 50;
	int camY = defY + 60;

	// Define errorThreshold for PPOD after changed signal
	float errorThreshold = ERROR_THRESHOLD;

	//	char filename[150];
	//	char filepath[150];

	// Used for comparing which IP address and port recvfrom received
	//char *CorrectIP;
	//u_short CorrectPort;

	// Create a string for reading lines in text file
	string line;

	// Variable for counting the number of lines in text file
	int LinesInTextFile = 0;

	// Variable for counting the number of comments in text file
	int CommentsInTextFile = 0;

	// Variable for counting the number of lines read in text file
	int LinesRead = 0;

	// Variable for first character read in line (to determine if comments)
	char FirstChar;

	// =================================================================================================================
	//Initilize DOD gantry
	// =================================================================================================================
	printf("setting home.\r\n");

	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	//sprintf(GCODEmessage,"G10 P0 L20 X0 Y0 Z0");
	sprintf(GCODEmessage, "$H");
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);

	string test = ReadSerialPort(m_hSerialCommGRBL);
	test.append("\r\n");
	printf(test.c_str());

	//Move DOD to default location
	printf("Moving to default location.\r\n");
	PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, defX, defY);

	//Sleep(25000);


	// =================================================================================================================
	// Check last four characters of argv[1] to ensure if a .txt file was provided
	// =================================================================================================================
	string ext = "";
	for (int i = strlen(argv[1]) - 1; i>strlen(argv[1]) - 5; i--) {
		ext += (argv[1])[i];
	}



	// =================================================================================================================
	// Text file provided
	// =================================================================================================================
	if (ext == "txt.") {  //backwards .txt

		printf("First parameter is a filename.\r\n");


		// =============================================================================================================
		// Count the number of lines in text file
		// =============================================================================================================

		// Use argv[1] for filename
		ifstream myfile(argv[1]);

		// Count lines
		if (myfile.is_open()) {
			while (getline(myfile, line)) {

				// Increment line counter
				LinesInTextFile += 1;

				// Check to see if first character is for comments (#)				
				sscanf(line.c_str(), "%c", &FirstChar);
				if (FirstChar == 35) {
					// Increment tests counter
					CommentsInTextFile += 1;
				}
			}
		}
		else {
			cout << "Unable to open file.";
		}

		// Close file
		myfile.close();

		printf("  Lines in text file: %d\r\n", LinesInTextFile);
		printf("  Number of tests to perform: %d\r\n", (LinesInTextFile - CommentsInTextFile));
		printf("\r\n\n============================================================================\r\n\n");



		// =============================================================================================================
		// Read lines in text file
		// =============================================================================================================

		// Use argv[1] for filename
		ifstream myFile(argv[1]);

		// Open text file
		if (myFile.is_open()) {

			// Continue reading lines until have read the last line
			while (LinesRead < LinesInTextFile) {

				// =====================================================================================================
				// Read next line, ignoring comments
				// =====================================================================================================
				getline(myFile, line);
				LinesRead += 1;

				// Scan line for first character
				sscanf(line.c_str(), "%c", &FirstChar);

				// Check to see if first character is for comments (#)
				while (FirstChar == '#') {

					// Read next line
					getline(myFile, line);
					LinesRead += 1;

					// Scan line for first character
					sscanf(line.c_str(), "%c", &FirstChar);
				}



				// =====================================================================================================
				// Scan line read from text file into local variables for current testing parameters
				// =====================================================================================================

				// Local variables to store line from text file into
				int FREQ = 0, VERT_AMPL = 0, HORIZ_AMPL = 0, PHASE_OFFSET = 0, SAVEDSIGNAL = 0, FPS_Side = 0, NUMIMAGES_Side = 0, INTEGER_MULTIPLE = 0, PULSETIME = 0,
					PPOD_RESET = 0, XPOS = 0, YPOS = 0, CAM_MOVE = 0, HORIZ_AMPL_X = 0, HORIZ_AMPL_Y = 0, VERT_ALPHA = 0, HORIZ_ALPHA_X = 0, HORIZ_ALPHA_Y = 0;
				char IDENTIFIER = 0;
				float DELAYTIME = 0;

				// Scan line for first character
				sscanf(line.c_str(), "%c", &FirstChar);

				// PPOD Saved Signal - [IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME, PPOD_RESET, CAM_MOVE, XPOS, YPOS]
				if (FirstChar == 'S') {
					//sscanf(line.c_str(), "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d", &IDENTIFIER, &SAVEDSIGNAL, &FREQ, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET);
					sscanf(line.c_str(), "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d%*c %d%*c %d%*c %d", &IDENTIFIER, &SAVEDSIGNAL, &FREQ, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET, &CAM_MOVE, &XPOS, &YPOS);

					printf("\n[IDENTIFIER, SAVEDSIGNAL: %c, %d\r\n", IDENTIFIER, SAVEDSIGNAL);
					printf("FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE: %d, %d, %d, %d\r\n", FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE);
					printf("PULSETIME, DELAYTIME: %d, %f\r\n", PULSETIME, DELAYTIME);
					printf("PPOD_RESET: %d\r\n", PPOD_RESET);
					printf("[XPOS, YPOS]: %d, %d\r\n", XPOS, YPOS);
					printf("CAM_MOVE: %d\r\n", CAM_MOVE);
				}
				// PPOD Equations - IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME, PPOD_RESET, XPOS, YPOS]
				else if (FirstChar == 'E') {
					//sscanf(line.c_str(), "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d%*c %d%*c %d%*c %d", &IDENTIFIER, &FREQ, &VERT_AMPL, &HORIZ_AMPL, &PHASE_OFFSET, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET, &XPOS, &YPOS, &CAM_MOVE);

					sscanf(line.c_str(), "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d%*c %d%*c %d%*c %d",
						&IDENTIFIER, &FREQ, &VERT_AMPL, &HORIZ_AMPL_X, &HORIZ_AMPL_Y, &VERT_ALPHA, &HORIZ_ALPHA_X, &HORIZ_ALPHA_Y,
						&PHASE_OFFSET, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET, &CAM_MOVE, &XPOS, &YPOS);

					printf("\n[IDENTIFIER: %c\r\n", IDENTIFIER);
					//printf("FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET: %d, %d, %d, %d\r\n", FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET);
					printf("FREQ, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y, PHASE_OFFSET: %d, %d, %d, %d, %d, %d, %d, %d\r\n", FREQ, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y, PHASE_OFFSET);
					printf("FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE: %d, %d, %d\r\n", FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE);
					printf("PULSETIME, DELAYTIME: %d, %f\r\n", PULSETIME, DELAYTIME);
					printf("PPOD_RESET: %d\r\n", PPOD_RESET);
					printf("[XPOS, YPOS]: %d, %d\r\n", XPOS, YPOS);
					printf("CAM_MOVE: %d\r\n", CAM_MOVE);
				}



				// =================================================================================================================
				// Move DOD to specified location
				// =================================================================================================================
				int Xdrop = defX + XPOS;
				int Ydrop = defY + YPOS;

				printf("Moving to (%d, %d).\r\n", XPOS, YPOS);
				PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, Xdrop, Ydrop);

				/*
				// =================================================================================================================
				// Send datagram to Machine B for current test parameters
				// =================================================================================================================

				wprintf(L"\nSending a datagram to Machine B...Initialization...\n");

				// Create message to send
				memset(&SendBuf[0], 0, sizeof(SendBuf));
				if (FirstChar == 'S') {
					sprintf(UDPmessage, "%c, %d, %d, %d, %d, %f, %d", IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME);
				}
				else if (FirstChar == 'E') {
					sprintf(UDPmessage, "%c, %d, %d, %d, %d, %d, %d, %d, %f", IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL_Y, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME);
				}
				strcpy(SendBuf, UDPmessage);

				// Send message to Machine B
				iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR *)& AddrMachineB, sizeof(AddrMachineB));
				if (iResult == SOCKET_ERROR) {
					wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
					closesocket(SendSocket);
					WSACleanup();
					return 1;
				}
				*/
			}
		}
	}
	CloseSerialPort(m_hSerialCommGRBL);

	// =========================================================
	// BEN'S CAMERA CODE BEGINS
    // =========================================================
		// The exit code of the sample application.
    int exitCode = 0;
    // Before using any pylon methods, the pylon runtime must be initialized. 
    PylonInitialize();

    try
    {
        // Create a USB instant camera object with the camera device found first.
        CBaslerUsbInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

		// Open camera
		camera.Open();

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;
		

		// The parameter MaxNumBuffer can be used to control the count of buffers
		// allocated for grabbing. The default value of this parameter is 10.
		camera.MaxNumBuffer = 10;

		// Set the acquisition mode to continuous frame
		camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		// Set the mode for the selected trigger
		camera.TriggerMode.SetValue(TriggerMode_Off);
		// Disable the acquisition frame rate parameter (this will disable the camera’s // internal frame rate control and allow you to control the frame rate with // external frame start trigger signals)
		camera.AcquisitionFrameRateEnable.SetValue(false);
		// Select the frame start trigger
		camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
		// Set the mode for the selected trigger
		camera.TriggerMode.SetValue(TriggerMode_On);
		// Set the source for the selected trigger
		camera.TriggerSource.SetValue(TriggerSource_Line1);
		// Set the trigger activation mode to rising edge
		camera.TriggerActivation.SetValue(TriggerActivation_RisingEdge);
		// Set for the trigger width exposure mode
		camera.ExposureMode.SetValue(ExposureMode_TriggerWidth);
		// Set the exposure overlap time max- the shortest exposure time // we plan to use is 1500 us
		camera.ExposureOverlapTimeMax.SetValue(1500);
		// Prepare for frame acquisition here
		camera.AcquisitionStart.Execute();
		
		// Start the grabbing of c_countOfImagesToGrab images.
		// The camera device is parameterized with a default configuration which
		// sets up free-running continuous acquisition.
		camera.StartGrabbing(c_countOfImagesToGrab);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when c_countOfImagesToGrab images have been retrieved.
		while (camera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded())
			{

				// Access the image data.
				cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
				cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
				const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
				cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

#ifdef PYLON_WIN_BUILD
				// Display the grabbed image.
				Pylon::DisplayImage(1, ptrGrabResult);
#endif
			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}
	}
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    // Releases all pylon resources. 
    PylonTerminate();  

    return exitCode;
}