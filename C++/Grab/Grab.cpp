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

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 5;


int main(int argc, char* argv[])
{
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