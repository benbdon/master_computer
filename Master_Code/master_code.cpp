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
#define ComPortPIC	 	"COM4"			// Serial communication port for PIC32
#define ComPortGRBL     "COM3"         // Serial communication port for Arduino/GRBL

// TCP/IP address & port
#define SERVERB   "129.105.69.121"  // server IP address (Machine B - Linux/Mikrotron) unused in this project, so far... 

#define SERVERC   "129.105.69.186"  // server IP address (Machine C - PPOD)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define PORTB "8080"

int NUM_TESTS = 0;

int main(int argc, char* argv[])
{
	int exitCode = 0;
	PylonAutoInitTerm autoInitTerm; //Automatically opens and closes PylonInitialize()
	WSADATA wsaData;
	WSADATA wsaDataB;
	SOCKET ConnectSocket = INVALID_SOCKET; 	
	// Socket to (C) PPOD computer
	SOCKET SocketToMachineB = INVALID_SOCKET; // Socket to (B) Linux computer
	HANDLE m_hSerialCommPIC = { 0 }; // Declare handle for serial comms to PIC32
	HANDLE m_hSerialCommGRBL = { 0 }; // Declare handle for serial comms to Arduino/GRBL
	ifstream myFile(argv[1]); //instantiate input file stream object

	try{
		// =========================================================
		// Initialize Basler camera 
		// =========================================================
		cout << "Initializing Basler Camera Settings" << endl;

		CBaslerUsbInstantCamera overheadCam(CTlFactory::GetInstance().CreateFirstDevice()); 		/// Create a USB instant camera object with the camera device found first.
		CGrabResultPtr ptrGrabResult; // This smart pointer will receive the grab result data
		CImageFormatConverter formatConverter; // Creates a pylon Image FormatConverter object
		formatConverter.OutputPixelFormat = PixelType_BGR8packed; // Specify the output pixel format
		CPylonImage pylonImage; // Create a Pylon Image that will be used to create OpenCV images later 
		Mat openCvImage; // Create an OpenCV image
		static const uint32_t c_countOfImagesToGrab = 10; //Frames to grab per test //TODO: technically this should be calculated using test parameters
		int grabbedImages = 0;

		overheadCam.Open(); 	// Open camera
		overheadCam.MaxNumBuffer = 15; // MaxNumBuffer can be used to control the size of buffers
		overheadCam.AcquisitionMode.SetValue(AcquisitionMode_Continuous); // Set the acquisition mode to continuous frame
		overheadCam.TriggerMode.SetValue(TriggerMode_Off); // Set the mode for the selected trigger 
		overheadCam.AcquisitionFrameRateEnable.SetValue(false); // Disable the acquisition frame rate parameter (this will disable the camera�s 
																// internal frame rate control and allow you to control the frame rate with 
																// external frame start trigger signals)
		overheadCam.TriggerSelector.SetValue(TriggerSelector_FrameStart); // Select the frame start trigger
		overheadCam.TriggerMode.SetValue(TriggerMode_On); // Set the mode for the selected trigger
		overheadCam.TriggerSource.SetValue(TriggerSource_Line1); // Set the source for the selected trigger ie Opto-isolated IN (Line1)
		overheadCam.TriggerActivation.SetValue(TriggerActivation_FallingEdge); 	// Set the trigger activation mode to falling edge ie this was determined by the PIC code that chose to trigger as a drop in voltage
		//overheadCam.ExposureMode.SetValue(ExposureMode_TriggerWidth); // Set for the trigger width exposure mode TODO: Find appropriate settings for this
		//overheadCam.ExposureOverlapTimeMax.SetValue(87000); // Set the exposure overlap time max- the shortest exposure time
		overheadCam.AcquisitionStart.Execute(); // Execute an acquisition start command to prepare for frame acquisition
		

		// =====================================================================================================================
		// Configuration of comms ports for PIC32 and GRBL
		// =====================================================================================================================
		cout << "\r\n\nConfiguring serial ports" << endl;
		m_hSerialCommPIC = ConfigureSerialPort(m_hSerialCommPIC, ComPortPIC);
		m_hSerialCommGRBL = ConfigureSerialPortGRBL(m_hSerialCommGRBL, ComPortGRBL); //Hint: if the program stalls out here, stop the program and run the "UniversalGcodeSender.jar" file to put the Arduino back into a usable condition
		printf("\r\n\n============Serial ports configured==============================\r\n\n");

		// =====================================================================================================================
		// Initialize Winsock, create Sender/Receiver socket to PPOD Computer
		// =====================================================================================================================
		printf("Connecting to PPOD server (C)\n");
		int iResult, iResultB; //return int from WSAStartup call

		struct addrinfo *result = NULL, *ptr = NULL, *resultB = NULL, *ptrB = NULL, hints, hintsB; // Declare an addrinfo object that contains a sockaddr structure
															// and initialize these values. For this application, the Internet address family is unspecified so that either an 
															// IPv6 or IPv4 address can be returned. The application requests the socket type to be a stream socket for the TCP 
															// protocol.
		
		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2,2) parameter of WSAStartup makes a request for
														// version 2.2 of Winsock on the system, and sets the passed version as the highest version of Windows Sockets support that the caller can use.
		iResultB = WSAStartup(MAKEWORD(2, 2), &wsaDataB);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			return 1;
		}
		
		// After initialization, a SOCKET object must be instantiated for use by the client.
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		ZeroMemory(&hintsB, sizeof(hintsB));
		hintsB.ai_family = AF_UNSPEC;
		hintsB.ai_socktype = SOCK_STREAM;
		hintsB.ai_protocol = IPPROTO_TCP;

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
		printf("\r\n\n=========================connected to PPOD server=====================================\r\n\n");
		
		
		// ======================================================================================
		// Create Sender/Receivere socket to Machine B - TCP communication
		// ========================================================================
		// Resolve the server address and port
		iResultB = getaddrinfo(SERVERB, PORTB, &hintsB, &resultB);
		if (iResultB != 0) {
			printf("getaddrinfo failed: %d\n", iResultB);
			WSACleanup();
			return 1;
		}
		ptrB = resultB;

		// Create a SOCKET for connecting to server
		SocketToMachineB = socket(ptrB->ai_family, ptrB->ai_socktype,
			ptrB->ai_protocol);
		if (SocketToMachineB == INVALID_SOCKET) { 	//Check for errors to ensure that the socket is a valid socket.
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(resultB);
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResultB = connect(SocketToMachineB, ptrB->ai_addr, (int)ptrB->ai_addrlen);
		if (iResultB == SOCKET_ERROR) {
			closesocket(SocketToMachineB);
			SocketToMachineB = INVALID_SOCKET;
		}

		freeaddrinfo(resultB);
		if (SocketToMachineB == INVALID_SOCKET) {
			printf("Unable to connect to Machine B / Linux PC  server!\n");
			WSACleanup();
			return 1;
		}
		const int sendbuflen = 19;
		char sendbuf[sendbuflen];
		const int recvbuflen = 50;
		char recvbuf[recvbuflen];
		iResultB = recv(SocketToMachineB, recvbuf, recvbuflen, 0);
		if (iResultB > 0) {
			printf("Bytes received fr: %d\n", iResultB);
		}
		else if (iResultB == 0)
			printf("Connection closed\n");
		else
			printf("recv failed: %d\n", WSAGetLastError());
		
		printf("\r\n\n=========================connected to Machine B server=====================================\r\n\n");

		// =================================================================================================================
		//Initialize gantry system
		// =================================================================================================================
		cout << "Setting home & moving to default location" << endl;
		char GCODEmessage[150];

		//Default DOD position
		int defX = -131; //-112;
		int defY = -140; // -185;

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

		// Validate contents of test file
		cout << "Verifying provided test file." << endl;
		int LinesInTextFile = file_verification(argv[1]);

		iResultB = recv(SocketToMachineB, recvbuf, recvbuflen, 0);
		if (iResultB > 0) {
			printf("Bytes received: %d\n", iResultB);
			printf("%.*s: Linux PC is ready to receive params\n", iResultB, recvbuf);
		}
		else if (iResultB == 0)
			printf("Connection closed\n");
		else
			printf("recv failed: %d\n", WSAGetLastError());

		printf("%s", recvbuf); //This should print that Machine B is ready to receive the test parameters
		sendbuf[0] = NUM_TESTS;
		printf("number of tests to perform: %d", NUM_TESTS);
		iResultB = send(SocketToMachineB, sendbuf, sendbuflen, 0);
		if (iResultB == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(SocketToMachineB);
			WSACleanup();
			return 1;
		}

		// =============================================================================================================
		// MAIN LOOP
		// =============================================================================================================
		cout << "\r\n=============================beginning main loop=============================\r\n" << endl;
		string current_line; //declare a string to hold 1 line of the text file
		Test_Variables current_test; // instance of class that stores and updates test parameters
		printf("lines in text file: %d\n", LinesInTextFile);

		for (int LinesRead = 0; LinesRead < LinesInTextFile; LinesRead++) { // Continue reading lines until have read the last line
			getline(myFile, current_line);
			printf("current test line is %d\n", LinesRead);
			if (current_line.front() == '#') { 
				printf("test was skipped due to first char\n");
				continue; 
			} // skip lines in .txt that contain a "#" at the beginnging

			replace(current_line.begin(), current_line.end(), ',', ' '); //replace the commas with spaces

			if (current_line.front() == 'E') //Determine which type of test will run by the INDENTIFIER
				current_test.E_set(current_line); //load test variables for PPOD equation mode into current_test instance
			else if (current_line.front() == 'S')
				current_test.S_set(current_line); //load test variables for PPOD saved signals mode into current_test instance
			else {
				cout << "Indentifier parameter must be either an 'E' or 'S'" << endl;
				return 1;
			}
			// current_test.display_updated_parameters(current_line); //Display relevant test parameters

			// =================================================================================================================
			// Move gantry head to specified location
			// =================================================================================================================
			int Xdrop = defX + current_test.XPOS;
			int Ydrop = defY + current_test.YPOS;

			cout << "Moving to (" << current_test.XPOS << ", " << current_test.YPOS << ")\r\n";
			PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, Xdrop, Ydrop); // function is called PositionAbsolute, but inputs are relative position from default + default position

			// =================================================================================================================
			// Send message to (C) PPOD Computer to adjust shaking parameters
			// =================================================================================================================
			printf("Sending test parameters to PPOD (C)\n");
			ostringstream test_params_for_PPOD;


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
			else if (current_test.IDENTIFIER == 'S') { //Todo: Untested branch
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
			
			// =================================================================================================================
			// Send datagram to Machine B for current test parameters
			// =================================================================================================================

			printf("\nSending a TCP message to Machine B...Initialization...\n");
			ostringstream test_params_for_MachineB;
			const int sendbuflenB = 50;
			char sendbufB[sendbuflenB];
			const int recvbuflenB = 50;
			char recvbufB[recvbuflenB];

			// Create message to send
			if (current_test.IDENTIFIER == 'E') {
				test_params_for_MachineB <<
					current_test.IDENTIFIER << " " <<
					current_test.FREQ << " " <<
					current_test.VERT_AMPL << " " <<
					current_test.HORIZ_AMPL_X << " " <<
					//current_test.HORIZ_AMPL_Y << " " <<
					current_test.PHASE_OFFSET << " " <<
					current_test.FPS_Side << " " <<
					current_test.NUMIMAGES_Side << " " <<
					current_test.PULSETIME << " " <<
					current_test.DELAYTIME;
				cout << "IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL_X, PHASE_OFFSET," //should be a horiz_AMPL_Y in there maybe
					"FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME" << endl;
				cout << "MESSAGE FOR TCP: " << test_params_for_MachineB.str() << endl;
				strcpy(sendbufB, test_params_for_MachineB.str().c_str());

			}
			else if (current_test.IDENTIFIER == 'S') { //Todo: Untested branch
				test_params_for_MachineB <<
					current_test.IDENTIFIER << " " <<
					current_test.SAVEDSIGNAL << " " <<
					current_test.FREQ << " " <<
					current_test.FPS_Side << " " <<
					current_test.NUMIMAGES_Side << " " <<
					current_test.PULSETIME << " " <<
					current_test.DELAYTIME;

				cout << "IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME" << endl;
				cout << "MESSAGE FOR TCP: " << test_params_for_MachineB.str() << endl;
				strcpy(sendbufB, test_params_for_MachineB.str().c_str());
			}

			// Send test parameters to Linux PC
			iResultB = send(SocketToMachineB, sendbufB, sendbuflenB, 0);
			if (iResultB == SOCKET_ERROR) {
				printf("send to (B) Linux PC failed: %d\n", WSAGetLastError());
				closesocket(SocketToMachineB);
				WSACleanup();
				return 1;
			}
			

			iResultB = recv(SocketToMachineB, recvbufB, recvbuflenB, 0);
			if (iResultB > 0) {
				printf("Bytes received from Linux PC: %d\n", iResultB);
			}
			else if (iResultB == 0)
				printf("Connection closed\n");
			else
				printf("recv failed: %d\n", WSAGetLastError());
			printf("Linux PC ready to grab frames");

			overheadCam.StartGrabbing(c_countOfImagesToGrab); // Start the grabbing of c_countOfImagesToGrab images.
			printf("\r\n\n============Basler Camera is awaiting trigger signals=============================\r\n\n");
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
			// Obtain frames from Basler camera using Trigger from PIC32 
			// =================================================================================================================
			int firstTimeThrough = true; // this variable is used to decided whether to move the DoD out of the way on a given test
				// currently the command to start moving the DoD is issued right after the first image capture is grabbed on the Baslery/Top Cam
	
			while (overheadCam.IsGrabbing())
			{
				// Wait for an image and then retrieve it. A timeout of 30,000 ms is used.
				overheadCam.RetrieveResult(30000, ptrGrabResult, TimeoutHandling_ThrowException); //throws an error if 30 seconds goes by without a trigger TODO: may want to reduce this

				// Save available images to disk
				if (ptrGrabResult->GrabSucceeded()) {
					cout << "Image Acquired: " << grabbedImages << endl;
					formatConverter.Convert(pylonImage, ptrGrabResult);
					openCvImage = Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t *)pylonImage.GetBuffer());
					ostringstream s; //  declare an output string to store the filename.
					s << "Image_" << grabbedImages << ".jpg";
					string imageName(s.str());
					imwrite(imageName, openCvImage);
					grabbedImages++;

					
					// Move DOD out of the way of camera after the first image is captured
					if (current_test.CAM_MOVE && firstTimeThrough) {
						cout << "Moving DOD out of the way (" << camX << ", " << camY << ")" << endl;
						PositionAbsolute(m_hSerialCommGRBL, GCODEmessage, camX, camY);
						firstTimeThrough = false;
					}
					
				}
				else {
					cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
				}
			}

			//NEED SOMETHING HERE THAT CONFIRMS CAMERA B IS DONE TAKING PICTURES
			cout << "\r\n=============================finished iteration=============================\r\n" << endl;

		}

		// =================================================================================================================
		// Clean-up and shutdown
		// =================================================================================================================
		cout << "\r\n=============================done & shutting down=============================\r\n" << endl;
		// TODO: Create a shutdown TCP message that sends zeros or some unique character string that triggers the a shutdown on the PPOD
		cout << endl << "Close Pylon image viewer if open. Press Enter to exit." << endl;
		while (cin.get() != '\n');
	}
	catch (const GenericException &e) {
		// Error handling.
		cerr << "An exception occurred. " << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
		cout << "exitCode: " << exitCode;
		
	}

	myFile.close(); // close the txt file
	CloseSerialPort(m_hSerialCommPIC); // Close PIC serial port
	CloseSerialPortGRBL(m_hSerialCommGRBL); //Close grbl serial port
	closesocket(ConnectSocket); // When the client application is completed using the Windows Sockets DLL
	closesocket(SocketToMachineB);
	WSACleanup(); // the WSACleanup function is called to release resources.

	return 0;
}