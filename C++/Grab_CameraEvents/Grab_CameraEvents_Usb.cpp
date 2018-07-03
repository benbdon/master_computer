// Grab_CameraEvents_Usb.cpp
/*
    Note: Before getting started, Basler recommends reading the Programmer's Guide topic
    in the pylon C++ API documentation that gets installed with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the Migration topic in the pylon C++ API documentation.

    Basler USB3 Vision cameras can send event messages. For example, when a sensor
    exposure has finished, the camera can send an Exposure End event to the PC. The event
    can be received by the PC before the image data for the finished exposure has been completely
    transferred. This sample illustrates how to be notified when camera event message data
    is received.

    The event messages are automatically retrieved and processed by the InstantCamera classes.
    The information carried by event messages is exposed as parameter nodes in the camera node map
    and can be accessed like "normal" camera parameters. These nodes are updated
    when a camera event is received. You can register camera event handler objects that are
    triggered when event data has been received.

    These mechanisms are demonstrated for the Exposure End event.
    The  Exposure End event carries the following information:
    * EventExposureEndFrameID: Indicates the number of the image frame that has been exposed.
    * EventExposureEndTimestamp: Indicates the moment when the event has been generated.
    transfer the exposed frame.

    It is shown in this sample how to register event handlers indicating the arrival of events
    sent by the camera. For demonstration purposes, several different handlers are registered
    for the same event.
*/

//UDP communication includes
#include <WinSock2.h>
#include <Ws2tcpip.h>

//Standard includes
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>     // std::strcmp
#include <string>       // std::string
#include <iostream>     // std::cout, std::ostream, std::hex
#include <fstream>      // std::ifstream
#include <sstream>      // std::stringbuf
#include <time.h>		// Sleep(milliseconds)
#include <math.h>


//CAMERA INCLUDES HERE
// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>

// Include files used by samples.
#include "../include/ConfigurationEventPrinter.h"
#include "../include/CameraEventPrinter.h"

// OpenCV includes?
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>

#define TRIGMODE	ASYNC_TRIGGER	// 0: Free Run, 1: Grabber Controlled, 2: Extern Trigger, 4: Software Trigger
#define EXPOSUREMS	20				// Exposure used for frame grabber (not sure what it really does - made as small as possible)
#define TRIGSOURCE	0				// 0-3: Trigger Source 0-3 (respectively)

#define BUFLEN		100				// Buffer length allowable on computer for frame grabber storage
// Frame Buffer Memory Size: 819 million pixels [Width*Height*BUFLEN]

#define nBoard		0				// Board number
#define NCAMPORT	PORT_A			// Port (PORT_A / PORT_B)

#define ERROR_THRESHOLD		0.2		// Error threshold needed from PPOD after changing signal before will create droplet and start video capturing

#define VIDEOFPS	5				// VideoWriter FPS
#define ISCOLOR		0				// VideoWriter color [0:grayscale, 1:color]

#define ComPortPIC		L"\\\\.\\COM13"			// Serial communication port - neccessary to add L in front for wide character
#define ComPortParams	L"COM6"			// Serial communication port - neccessary to add L in front for wide character

#define ComPortGRBL     L"COM9"         // Serial communication port - neccessary to add L in front for wide character

#define SERVERA   "129.105.69.140"  // server IP address (Machine A - this one)
#define PORTA      9090             // port on which to listen for incoming data (Machine A)
#define PORTE      4500             // port on which to listen for incoming data (PPOD)
#define SERVERB   "129.105.69.220"  // server IP address (Machine B - Linux/Mikrotron)
#define PORTB      51717            // port on which to send data (Machine B - capture_sequence_avi.c)
#define PORTD      51718            // port on which to send data (Machine B - Matlab)
#define SERVERC   "129.105.69.253"  // server IP address (Machine C - PPOD)
#define PORTC      9091             // port on which to send data (Machine C)
#define UDPBUFLEN  512              // max length of buffer


// Namespace for using pylon objects.
using namespace Pylon;

#if defined( USE_USB )
// Settings for using Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
typedef CBaslerUsbCameraEventHandler CameraEventHandler_t; // Or use Camera_t::CameraEventHandler_t
using namespace Basler_UsbCameraParams;
#else
#error Camera type is not specified. For example, define USE_USB for using USB cameras.
#endif

// Namespace for using cout.
using namespace std;

//Enumeration used for distinguishing different events.
enum MyEvents
{
    eMyExposureEndEvent  = 100
    // More events can be added here.
};

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 5;


