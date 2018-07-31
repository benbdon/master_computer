// master_code.cpp
/*
    Overview: The compiled program based on the master_code.cpp source code is tightly couple to 3 (eventually 4) other distributed computers to control levitating droplets. It commands when and where the droplets should be released onto the vibrating surface, what control pattern the surface should be executing to horizontally manipulate and levitate the droplets, and lastly set the speed (FPS), # of frames, and resolution of the camera(s) capturing its motion.
	
	Distributed computer overview:
	1. This Windows PC/(A)/(129.105.69.211) runs the "master_code" interfacing to the Baser camera/trackcam/overhead camera over USB 
	2. The Windows PC/(C)/(129.105.67.174) runs the controller code for the shaker table/PPOD with 2 National Instruments DAQ cards for AI/AO from accelerometers & to motors + the PIC32 respectively
	3. The Arduino/gantry system/grbl board receive commands from this PC on where to move
	4. The NU32/PIC32 issues synchronized frame acquisition trigger signals to the camera(s) and droplet creation signals
*/

//TCP/IP includes
#include <winsock2.h> //most of the Winsock functions, structures, and definitions 
#include <Ws2tcpip.h> //definitions introduced in the WInSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP address.
#include "file_IO.h"
#include "comms.h"
#pragma comment(lib, "Ws2_32.lib")

