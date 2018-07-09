////////////////////////////////////////////////////////////////////////////
// Command the DoD to move and release a drop
//
//
//
// File:	grbl_move.cpp
//
// Copyrights by Silicon Software 2002-2010
////////////////////////////////////////////////////////////////////////////

#define ComPortGRBL     "COM3"         // Serial communication port - neccessary to add L in front for wide character
#define sleep_time  1000



// UDP communication includes - must be before include windows.h
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <sstream>      // std::stringbuf
#include <string>       // std::string
#include <time.h>		// Sleep(milliseconds)
#include <windows.h>

// Necessary for UDP - WinSock
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "Ws2_32.lib")   // Link with ws2_32.lib

using namespace std;


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
		printf("A \r\n");
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

/*int main(int argc, char* argv[], char* envp[]) {


	// =====================================================================================================================
	// Configuration of serial port: COM3 to talk to Arduino
	// =====================================================================================================================

	// Create empty handle to serial port
	HANDLE m_hSerialCommGRBL = { 0 };

	// Serial communication using port COM13
	const LPCTSTR m_pszPortNameGRBL = ComPortGRBL;

	// Configure serial port
	m_hSerialCommGRBL = ConfigureSerialPortGRBL(m_hSerialCommGRBL, m_pszPortNameGRBL);



	// =================================================================================================================
	// Display each command-line argument
	// =================================================================================================================
	int count;

	cout << "Command-line arguments:\n";

	for (count = 0; count < argc; count++) {
		cout << " argv[" << count << "]   " << argv[count] << "\n";
	}
	cout << endl;

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

	// =================================================================================================================
	// Check last four characters of argv[1] to ensure if a .txt file was provided
	// =================================================================================================================
	string ext = "";
	for (int i = strlen(argv[1]) - 1; i > strlen(argv[1]) - 5; i--) {
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
				// Moce DOD to specified location
				// =================================================================================================================
				int Xdrop = defX + XPOS;
				int Ydrop = defY + YPOS;

				printf("Moving to (%d, %d).\r\n", XPOS, YPOS);
				PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, Xdrop, Ydrop);
			}
		}
	}
	CloseSerialPort(m_hSerialCommGRBL);
}
*/