// Example handler for camera events.
class CSampleCameraEventHandler : public CameraEventHandler_t
{
public:
    // Only very short processing tasks should be performed by this method. Otherwise, the event notification will block the
    // processing of images.
    virtual void OnCameraEvent( Camera_t& camera, intptr_t userProvidedId, GenApi::INode* /* pNode */)
    {
        std::cout << std::endl;
        switch ( userProvidedId )
        {
        case eMyExposureEndEvent: // Exposure End event
            cout << "Exposure End event. FrameID: " << camera.EventExposureEndFrameID.GetValue() << " Timestamp: " << camera.EventExposureEndTimestamp.GetValue() << std::endl << std::endl;
            break;
        // More events can be added here.
        }
    }
};

//Example of an image event handler.
class CSampleImageEventHandler : public CImageEventHandler
{
public:
    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
    {
        cout << "CSampleImageEventHandler::OnImageGrabbed called." << std::endl;
        cout << std::endl;
        cout << std::endl;
    }
};

int main(int argc, char* argv[])
{
    // The exit code of the sample application
    int exitCode = 0;

    // Before using any pylon methods, the pylon runtime must be initialized. 
    PylonInitialize();

    // Create an example event handler. In the present case, we use one single camera handler for handling multiple camera events.
    // The handler prints a message for each received event.
    CSampleCameraEventHandler* pHandler1 = new CSampleCameraEventHandler;

    // Create another more generic event handler printing out information about the node for which an event callback
    // is fired.
    CCameraEventPrinter*  pHandler2 = new CCameraEventPrinter;

    try
    {
        // Only look for cameras supported by Camera_t
        CDeviceInfo info;
        info.SetDeviceClass( Camera_t::DeviceClass());

        // Create an instant camera object with the first found camera device matching the specified device class.
        Camera_t camera( CTlFactory::GetInstance().CreateFirstDevice( info));

        // Register the standard configuration event handler for enabling software triggering.
        // The software trigger configuration handler replaces the default configuration
        // as all currently registered configuration handlers are removed by setting the registration mode to RegistrationMode_ReplaceAll.
        camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);

        // For demonstration purposes only, add sample configuration event handlers to print out information
        // about camera use and image grabbing.
        camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete); // Camera use.

        // For demonstration purposes only, register another image event handler.
        camera.RegisterImageEventHandler( new CSampleImageEventHandler, RegistrationMode_Append, Cleanup_Delete);

        // Camera event processing must be activated first, the default is off.
        camera.GrabCameraEvents = true;


        // Register an event handler for the Exposure End event. For each event type, there is a "data" node
        // representing the event. The actual data that is carried by the event is held by child nodes of the
        // data node. In the case of the Exposure End event, the child nodes are EventExposureEndFrameID and EventExposureEndTimestamp.
        // The CSampleCameraEventHandler demonstrates how to access the child nodes within
        // a callback that is fired for the parent data node.
        // The user-provided ID eMyExposureEndEvent can be used to distinguish between multiple events (not shown).
        camera.RegisterCameraEventHandler( pHandler1, "EventExposureEndData", eMyExposureEndEvent, RegistrationMode_ReplaceAll, Cleanup_None);

        // The handler is registered for both, the EventExposureEndFrameID and the EventExposureEndTimestamp
        // node. These nodes represent the data carried by the Exposure End event.
        // For each Exposure End event received, the handler will be called twice, once for the frame ID, and
        // once for the time stamp.
        camera.RegisterCameraEventHandler( pHandler2, "EventExposureEndFrameID", eMyExposureEndEvent, RegistrationMode_Append, Cleanup_None);
        camera.RegisterCameraEventHandler( pHandler2, "EventExposureEndTimestamp", eMyExposureEndEvent, RegistrationMode_Append, Cleanup_None);


        // Open the camera for setting parameters.
        camera.Open();

        // Check if the device supports events.
        if ( !GenApi::IsAvailable( camera.EventSelector))
        {
            throw RUNTIME_EXCEPTION( "The device doesn't support events.");
        }

        // Enable sending of Exposure End events.
        // Select the event to receive.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);
        // Enable it.
        camera.EventNotification.SetValue(EventNotification_On);

        // Start the grabbing of c_countOfImagesToGrab images.
        camera.StartGrabbing( c_countOfImagesToGrab);

        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        while ( camera.IsGrabbing())
        {
            // Execute the software trigger. Wait up to 1000 ms for the camera to be ready for trigger.
            if ( camera.WaitForFrameTriggerReady( 1000, TimeoutHandling_ThrowException))
            {
                camera.ExecuteSoftwareTrigger();
            }

            // Retrieve grab results and notify the camera event and image event handlers.
            camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);
            // Nothing to do here with the grab result, the grab results are handled by the registered event handler.
        }

        // Disable sending Exposure End events.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);
        camera.EventNotification.SetValue(EventNotification_Off);
    }
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Delete the event handlers.
    delete pHandler1;
    delete pHandler2;

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    // Releases all pylon resources. 
    PylonTerminate(); 

    return exitCode;
}

