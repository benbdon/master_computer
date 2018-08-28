// master_code.cpp
/*
    Overview: TBD
	Distributed computer overview:
	1. This Windows PC/(A)/(129.105.69.211) runs the "master_code" interfacing to the Baser camera/trackcam/overhead camera over USB 
	2. The Windows PC/(C)/(129.105.67.174) runs the controller code for the shaker table/PPOD with 2 National Instruments DAQ cards for AI from accelerometers & AO to motors & the PIC32 respectively
	3. The Arduino/gantry system/grbl board receive commands from this PC on where to move
	4. The NU32/PIC32 issues synchronized frame acquisition trigger signals to the camera(s) and droplet creation signals
*/

// TCP/IP includes
#include <winsock2.h> //most of the Winsock functions, structures, and definitions 
#include <Ws2tcpip.h> //definitions introduced in the WinSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP address.
#pragma comment(lib, "Ws2_32.lib")

// Standard includes
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <algorithm>	//std::replace
#include <string>

// .h File includes
#include "file_IO.h"
#include "comms.h"
#include "test_variables.h"

// Basler camera
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/usb/PylonUsbIncludes.h>

// OpenCV API includes
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

//To avoid std:: and other such annoying scope operators...
using namespace Pylon; //Basler camera functions
using namespace Basler_UsbCameraParams; // Specific Basler camera functions
using namespace std; // C++ standard library
using namespace cv; // Computer Vision functions

// Serial ports
#define ComPortPIC		"COM4"			// Serial communication port for PIC32
#define ComPortGRBL     "COM3"         // Serial communication port for Arduino/GRBL

// TCP/IP address & port
//#define SERVERB   "129.105.69.220"  // server IP address (Machine B - Linux/Mikrotron) unused in this project, so far...