// Standard includes
#include <iostream>     // std::cout, std::ostream, std::hex
#include <fstream>      // std::ifstream
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
		cout << "Init Cam" << endl;
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
		// Disable the acquisition frame rate parameter (this will disable the camera�s // internal frame rate control and allow you to control the frame rate with // external frame start trigger signals)
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
		int CommentsInTextFile = 0;
		char FirstChar;
		int LinesRead = 0;
		string line;
		char IDENTIFIER = 0;
		int SAVEDSIGNAL = 0, FREQ = 0, VERT_AMPL = 0, HORIZ_AMPL_X = 0, HORIZ_AMPL_Y = 0, VERT_ALPHA = 0, HORIZ_ALPHA_X = 0;
		int HORIZ_ALPHA_Y = 0, PHASE_OFFSET = 0, FPS_Side = 0, NUMIMAGES_Side = 0, INTEGER_MULTIPLE = 0, PULSETIME = 0;
		float DELAYTIME = 0;
		int PPOD_RESET = 0, CAM_MOVE = 0, XPOS = 0, YPOS = 0;

		ifstream myFile(argv[1]); //instantiate 'read from file' object

		while (LinesRead < LinesInTextFile) { // Continue reading lines until have read the last line
			getline(myFile, line);
			LinesRead += 1;
			sscanf(line.c_str(), "%c", &FirstChar);
			while (FirstChar == '#') { // Check to see if first character is for comments (#)
				getline(myFile, line);
				LinesRead += 1;
				sscanf(line.c_str(), "%c", &FirstChar);
			}

			// =====================================================================================================
			// Scan line read from text file into local variables for current testing parameters
			// =====================================================================================================

			// 1. IDENTIFIER - S & E - S for saved signal, E for PPOD equation
			// 2. SAVED SIGNAL - Just S - Determines which saved signal is run
			// 3. FREQ - S & E - PPOD vibration frequency in HZ
			// 4. VERT_AMPL - Just E - Z axis accelerations of PPOD table in m/s^2
			// 5. HORIZ_AMPL_X - Just E - X axis acceleration of PPOD table in m/s^2
			// 6. HORIZ_AMPL_Y - Just E - Y axis acceleration of PPOD table in m/s^2
			// 7. VERT_ALPHA - Just E - Amplitude of frequency rotating surface about Z axis
			// 8. HORIZ_ALPHA_X - Just E - Amplitude of the frequency rotating PPOD about gantry x axis
			// 9. HORIZ_ALPHA_Y - Just E - Amplitude of frequency roating PPOD about gantry y axis
			// 10. PHASE_OFFSET - Just E - Decouples horizontal and vertical frequencies for adjusting bouncing behavior (0 - 360)
			// 11. FPS_Side - S & E - Frame rate of Mikrotron (side) camera in frames per second
			// 12. NUMIMAGES_Side - S & E - Number of frames the side camera will capture in a given experiment
			// 13. INTEGER_MULTIPLE - S & E - Determines the frame rate for the TrackCam (overhead)
			// 14. PULSETIME - S & E - Pulse time for piezeo in droplet generator in microseconds (680-900)
			// 15. DELAYTIME - S & E - Delay from when cameras start capture to when the droplet is made in seconds (0.000 - 0.0500)
			// 16. PPOD_RESET - S & E - Use to turn off PPOD after a given experiment completes
			// 17. Camera_Move - S & E - (1) Moves the cam out of the cam frame (0) doesn't move the camera
			// 18. XPOS - S & E - Location of the droplet in x axis relative to the center of the bath (-15 - 15)
			// 19. YPOS - S & E - Location of the droplet in y axis relative to the center of the bath (-15 - 15)


			// Scan line for first character
			sscanf(line.c_str(), "%c", &FirstChar);

			// PPOD Saved Signal - [IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME, PPOD_RESET, CAM_MOVE, XPOS, YPOS]
			if (FirstChar == 'S') {
				sscanf(line.c_str(), "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d%*c %d%*c %d%*c %d", &IDENTIFIER, &SAVEDSIGNAL, &FREQ, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET, &CAM_MOVE, &XPOS, &YPOS);

				printf("\n[IDENTIFIER, SAVEDSIGNAL: %c, %d\r\n", IDENTIFIER, SAVEDSIGNAL);
				printf("FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE: %d, %d, %d, %d\r\n", FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE);
				printf("PULSETIME, DELAYTIME: %d, %f\r\n", PULSETIME, DELAYTIME);
				printf("PPOD_RESET: %d\r\n", PPOD_RESET);
				printf("(XPOS, YPOS)]: %d, %d\r\n", XPOS, YPOS);
				printf("CAM_MOVE: %d\r\n", CAM_MOVE);
			}
			// PPOD Equations - [IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME, PPOD_RESET, CAM_MOVE, XPOS, YPOS]
			else if (FirstChar == 'E') {
				sscanf(line.c_str(), "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d%*c %d%*c %d%*c %d",
					&IDENTIFIER, &FREQ, &VERT_AMPL, &HORIZ_AMPL_X, &HORIZ_AMPL_Y, &VERT_ALPHA, &HORIZ_ALPHA_X, &HORIZ_ALPHA_Y,
					&PHASE_OFFSET, &FPS_Side, &NUMIMAGES_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME, &PPOD_RESET, &CAM_MOVE, &XPOS, &YPOS);

				printf("\n[IDENTIFIER: %c\r\n", IDENTIFIER);
				printf("FREQ, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y, PHASE_OFFSET: %d, %d, %d, %d, %d, %d, %d, %d\r\n", FREQ, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y, PHASE_OFFSET);
				printf("FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE: %d, %d, %d\r\n", FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE);
				printf("PULSETIME, DELAYTIME: %d, %f\r\n", PULSETIME, DELAYTIME);
				printf("PPOD_RESET: %d\r\n", PPOD_RESET);
				printf("(XPOS, YPOS): %d, %d\r\n", XPOS, YPOS);
				printf("CAM_MOVE: %d\r\n", CAM_MOVE);
			}

			// =================================================================================================================
			// Move DOD to specified location
			// =================================================================================================================
			int Xdrop = defX + XPOS;
			int Ydrop = defY + YPOS;

			printf("Moving to (%d, %d).\r\n", XPOS, YPOS);
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
			if (IDENTIFIER == 'E') {
				sprintf(sendbuf, "%c %d %d %d %d %d %d %d", IDENTIFIER, PHASE_OFFSET, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y);
				printf("TCP message: %s\n", sendbuf);
			}
			else {
				printf("Was expecting type E\n");
			}
			//TODO: Add back the equation case by figuring out the right number of bytes to send
			/*else if (IDENTIFIER == 'S') {
				sprintf(UDPmessage, "%c %d", IDENTIFIER, SAVEDSIGNAL);
			}*/

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
				float PPOD_ERROR = 0.2;
				iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
				if (iResult > 0) {
					printf("Bytes received: %d\n", iResult);
					cout << PPOD_ERROR << endl;
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
			cout << "Pictures are taken" << endl;
			static const uint32_t c_countOfImagesToGrab = 5;
			
			/*

			// =======================================
			// Kickoff triggered acquisition & grabbing images from the buffer
			//=======================================

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