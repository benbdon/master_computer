// comms.cpp

#include <winsock2.h> //most of the Winsock functions, structures, and definitions
#include <iostream>
#include <sstream>
#include "comms.h"

// Configure Serial Port
HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const LPCTSTR m_pszPortName) {

	// Attempt to open a serial port connection in non-overlapped mode
	m_hSerialComm = CreateFile(
		m_pszPortName, // lpFileName 
		GENERIC_READ | GENERIC_WRITE, // dwDesiredAccess - means READ and WRITE access
		0, //dwShareMode - 0 prevents other processes from opening a file or device if they request delete, read, or write access.
		NULL, // lpSecurityAttributes - NULL mean the handle returned by CreateFile cannot be inherited by any child processes the application may create and the file or device associated with the returned handle gets a default security descriptor.
		OPEN_EXISTING, // dwCreationDisposition - Opens a file or device, only if it exists. If the specified file or device does not exist, the function fails and the last - error code is set to ERROR_FILE_NOT_FOUND
		0, //dwFlagsAndAttributes  
		NULL); // hTemplateFile

	if (m_hSerialComm == INVALID_HANDLE_VALUE) {
		printf("Handle error connection.\r\n");
	}
	else {
		printf("Connection established.\r\n");
	}

	// Set configuration settings for the serial port & ensure no errors
	DCB dcbConfig;
	FillMemory(&dcbConfig, sizeof(dcbConfig), 0);
	dcbConfig.BaudRate = 230400;						// 230400 baud rate
	dcbConfig.Parity = NOPARITY;						// No parity
	dcbConfig.ByteSize = 8;								// 8 data bits
	dcbConfig.StopBits = ONESTOPBIT;					// 1 stop bit
	dcbConfig.fRtsControl = RTS_CONTROL_HANDSHAKE;		// RequestToSend handshake
	dcbConfig.fBinary = TRUE;							// Windows API does not support nonbinary mode transfers, so this member should be TRUE
	dcbConfig.fParity = TRUE;							// Specifies whether parity checking is enabled.
	if (!SetCommState(m_hSerialComm, &dcbConfig)) {
		printf("Error in SetCommState. Possibly a problem with the communications port handle or a problem with the DCB structure itself.\r\n");
	}
	printf("Serial port configured.\r\n");

	// Set new state of CommTimeouts
	COMMTIMEOUTS commTimeout; //variable to store timeout config
	commTimeout.ReadIntervalTimeout = 20;
	commTimeout.ReadTotalTimeoutConstant = 10;
	commTimeout.ReadTotalTimeoutMultiplier = 100;
	commTimeout.WriteTotalTimeoutConstant = 10;
	commTimeout.WriteTotalTimeoutMultiplier = 10;
	if (!SetCommTimeouts(m_hSerialComm, &commTimeout)) {
		// Error in SetCommTimeouts
		printf("Error in SetCommTimeouts. Possibly a problem with the communications port handle or a problem with the COMMTIMEOUTS structure itself.\r\n");
	}
	printf("Serial port set for Read and Write Timeouts.\r\n");
	return (m_hSerialComm);
}

// Read Serial Port - receives data from the PIC32
string ReadSerialPort(HANDLE m_hSerialComm) {

	// String buffer to store bytes read and dwEventMask is an output parameter, which reports the event type that was fired
	stringbuf sb;
	DWORD dwEventMask;

	// Setup a Read Event using the SetCommMask function using event EV_RXCHAR
	if (!SetCommMask(m_hSerialComm, EV_RXCHAR)) {
		printf("Error in SetCommMask to read an event.\r\n");
	}

	// Call the WaitCommEvent function to wait for the event to occur
	if (WaitCommEvent(m_hSerialComm, &dwEventMask, NULL)){
		printf("WaitCommEvent successfully called.\r\n");
		char szBuf;
		DWORD dwIncomingReadSize;
		DWORD dwSize = 0;

		// Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally
		do {
			if (ReadFile(m_hSerialComm, &szBuf, 1, &dwIncomingReadSize, NULL) != 0) {
				if (dwIncomingReadSize > 0) {
					dwSize += dwIncomingReadSize;
					sb.sputn(&szBuf, dwIncomingReadSize);
				}
			}
			else {
				//Handle Error Condition
				printf("Error in ReadFile.\r\n");
			}
		} while (dwIncomingReadSize > 0);
	}
	else {
		//Handle Error Condition
		printf("Could not call the WaitCommEvent.\r\n");
	}
	return sb.str();
}

// Write Serial Port - 1 byte at a time
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
}

// Close Serial Port
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
	
	string returnedMessage = ReadSerialPortGRBL(m_hSerialComm);
	returnedMessage.append("\r\n");
	printf(returnedMessage.c_str());
	
	return m_hSerialComm;
}

// Read Serial Port - receives data from the PIC32
string ReadSerialPortGRBL(HANDLE m_hSerialComm) {

	// String buffer to store bytes read and dwEventMask is an output parameter, which reports the event type that was fired
	stringbuf sb;
	DWORD dwEventMask;

	// Setup a Read Event using the SetCommMask function using event EV_RXCHAR
	if (!SetCommMask(m_hSerialComm, EV_RXCHAR)) {
		printf("Error in SetCommMask to read an event.\r\n");
	}

	// Call the WaitCommEvent function to wait for the event to occur
	if (WaitCommEvent(m_hSerialComm, &dwEventMask, NULL)) {
		printf("WaitCommEvent successfully called.\r\n");
		char szBuf;
		DWORD dwIncomingReadSize;
		DWORD dwSize = 0;

		// Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally
		do {
			if (ReadFile(m_hSerialComm, &szBuf, 1, &dwIncomingReadSize, NULL) != 0) {
				if (dwIncomingReadSize > 0) {
					dwSize += dwIncomingReadSize;
					sb.sputn(&szBuf, dwIncomingReadSize);
				}
			}
			else {
				//Handle Error Condition
				printf("Error in ReadFile.\r\n");
			}
		} while (dwIncomingReadSize > 0);
	}
	else {
		//Handle Error Condition
		printf("Could not call the WaitCommEvent.\r\n");
	}
	return sb.str();
}

//Returns DOD generator to home position, should be left corner against wall, if it isn't, use Universal Gcode Sender to reset home there
void Home(HANDLE m_hSerialCommGRBL, char GCODEmessage[]) {
	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "$H");
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);
}

//Sends DOD Generator to specified location. Coordinats are relative to machine home position, center of PPOD should be around X110 Y-210
void PositionAbsolute(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos) {
	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G90 G0 X%d Y%d", Xpos, Ypos);
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);
}

//Moves DOD Generator relative to it's current location
void PositionRelative(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos) {
	memset(&GCODEmessage[0], 0, sizeof(GCODEmessage));
	sprintf(GCODEmessage, "G91 G0 X%d Y%d", Xpos, Ypos);
	WriteSerialPort(m_hSerialCommGRBL, GCODEmessage);
}