// master_code.cpp
/*
    Overview: TBD
	Distributed computer overview:
	1. This Windows PC/(A)/(129.105.69.211) runs the "master_code" interfacing to the Baser camera/trackcam/overhead camera over USB 
	2. The Windows PC/(C)/(129.105.67.174) runs the controller code for the shaker table/PPOD with 2 National Instruments DAQ cards for AI/AO from accelerometers & to motors + the PIC32 respectively
	3. The Arduino/gantry system/grbl board receive commands from this PC on where to move
	4. The NU32/PIC32 issues synchronized frame acquisition trigger signals to the camera(s) and droplet creation signals
*/

//TCP/IP includes
#include <winsock2.h> //most of the Winsock functions, structures, and definitions 
#include <Ws2tcpip.h> //definitions introduced in the WinSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP address.
#include "file_IO.h"
#include "comms.h"
#include "test_variables.h"
#pragma comment(lib, "Ws2_32.lib")

// Standard includes
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <algorithm>	//std::replace
#include <string>
#include <time.h>		// Sleep(milliseconds)

// Basler camera
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/usb/PylonUsbIncludes.h>
using namespace Pylon;
using namespace Basler_UsbCameraParams;
using namespace std;

// Serial ports
#define ComPortPIC		"COM4"			// Serial communication port - neccessary to add L in front for wide character
#define ComPortGRBL     "COM3"         // Serial communication port - maybe neccessary to add L in front for wide character

