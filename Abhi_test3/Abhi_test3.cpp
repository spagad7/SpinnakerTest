//=============================================================================
// Copyright © 2015 Point Grey Research, Inc. All Rights Reserved.
//
// This software is the confidential and proprietary information of
// Point Grey Research, Inc. ("Confidential Information"). You shall not
// disclose such Confidential Information and shall use it only in
// accordance with the terms of the "License Agreement" that you
// entered into with PGR in connection with this software.
//
// UNLESS OTHERWISE SET OUT IN THE LICENSE AGREEMENT, THIS SOFTWARE IS
// PROVIDED ON AN “AS-IS” BASIS AND POINT GREY RESEARCH INC. MAKES NO
// REPRESENTATIONS OR WARRANTIES ABOUT THE SOFTWARE, EITHER EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY IMPLIED WARRANTIES OR
// CONDITIONS OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
// NON-INFRINGEMENT. POINT GREY RESEARCH INC. SHALL NOT BE LIABLE FOR ANY
// DAMAGES, INCLUDING BUT NOT LIMITED TO ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES, OR ANY LOSS OF PROFITS,
// REVENUE, DATA OR DATA USE, ARISING OUT OF OR IN CONNECTION WITH THIS
// SOFTWARE OR OTHERWISE SUFFERED BY YOU AS A RESULT OF USING, MODIFYING
// OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================

/**
 *  @example Trigger.cpp
 *
 *  @brief Trigger.cpp shows how to trigger the camera. It relies on information
 *	provided in the Enumeration, Acquisition, and NodeMapInfo examples.
 *
 *	It can also be helpful to familiarize yourself with the ImageFormatControl
 *	and Exposure examples. As they are somewhat shorter and simpler, either
 *	provides a strong introduction to camera customization.
 *
 *	This example shows the process of configuring, using, and cleaning up a
 *	camera for use with both a software and a hardware trigger.
 */

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;
using namespace cv;

// Use the following enum and global constant to select whether a software or
// hardware trigger is used.
enum triggerType
{
	SOFTWARE,
	HARDWARE
};

const triggerType chosenTrigger = SOFTWARE;