#define SERVERC   "129.105.69.174"  // server IP address (Machine C - PPOD)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int main(int argc, char* argv[])
{
	int exitCode = 0;
	PylonAutoInitTerm autoInitTerm; //Automatically opens and closes PylonInitialize()
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET; 	// Declare Socket
	HANDLE m_hSerialCommPIC = { 0 }; // Declare handle for serial comms to PIC32
	HANDLE m_hSerialCommGRBL = { 0 }; // Declare handle for serial comms to Arduino/GRBL
	ifstream myFile(argv[1]); //instantiate input file stream object

	try{
		// =========================================================
		// Configure Basler camera 
		// =========================================================
		cout << "Initializing Basler Camera Settings" << endl;

		CBaslerUsbInstantCamera overheadCam(CTlFactory::GetInstance().CreateFirstDevice()); 		/// Create a USB instant camera object with the camera device found first.
		static const uint32_t c_countOfImagesToGrab = 5; //Frames to grab per test //technically this should be calculated using test parameters FPS_Side,  

		overheadCam.Open(); 	// Open camera
		overheadCam.MaxNumBuffer = 15; // MaxNumBuffer can be used to control the size of buffers
		overheadCam.AcquisitionMode.SetValue(AcquisitionMode_Continuous); // Set the acquisition mode to continuous frame
		overheadCam.TriggerMode.SetValue(TriggerMode_Off); // Set the mode for the selected trigger
		overheadCam.AcquisitionFrameRateEnable.SetValue(false); // Disable the acquisition frame rate parameter (this will disable the camera’s 
																// internal frame rate control and allow you to control the frame rate with 
																// external frame start trigger signals)
		overheadCam.TriggerSelector.SetValue(TriggerSelector_FrameStart); // Select the frame start trigger
		overheadCam.TriggerMode.SetValue(TriggerMode_On); // Set the mode for the selected trigger
		overheadCam.TriggerSource.SetValue(TriggerSource_Line1); // Set the source for the selected trigger
		overheadCam.TriggerActivation.SetValue(TriggerActivation_FallingEdge); 	// Set the trigger activation mode to falling edge
		//overheadCam.ExposureMode.SetValue(ExposureMode_TriggerWidth); // Set for the trigger width exposure mode
		//overheadCam.ExposureOverlapTimeMax.SetValue(1500); // Set the exposure overlap time max- the shortest exposure time // we plan to use is 1500 us
		
		CGrabResultPtr ptrGrabResult; // This smart pointer will receive the grab result data
		CImageFormatConverter formatConverter; // Creates a pylon Image FormatConverter object
		formatConverter.OutputPixelFormat = PixelType_BGR8packed; // Specify the output pixel format
		CPylonImage pylonImage; // Create a Pylon Image that will be used to create OpenCV images later 
		int grabbedImages = 0;
		Mat openCvImage; // Create an OpenCV image

		printf("\r\n\n============Cam settings initialized=============================\r\n\n");

		// =====================================================================================================================
		// Configuration of comms ports for PIC32 and GRBL
		// =====================================================================================================================
		cout << "\r\n\nConfiguring serial ports" << endl;
		m_hSerialCommPIC = ConfigureSerialPort(m_hSerialCommPIC, ComPortPIC);
		m_hSerialCommGRBL = ConfigureSerialPortGRBL(m_hSerialCommGRBL, ComPortGRBL); //Hint: if the program stalls out here, stop the program and run the "UniversalGcodeSender.jar" file to put the Arduino back into a usable condition
		printf("\r\n\n============Serial ports configured==============================\r\n\n");

		// =====================================================================================================================
		// Initialize Winsock, create Receiver/Sender socket
		// =====================================================================================================================
		printf("Connecting to PPOD server (C)\n");
		int iResult; //return int from WSAStartup call

		struct addrinfo *result = NULL, *ptr = NULL, hints; //Declare an addrinfo object that contains a sockaddr structure
															// and initialize these values. For this application, the Internet address family is unspecified so that either an 
															// IPv6 or IPv4 address can be returned. The application requests the socket type to be a stream socket for the TCP 
															// protocol.

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
		printf("\r\n\n=========================connected to server=====================================\r\n\n");

		// =================================================================================================================
		//Initialize gantry system
		// =================================================================================================================
		cout << "Setting home & moving to default location" << endl;
		char GCODEmessage[150];

		//Default DOD position
		int defX = -112;
		int defY = -185;

		// Location to move DOD when camMove enabled
		int camX = defX - 50;
		int camY = defY + 60;

		// Home the gantry head
		printf("Setting home.\r\n");
		Home(m_hSerialCommGRBL, GCODEmessage);

		// Move gantry head to default location
		printf("Moving to default location.\r\n");
		PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, defX, defY);
		printf("\r\n\n============================home and default set =======================================\r\n\n");

		// ====================================
		// Validate contents of test file
		// ===================================
		cout << "Verifying provided test file." << endl;
		int LinesInTextFile = file_verification(argv[1]);

		// =============================================================================================================
		// MAIN LOOP
		// =============================================================================================================
		cout << "\r\n=============================beginning main loop=============================\r\n" << endl;
		string current_line;
		Test_Variables current_test; // instance of class that stores and updates test parameters

		for (int LinesRead = 0; LinesRead < LinesInTextFile; LinesRead++) { // Continue reading lines until have read the last line
			getline(myFile, current_line);

			if (current_line.front() == '#') { continue; } // skip lines that contain a # at the beginnging

			replace(current_line.begin(), current_line.end(), ',', ' '); //replace the commas with spaces

			if (current_line.front() == 'E') //Determine which type of test will run by the INDENTIFIER
				current_test.E_set(current_line); //load test variables for PPOD equation mode into current_test instance
			else if (current_line.front() == 'S')
				current_test.S_set(current_line); //load test variables for PPOD saved signals mode into current_test instance
			else {
				cout << "Indentifier parameter must be either an 'E' or 'S'" << endl;
				return 1;
			}
			// current_test.display_updated_parameters(current_line); //Dispay relevant test parameters

			// =================================================================================================================
			// Move gantry head to specified location
			// =================================================================================================================
			int Xdrop = defX + current_test.XPOS;
			int Ydrop = defY + current_test.YPOS;

			cout << "Moving to (" << current_test.XPOS << ", " << current_test.YPOS << ")\r\n";
			PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, Xdrop, Ydrop);

			// =================================================================================================================
			// Send message to PPOD to adjust shaking parameters
			// =================================================================================================================
			printf("Sending test parameters to PPOD (C)\n");
			ostringstream test_params_for_PPOD;
			int sendbuflen = 19;
			char sendbuf[19];
			int recvbuflen = 5;
			char recvbuf[5];

			// Create message to send
			if (current_test.IDENTIFIER == 'E') {
				test_params_for_PPOD <<
					current_test.IDENTIFIER << " " <<
					current_test.PHASE_OFFSET << " " <<
					current_test.VERT_AMPL << " " <<
					current_test.HORIZ_AMPL_X << " " <<
					current_test.HORIZ_AMPL_Y << " " <<
					current_test.VERT_ALPHA << " " <<
					current_test.HORIZ_ALPHA_X << " " <<
					current_test.HORIZ_ALPHA_Y;
				cout << "IDENTIFIER, PHASE_OFFSET, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y,"
					"VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y" << endl;
				cout << "MESSAGE FOR TCP: " << test_params_for_PPOD.str() << endl;
				strcpy(sendbuf, test_params_for_PPOD.str().c_str());

			}
			else if (current_test.IDENTIFIER == 'S') {
				test_params_for_PPOD << current_test.IDENTIFIER << " " <<
					current_test.SAVEDSIGNAL;
				cout << "IDENTIFIER, SAVEDSIGNAL" << endl;
				cout << "MESSAGE FOR TCP: " << test_params_for_PPOD.str() << endl;
				strcpy(sendbuf, test_params_for_PPOD.str().c_str());
			}

			// Send PPOD parameters to PPOD PC
			iResult = send(ConnectSocket, sendbuf, sendbuflen, 0);
			if (iResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}

			// Receive data from server closes
			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				printf("Bytes received: %d\n", iResult);
				printf("%.*s: Shaker platform is ready\n", iResult, recvbuf);
			}
			else if (iResult == 0)
				printf("Connection closed\n");
			else
				printf("recv failed: %d\n", WSAGetLastError());

			// =============================================================================================================
			// Send serial command to PIC for [NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME]
			// =============================================================================================================
			ostringstream test_params_for_PIC32;
			char message[200];
			
			cout << "NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME" << endl;
			test_params_for_PIC32 <<
				current_test.NUMIMAGES_Side << " " <<
				current_test.FPS_Side << " " <<
				current_test.INTEGER_MULTIPLE << " " <<
				current_test.PULSETIME << " " <<
				current_test.DELAYTIME;
			cout << "Msg for PIC32: " << test_params_for_PIC32.str() << endl;
			strcpy(message, test_params_for_PIC32.str().c_str());
			WriteSerialPort(m_hSerialCommPIC, message);

			// =================================================================================================================
			// Move DOD to out of the way of camera
			// =================================================================================================================
			if (current_test.CAM_MOVE) {
				cout << "Moving DOD out of the way (" << camX << ", " << camY << ")" << endl;
				PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, camX, camY);
			}
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
			cout << "Triggering images" << endl;

			while (overheadCam.IsGrabbing())
			{
				// Wait for an image and then retrieve it. A timeout of 30,000 ms is used.
				overheadCam.RetrieveResult(30000, ptrGrabResult, TimeoutHandling_ThrowException);

				// Image grabbed successfully?
				if (ptrGrabResult->GrabSucceeded()) {
					// Access the image data.
					cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
					cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;			
					//const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
					//cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;
					cout << "Image Acquired" << endl;
					formatConverter.Convert(pylonImage, ptrGrabResult);
					openCvImage = Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t *)pylonImage.GetBuffer());
					ostringstream s; // Create the current image name for saving.
					s << "Image_" << grabbedImages << ".jpg";
					string imageName(s.str());
					imwrite(imageName, openCvImage);
					grabbedImages++;
					/*#ifdef PYLON_WIN_BUILD
						// Display the grabbed image.
						Pylon::DisplayImage(1, ptrGrabResult);
					#endif*/
				}
				else {
					cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
				}
			}
			cout << "\r\n=============================finished iteration=============================\r\n" << endl;
		}
		// TODO: Create a shutdown TCP message that sends zeros or some unique character string that triggers the a shutdown on the PPOD
		namedWindow("OpenCV Display Window", CV_WINDOW_NORMAL);
		Mat image = imread("Image_0.jpg");
		imshow("OpenCV Display Window", image); // Display current image in OpenCV display window
		waitKey(0);

		cout << "\r\n=============================done & shutting down=============================\r\n" << endl;
		// =================================================================================================================
		// Clean-up and shutdown
		// =================================================================================================================
		cout << endl << "Close Pylon image viewer if open. Press Enter to exit." << endl;
		while (cin.get() != '\n');
	}
	catch (const GenericException &e) {
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	myFile.close(); // close the txt file
	CloseSerialPort(m_hSerialCommPIC); // Close PIC serial port
	CloseSerialPortGRBL(m_hSerialCommGRBL); //Close grbl serial port
	closesocket(ConnectSocket); // When the client application is completed using the Windows Sockets DLL
	WSACleanup(); // the WSACleanup function is called to release resources.

	return 0;
}