// TCP/IP address & port
#define SERVERA   "129.105.69.211"  // server IP address (Machine A - this one)
#define PORTA      9090             // port on which to listen for incoming data (Machine A)
#define PORTE      4500             // port on which to listen for incoming data (PPOD)
#define SERVERB   "129.105.69.220"  // server IP address (Machine B - Linux/Mikrotron)
#define PORTB      51717            // port on which to send data (Machine B - capture_sequence_avi.c)
#define PORTD      51718            // port on which to send data (Machine B - Matlab)
#define SERVERC   "129.105.69.174"  // server IP address (Machine C - PPOD)
#define PORTC      9091             // port on which to send data (Machine C)
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int main(int argc, char* argv[], char* envp[])
{
	//Before using any pylon methods, the pylon runtime must be initialized. 
	int exitCode = 0;
	PylonInitialize();

	try {
		// =========================================================
		// Initialization of Basler camera 
		// =========================================================
		cout << "Initialize Basler Camera Settings" << endl;
		// Create a USB instant camera object with the camera device found first.
		CBaslerUsbInstantCamera overheadCam(CTlFactory::GetInstance().CreateFirstDevice());

		// Open camera
		overheadCam.Open();

		// =================================================================================================================
		// Memory allocation
		// =================================================================================================================
		// The parameter MaxNumBuffer can be used to control the count of buffers
		// allocated for grabbing. The default value of this parameter is 10.
		overheadCam.MaxNumBuffer = 10;

		// =================================================================================================================
		// Setting the trigger mode for Extern Trigger
		// =================================================================================================================
		// Set the acquisition mode to continuous frame
		overheadCam.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		// Set the mode for the selected trigger
		overheadCam.TriggerMode.SetValue(TriggerMode_Off);
		// Disable the acquisition frame rate parameter (this will disable the camera’s // internal frame rate control and allow you to control the frame rate with // external frame start trigger signals)
		overheadCam.AcquisitionFrameRateEnable.SetValue(false);
		// Select the frame start trigger
		overheadCam.TriggerSelector.SetValue(TriggerSelector_FrameStart);
		// Set the mode for the selected trigger
		overheadCam.TriggerMode.SetValue(TriggerMode_On);
		// Set the source for the selected trigger
		overheadCam.TriggerSource.SetValue(TriggerSource_Line1);
		// Set the trigger activation mode to rising edge
		overheadCam.TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// =================================================================================================================
		// Setting the image size and camera format // but really mostly just exposure
		// =================================================================================================================
		// Set for the trigger width exposure mode
		overheadCam.ExposureMode.SetValue(ExposureMode_TriggerWidth);
		// Set the exposure overlap time max- the shortest exposure time // we plan to use is 1500 us
		overheadCam.ExposureOverlapTimeMax.SetValue(1500);
		
		
		// =====================================================================================================================
		// Configuration of all comms ports
		// =====================================================================================================================

		HANDLE m_hSerialCommPIC = { 0 };
		m_hSerialCommPIC = ConfigureSerialPort(m_hSerialCommPIC, ComPortPIC);

		HANDLE m_hSerialCommGRBL = { 0 };
		m_hSerialCommGRBL = ConfigureSerialPortGRBL(m_hSerialCommGRBL, ComPortGRBL);

		// =====================================================================================================================
		// Initialize Winsock, create Receiver/Sender sockets, and bind the sockets
		// =====================================================================================================================
		WSADATA wsaData;
		//SOCKET RecvSocket, RecvSocketPPOD;
		//SOCKET SendSocket = INVALID_SOCKET;
		int iResult; //return int from WSAStartup call
		SOCKET ConnectSocket = INVALID_SOCKET; 	// Create a SOCKET object called ConnectSocket.
		struct addrinfo *result = NULL, *ptr = NULL, hints; //Declare an addrinfo object that contains a sockaddr structure
															// and initialize these values. For this application, the Internet address family is unspecified so that either an 
															// IPv6 or IPv4 address can be returned. The application requests the socket type to be a stream socket for the TCP 
															// protocol.
		if (argc != 2) {
			printf("usage: %s server-name\n", argv[0]);
			return 1;
		}

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); //MAKEWORD(2,2) parameter of WSAStartup makes a request for
														// version 2.2 of Winsock on the system, and sets the passed version as the highest version of Windows Sockets support that the caller can use.
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			return 1;
		}

		// After initialization, a SOCKET object must be instantiated for use by the client.
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		iResult = getaddrinfo(SERVERC, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		ptr = result;

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) { 	//Check for errors to ensure that the socket is a valid socket.
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
		}

		freeaddrinfo(result);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("Unable to connect to server!\n");
			WSACleanup();
			return 1;
		}

		// =================================================================================================================
		//Initilize DOD gantry
		// =================================================================================================================
		char GCODEmessage[150];
		//Default DOD position
		int defX = -112;
		int defY = -185;

		//Home the DOD
		printf("Setting home.\r\n");
		Home(m_hSerialCommGRBL, GCODEmessage);

		//Move DOD to default location
		printf("Moving to default location.\r\n");
		PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, defX, defY);

		// ====================================
		// Validate contents of test file
		// ===================================
		printf("Verifying provided test file. \r\n");
		int LinesInTextFile = file_verification(argv[1]);

		// =============================================================================================================
		// MAIN LOOP
		// =============================================================================================================
		char FirstChar;
		string current_line;
		Test_Variables current_test; // instance of class that stores and updates test parameters

		ifstream myFile(argv[1]); //instantiate input file stream object

		for (int LinesRead = 0; LinesRead < LinesInTextFile; LinesRead++) { // Continue reading lines until have read the last line
			getline(myFile, current_line);
			
			if (current_line.front() == '#') { continue; } // skip lines that contain a # at the beginnging
			
			replace(current_line.begin(), current_line.end(), ',', ' '); //replace the commas with spaces

			if (FirstChar == 'E')
				current_test.E_set(current_line); //load test variables for PPOD equation mode
			else
				current_test.S_set(current_line); //load test variables for PPOD saved signals mode

			current_test.display_updated_parameters(current_line);

			// =================================================================================================================
			// Move DOD to specified location
			// =================================================================================================================
			int Xdrop = defX + current_test.XPOS;
			int Ydrop = defY + current_test.YPOS;

			cout << "Moving to ("<< current_test.XPOS << ", " << current_test.YPOS << ")\r\n";
			PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, Xdrop, Ydrop);

			// =================================================================================================================
			// Send datagram to PPOD to adjust shaking parameters
			// =================================================================================================================
			printf("Sending a datagram to Machine C...PPOD shaking parameters...\n");
			char recvbuf[DEFAULT_BUFLEN];
			int recvbuflen = DEFAULT_BUFLEN;
			char sendbuf[DEFAULT_BUFLEN];
			int sendbuflen = DEFAULT_BUFLEN;

			// Create message to send
			
			/*
			if (IDENTIFIER == 'E') {
				sprintf(sendbuf, "%c %d %d %d %d %d %d %d", current_test, IDENTIFIER, PHASE_OFFSET, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y);
				printf("The send message is this long: %d", strlen(sendbuf));
				printf("TCP message: %s\n", sendbuf);
			}
			else {
				printf("Was expecting type E\n");
			}
			//TODO: Add back the equation case by figuring out the right number of bytes to send
			else if (IDENTIFIER == 'S') {
				sprintf(UDPmessage, "%c %d", IDENTIFIER, SAVEDSIGNAL);
			}
			*/

			// Send PPOD parameters to PPOD PC
			iResult = send(ConnectSocket, sendbuf, sendbuflen, 0);
			if (iResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}

			printf("Bytes Sent: %ld\n", iResult);

			iResult = shutdown(ConnectSocket, SD_SEND); //closes connection for sending, but still receiving
			if (iResult == SOCKET_ERROR) {
				printf("shutdown failed: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}
			// Receive data until the server closes the connection
			do {
				// Define errorThreshold for PPOD after changed signal
				iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
				if (iResult > 0) {
					printf("Bytes received: %d\n", iResult);
					printf("%.*s: Shaker platform is ready\n", 4, recvbuf);
				}
				else if (iResult == 0)
					printf("Nothing to read\n");
				else
					printf("recv failed: %d\n", WSAGetLastError());
			} while (iResult > 0);

			// =================================================================================================================
			// Send command to PIC32 to trigger frame capture
			// =================================================================================================================
			// TODO

			
			

			// =======================================
			// Kickoff triggered acquisition & grabbing images from the buffer
			//=======================================
			static const uint32_t c_countOfImagesToGrab = 5;
			cout << "Pictures are taken" << endl;
			/*
			// Prepare for frame acquisition here
			overheadCam.AcquisitionStart.Execute();

			// Start the grabbing of c_countOfImagesToGrab images.
			// The camera device is parameterized with a default configuration which
			// sets up free-running continuous acquisition.
			overheadCam.StartGrabbing(c_countOfImagesToGrab);
			printf("Basler Camera is armed and ready\n\r");

			// =================================================================================================================
			// Obtain frames from TrackCam using ExSync Trigger and store pointer to image in buf
			// =================================================================================================================
			// This smart pointer will receive the grab result data.

			CGrabResultPtr ptrGrabResult;

			// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
			// when c_countOfImagesToGrab images have been retrieved.
			while (overheadCam.IsGrabbing())
			{
				// Wait for an image and then retrieve it. A timeout of 15000 ms is used.
				overheadCam.RetrieveResult(15000, ptrGrabResult, TimeoutHandling_ThrowException);

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
			*/


		}
		// cleanup
		myFile.close(); // close the txt file
		closesocket(ConnectSocket); // When the client application is completed using the Windows Sockets DLL
		WSACleanup(); // the WSACleanup function is called to release resources.
		CloseSerialPort(m_hSerialCommPIC); // Close PIC serial port
		CloseSerialPort(m_hSerialCommGRBL); //Close grbl serial port
		PylonTerminate();
	}
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
    }
	return 0;
}