// This function configures the camera to use a trigger. First, trigger mode is
// set to off in order to select the trigger source. Once the trigger source
// has been selected, trigger mode is then enabled, which has the camera
// capture only a single image upon the execution of the chosen trigger.
int ConfigureTrigger(INodeMap & nodeMap)
{
	int result = 0;

	cout << endl << endl << "*** CONFIGURING TRIGGER ***" << endl << endl;

	if (chosenTrigger == SOFTWARE)
	{
		cout << "Software trigger chosen..." << endl;
	}
	else if (chosenTrigger == HARDWARE)
	{
		cout << "Hardware trigger chosen..." << endl;
	}

	try
	{
		//
		// Ensure trigger mode off
		//
		// *** NOTES ***
		// The trigger must be disabled in order to configure whether the source
		// is software or hardware.
		//
		CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
		if (!IsAvailable(ptrTriggerMode) || !IsReadable(ptrTriggerMode))
		{
			cout << "Unable to disable trigger mode (node retrieval). Aborting..." << endl;
			return -1;
		}

		CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
		if (!IsAvailable(ptrTriggerModeOff) || !IsReadable(ptrTriggerModeOff))
		{
			cout << "Unable to disable trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

		cout << "Trigger mode disabled..." << endl;

		//
		// Select trigger source
		//
		// *** NOTES ***
		// The trigger source must be set to hardware or software while trigger
		// mode is off.
		//
		CEnumerationPtr ptrTriggerSource = nodeMap.GetNode("TriggerSource");
		if (!IsAvailable(ptrTriggerSource) || !IsWritable(ptrTriggerSource))
		{
			cout << "Unable to set trigger mode (node retrieval). Aborting..." << endl;
			return -1;
		}

		if (chosenTrigger == SOFTWARE)
		{
			// Set trigger mode to software
			CEnumEntryPtr ptrTriggerSourceSoftware = ptrTriggerSource->GetEntryByName("Software");
			if (!IsAvailable(ptrTriggerSourceSoftware) || !IsReadable(ptrTriggerSourceSoftware))
			{
				cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
				return -1;
			}

			ptrTriggerSource->SetIntValue(ptrTriggerSourceSoftware->GetValue());

			cout << "Trigger source set to software..." << endl;
		}
		else if (chosenTrigger == HARDWARE)
		{
			// Set trigger mode to hardware ('Line0')
			CEnumEntryPtr ptrTriggerSourceHardware = ptrTriggerSource->GetEntryByName("Line0");
			if (!IsAvailable(ptrTriggerSourceHardware) || !IsReadable(ptrTriggerSourceHardware))
			{
				cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
				return -1;
			}

			ptrTriggerSource->SetIntValue(ptrTriggerSourceHardware->GetValue());

			cout << "Trigger source set to hardware..." << endl;
		}

		//
		// Turn trigger mode on
		//
		// *** LATER ***
		// Once the appropriate trigger source has been set, turn trigger mode
		// on in order to retrieve images using the trigger.
		//
		CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
		if (!IsAvailable(ptrTriggerModeOn) || !IsReadable(ptrTriggerModeOn))
		{
			cout << "Unable to enable trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());

		cout << "Trigger mode turned back on..." << endl << endl;
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

// This function retrieves a single image using the trigger. In this example,
// only a single image is captured and made available for acquisition - as such,
// attempting to acquire two images for a single trigger execution would cause
// the example to hang. This is different from other examples, whereby a
// constant stream of images are being captured and made available for image
// acquisition.
int GrabNextImageByTrigger(INodeMap & nodeMap, CameraPtr pCam)
{
	int result = 0;

	try
	{
		//
		// Use trigger to capture image
		//
		// *** NOTES ***
		// The software trigger only feigns being executed by the Enter key;
		// what might not be immediately apparent is that there is not a
		// continuous stream of images being captured; in other examples that
		// acquire images, the camera captures a continuous stream of images.
		// When an image is retrieved, it is plucked from the stream.
		//
		if (chosenTrigger == SOFTWARE)
		{
			// Get user input
			cout << "Press the Enter key to initiate software trigger." << endl;
			getchar();

			// Execute software trigger
			CCommandPtr ptrSoftwareTriggerCommand = nodeMap.GetNode("TriggerSoftware");
			if (!IsAvailable(ptrSoftwareTriggerCommand) || !IsWritable(ptrSoftwareTriggerCommand))
			{
				cout << "Unable to execute trigger. Aborting..." << endl;
				return -1;
			}

			ptrSoftwareTriggerCommand->Execute();
		}
		else if (chosenTrigger == HARDWARE)
		{
			// Execute hardware trigger
			cout << "Use the hardware to trigger image acquisition." << endl;
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

// This function returns the camera to a normal state by turning off trigger
// mode.
int ResetTrigger(INodeMap & nodeMap)
{
	int result = 0;

	try
	{
		//
		// Turn trigger mode back off
		//
		// *** NOTES ***
		// Once all images have been captured, turn trigger mode back off to
		// restore the camera to a clean state.
		//
		CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
		if (!IsAvailable(ptrTriggerMode) || !IsReadable(ptrTriggerMode))
		{
			cout << "Unable to disable trigger mode (node retrieval). Non-fatal error..." << endl;
			return -1;
		}

		CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
		if (!IsAvailable(ptrTriggerModeOff) || !IsReadable(ptrTriggerModeOff))
		{
			cout << "Unable to disable trigger mode (enum entry retrieval). Non-fatal error..." << endl;
			return -1;
		}

		ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

		cout << "Trigger mode disabled..." << endl << endl;
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

// This function prints the device information of the camera from the transport
// layer; please see NodeMapInfo example for more in-depth comments on printing
// device information from the nodemap.
#if 0
int PrintDeviceInfo(INodeMap & nodeMap)
{
	int result = 0;

	cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;

	try
	{
		FeatureList_t features;
		CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
		if (IsAvailable(category) && IsReadable(category))
		{
			category->GetFeatures(features);

			FeatureList_t::const_iterator it;
			for (it = features.begin(); it != features.end(); ++it)
			{
				CNodePtr pfeatureNode = *it;
				cout << pfeatureNode->GetName() << " : ";
				CValuePtr pValue = (CValuePtr)pfeatureNode;
				cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
				cout << endl;
			}
		}
		else
		{
			cout << "Device control information not available." << endl;
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}
#endif

// This function acquires and saves 10 images from a device; please see
// Acquisition example for more in-depth comments on acquiring images.
int AcquireImages(CameraList camList, INodeMap & nodeMap/*, INodeMap & nodeMapTLDevice*/)
{
	int result = 0;
	CameraPtr pCam = NULL;

	cout << endl << "*** IMAGE ACQUISITION ***" << endl << endl;

	try
	{
		//
		// Prepare each camera to acquire images
		//
		// *** NOTES ***
		// For pseudo-simultaneous streaming, each camera is prepared as if it
		// were just one, but in a loop. Notice that cameras are selected with
		// an index. We demonstrate pseduo-simultaneous streaming because true
		// simultaneous streaming would require multiple process or threads,
		// which is too complex for an example.
		//
		// Serial numbers are the only persistent objects we gather in this
		// example, which is why a vector is created.
		//

		for (int i = 0; i < camList.GetSize(); i++)
		{
			// Select camera
			pCam = camList.GetByIndex(i);
		    // Set acquisition mode to continuous
			CEnumerationPtr ptrAcquisitionMode
				= pCam->GetNodeMap().GetNode("AcquisitionMode");
			if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
			{
				cout << "Unable to set acquisition mode to continuous (node retrieval; camera " << i << "). Aborting..." << endl << endl;
				return -1;
			}

		    CEnumEntryPtr ptrAcquisitionModeContinuous
		        = ptrAcquisitionMode->GetEntryByName("Continuous");
		    if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
		    {
			    cout << "Unable to set acquisition mode to continuous (entry 'continuous' retrieval " << i << "). Aborting..." << endl << endl;
			    return -1;
		    }

		    int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

		    ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

		    cout << "Camera " << i << " acquisition mode set to continuous..." << endl;

		    // Begin acquiring images
		    pCam->BeginAcquisition();

		    cout << "Camera " << i << " started acquiring images..." << endl;
		}

		//
		// Retrieve, convert, and save images for each camera
		//
		// *** NOTES ***
		// In order to work with simultaneous camera streams, nested loops are
		// needed. It is important that the inner loop be the one iterating
		// through the cameras; otherwise, all images will be grabbed from a
		// single camera before grabbing any images from another.
		//
		char key = 0;
		cv::Mat src[2];
		string label = "Cam";

		while(key!='q')
		{
			for (int i = 0; i < camList.GetSize(); i++)
			{
				try
				{
					// Select camera
					pCam = camList.GetByIndex(i);

				    // Retrieve the next image from the trigger
					ImagePtr pResultImage = GrabNextImageByTrigger(nodeMap, pCam);
				    result = result | pResultImage;
#if 0
				    // Retrieve the next received image
				    ImagePtr pResultImage = pCam->GetNextImage();
#endif
				    if (pResultImage->IsIncomplete())
				    {
				        cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << endl << endl;
				    }
				    else
				    {
#if 0
					    // Print image information
					    cout << "Grabbed image " << imageCnt << ", width = " << pResultImage->GetWidth() <<
                            ", height = " << pResultImage->GetHeight() << endl;
#endif
					    // Convert image to mono 8
						ImagePtr convertedImage
					        = pResultImage->Convert(PixelFormat_BGR8, HQ_LINEAR);
						unsigned int rowBytes
							= (int)convertedImage->GetImageSize()/convertedImage->GetHeight();


						src[i] = cv::Mat(convertedImage->GetHeight(),
							convertedImage->GetWidth(), CV_8UC1, convertedImage->GetData(),
							rowBytes);

						cv::resize(src[i], src[i], Size(640, 480), 0,0, INTER_LINEAR);

						cv::namedWindow("image", 1);
						cv::imshow(label+to_string(i), src[i]);
						key = cv::waitKey(1);
					}

					pResultImage->Release();

				}
				catch (Spinnaker::Exception &e)
				{
					cout << "Error: " << e.what() << endl;
					result = -1;
				}
			}
		}

		//
		// End acquisition for each camera
		//
		// *** NOTES ***
		// Notice that what is usually a one-step process is now two steps
		// because of the additional step of selecting the camera. It is worth
		// repeating that camera selection needs to be done once per loop.
		//
		// It is possible to interact with cameras through the camera list with
		// GetByIndex(); this is an alternative to retrieving cameras as
		// CameraPtr objects that can be quick and easy for small tasks.
		//
		for (int i = 0; i < camList.GetSize(); i++)
		{
			// End acquisition
			camList.GetByIndex(i)->EndAcquisition();
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

// This function acts as the body of the example; please see NodeMapInfo example
// for more in-depth comments on setting up cameras.
int RunMultipleCameras(CameraList camList)
{
	int result = 0;
	int err = 0;
	CameraPtr pCam = NULL;

	try
	{
		//
		// Retrieve transport layer nodemaps and print device information for
		// each camera
		//
		// *** NOTES ***
		// This example retrieves information from the transport layer nodemap
		// twice: once to print device information and once to grab the device
		// serial number. Rather than caching the nodemap, each nodemap is
		// retrieved both times as needed.
		//
		cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;

		//NodeMap & nodeMapTLDevice = NULL;
		for (int i = 0; i < camList.GetSize(); i++)
		{
			// Select camera
			pCam = camList.GetByIndex(i);

			// Retrieve TL device nodemap
			//nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

			// Print device information
			//result = PrintDeviceInfo(nodeMapTLDevice, i);
		}

		//
		// Initialize each camera
		//
		// *** NOTES ***
		// You may notice that the steps in this function have more loops with
		// less steps per loop; this contrasts the AcquireImages() function
		// which has less loops but more steps per loop. This is done for
		// demonstrative purposes as both work equally well.
		//
		// *** LATER ***
		// Each camera needs to be deinitialized once all images have been
		// acquired.
		//
		for (int i = 0; i < camList.GetSize(); i++)
		{
			// Select camera
			pCam = camList.GetByIndex(i);

			// Initialize camera
			pCam->Init();
		}

		// Retrieve GenICam nodemap
		INodeMap & nodeMap = camList.GetByIndex(0)->GetNodeMap();

		// Configure trigger
		err = ConfigureTrigger(nodeMap);
		if (err < 0)
		{
			return err;
		}

		// Acquire images on all cameras
		result = result | AcquireImages(camList, nodeMap/*, nodeMapTLDevice*/);

		// Reset trigger
		result = result | ResetTrigger(nodeMap);

		//
		// Deinitialize each camera
		//
		// *** NOTES ***
		// Again, each camera must be deinitialized separately by first
		// selecting the camera and then deinitializing it.
		//
		for (int i = 0; i < camList.GetSize(); i++)
		{
			// Select camera
			pCam = camList.GetByIndex(i);

			// Deinitialize camera
			pCam->DeInit();
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

// Example entry point; please see Enumeration example for more in-depth
// comments on preparing and cleaning up the system.
int main(int /*argc*/, char** /*argv*/)
{
	int result = 0;

	// Print application build information
	cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

	// Retrieve singleton reference to system object
	SystemPtr system = System::GetInstance();

	// Retrieve list of cameras from the system
	CameraList camList = system->GetCameras();

	unsigned int numCameras = camList.GetSize();

	cout << "Number of cameras detected: " << numCameras << endl << endl;

	// Finish if there are no cameras
	if (numCameras == 0)
	{
		// Clear camera list before releasing system
		camList.Clear();

		// Release system
		system->ReleaseInstance();

		cout << "Not enough cameras!" << endl;
		cout << "Done! Press Enter to exit..." << endl;
		getchar();

		return -1;
	}

	// Run example on all cameras
	cout << endl << "Running example for all cameras..." << endl;

	result = RunMultipleCameras(camList);

	cout << "Example complete..." << endl << endl;

	// Clear camera list before releasing system
	camList.Clear();

	// Release system
	system->ReleaseInstance();

	cout << endl << "Done! Press Enter to exit..." << endl;
	getchar();

	return result;
}
