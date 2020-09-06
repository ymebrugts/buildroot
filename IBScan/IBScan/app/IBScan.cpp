#ifdef WIN32
    #include "stdafx.h"
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <sys/time.h>
    #include <unistd.h>
#endif

#include "IBScanUltimateApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

/*******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************/

static void GetConfiguration(int *pDeviceCount);

static void CALLBACK OnPreviewImageAvailable(const int deviceHandle,
    void*                pContext,
    const IBSU_ImageData image );

static void CALLBACK OnFingerCountChange(
    const int                   deviceHandle,
    void*                       pContext,
    const IBSU_FingerCountState fingerCountState );

static void CALLBACK OnFingerQualityChange(
    const int                      deviceHandle,   
    void*                          pContext,       
    const IBSU_FingerQualityState* pQualityArray, 
    const int                      qualityArrayCount);

static void CALLBACK OnDeviceCountChange(
    int   detectedDevices,
    void* pContext );

static void CALLBACK OnInitProgressChange(
    const int deviceIndex,
    void*     pContext,
    const int progressValue );

static void CALLBACK OnDeviceComunicationBreak (
    const int deviceHandle,
    void*     pContext );

static void CALLBACK OnTakingAcquisition (
    const int            deviceHandle,
    void*                pContext,
    const IBSU_ImageType imageType );

static void CALLBACK OnCompleteAcquisition (
    const int            deviceHandle,
    void*                pContext,
    const IBSU_ImageType imageType );

static void CALLBACK OnResultImageAvailableEx (
    const int                   deviceHandle,
    void*                       pContext,
    const int                   imageStatus,
    const IBSU_ImageData        image,
    const IBSU_ImageType        imageType,
    const int                   detectedFingerCount,
    const int                   segmentImageArrayCount,
    const IBSU_ImageData*       pSegmentImageArray,
    const IBSU_SegmentPosition* pSegmentPositionArray);

static BOOL OpenCaptureDevice();

static float FramesPerSecond();

static BOOL StartCapture(IBSU_ImageType imageType);

static void RunCaptureTest();

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

/* Start time of capture to calculate the frame rate. */
static struct timeval m_startTime;

/* Counter for number of preview images to calculate the frame rate. */
static int m_previewCount = 0;

/* The handle of the currently-opened device. */
static int m_deviceHandle = -1;

/* Dummy variable for registering callbacks. */
static char m_context[4];

/* The last finger count state, to detect changes. */
static int m_SavedFingerCountState = -1;

/* Options 0=save Bitmap and WSQ, 1=calculate NFIQ score, 2=set preview image*/
static int m_options[3];

static BOOL m_acquisitionCompleted = FALSE;

static int m_messageNumber = 0;

/*******************************************************************************
 * GetConfiguration()
 *
 * DESCRIPTION:
 *     Get the number of attached scanners.
 ******************************************************************************/

static void GetConfiguration(int *pDeviceCount) 
{
    int nRc = IBSU_GetDeviceCount(pDeviceCount);
    if (nRc < IBSU_STATUS_OK)
    {
        printf("Error %d Retrieving device count\n", nRc);
        return;
    }
    printf("Found %d devices attached\n", *pDeviceCount);
}


/*******************************************************************************
 * OnPreviewImageAvailable()
 * 
 * DESCRIPTION:
 *     The callback that is invoked when a preview image is available.
 ******************************************************************************/
 
static void CALLBACK OnPreviewImageAvailable(
    const int deviceHandle,
    void*     pContext,
    const IBSU_ImageData image) 
{
    printf("C");
    fflush(stdout);
    m_previewCount++;
}

static void sendResponse(string message)
{
    m_messageNumber++;
    ofstream myfile;
    myfile.open("message" + to_string(m_messageNumber) + ".txt");
    myfile << message;
    myfile.close();
}

/*******************************************************************************
 * OnFingerCountChange()
 *
 * DESCRIPTION:
 *     The callback that is invoked when the number of fingers on the platen 
 *     changes.
 ******************************************************************************/
 
static void CALLBACK OnFingerCountChange(
    const int                   deviceHandle,
    void*                       pContext,
    const IBSU_FingerCountState fingerCountState) 
{
    if (m_SavedFingerCountState != (int)fingerCountState)
    {
        const char *pcState;  

        switch (fingerCountState)
        {
            case ENUM_IBSU_FINGER_COUNT_OK:
                pcState = "FINGER_COUNT_OK";
                break;
            case ENUM_IBSU_TOO_MANY_FINGERS:
                pcState = "TOO_MANY_FINGERS";
                break;
            case ENUM_IBSU_TOO_FEW_FINGERS:
                pcState = "TOO_FEW_FINGERS";
                break;
            case ENUM_IBSU_NON_FINGER:
                pcState = "NON-FINGER";
                break;
            default:
                pcState = "UNKNOWN";
                break;
        }

        //printf("\n-- Finger count changed -- Device= %d, State= %s\n", 
        //    deviceHandle, pcState);
        //fflush(stdout);

        printf("\n Sending Response: METHOD_FP_COUNT 0x0%d\n", fingerCountState);

        fflush(stdout);
        sendResponse("METHOD_FP_COUNT 0x0" + to_string(fingerCountState));

        m_SavedFingerCountState = (int)fingerCountState;
    }
}

/*******************************************************************************
 * OnFingerQualityChange()
 *
 * DESCRIPTION:
 *     The callback that is invoked when the quality of fingers on the platen 
 *     changes.
 ******************************************************************************/

static void CALLBACK OnFingerQualityChange(
    const int                      deviceHandle,   
    void*                          pContext,       
    const IBSU_FingerQualityState* pQualityArray, 
    const int                      qualityArrayCount)
{
    //printf("\n-- Finger quality changed -- Device= %d, Qualities= ", deviceHandle);

    for (int i = 0; i < qualityArrayCount; i++)
    {
        const char *pcQuality;

        switch (pQualityArray[i])
        {
            case ENUM_IBSU_FINGER_NOT_PRESENT:
                pcQuality = "NOT_PRESENT";
                break;
            case ENUM_IBSU_QUALITY_GOOD:
                pcQuality = "GOOD";
                break;
            case ENUM_IBSU_QUALITY_FAIR:
                pcQuality = "FAIR";
                break;
            case ENUM_IBSU_QUALITY_POOR:
                pcQuality = "POOR";
                break;
            case ENUM_IBSU_QUALITY_INVALID_AREA_TOP:
                pcQuality = "INVALID_AREA_TOP";
                break;
            case ENUM_IBSU_QUALITY_INVALID_AREA_LEFT:
                pcQuality = "INVALID_AREA_LEFT";
                break;
            case ENUM_IBSU_QUALITY_INVALID_AREA_RIGHT:
                pcQuality = "INVALID_AREA_RIGHT";
                break;
            default:
                pcQuality = "UNKNOWN";
                break;
        }        
        if (i != 0)
        {
            printf(", ");
        }
        //printf("%s", pcQuality);
    }

    string message = "";
    for (int i = 0; i < qualityArrayCount; i++)
    {
            
        if (i == 0)
        {
            printf("\n Sending Response: METHOD_FP_QUALITY ");
        }
        printf("0x0%d ", pQualityArray[i]);
        message += "0x0" + to_string(pQualityArray[i]);
    }

    sendResponse("METHOD_FP_QUALITY " + message);

    printf("\n");
}

/*******************************************************************************
 * OnDeviceCountChange()
 *
 * DESCRIPTION:
 *     The callback that is invoked when the number of attached scanners 
 *     changes.
 ******************************************************************************/

void CALLBACK OnDeviceCountChange(
    int   detectedDevices,
    void* pContext) 
{
   printf("\nDeviceCountChange: now %d devices\n ", detectedDevices);
   fflush(stdout);
}

/*******************************************************************************
 * OnInitProgressChange()
 *
 * DESCRIPTION:
 *     The callback that is invoked when the initialization progresses of a 
 *     scanner being opened.
 ******************************************************************************/

void CALLBACK OnInitProgressChange(
    const int deviceIndex,
    void*     pContext,
    const int progressValue ) 
{
   printf("\nInitializing device... %d%%\n ", progressValue);
   fflush(stdout);
}

/*******************************************************************************
 * OnDeviceComunicationBreak()
 *
 * DESCRIPTION:
 *     The callback that is invoked when the communication with a scanner is 
 *     broken.
 ******************************************************************************/

static void CALLBACK OnDeviceComunicationBreak (
    const int deviceHandle,
    void*     pContext) 
{
    int rc;

    printf("\nDevice Communications break\n");
    rc = IBSU_CloseDevice(deviceHandle);
    if (rc < IBSU_STATUS_OK)
    {
        printf("Error %d closing device\n", rc);
    }
    if (deviceHandle == m_deviceHandle) 
    {
        m_deviceHandle = -1;
    }
    fflush(stdout);
}

/*******************************************************************************
 * OnTakingAcquisition()
 *
 * DESCRIPTION:
 *     The callback that is invoked when rolling acquisition begins.
 ******************************************************************************/

static void CALLBACK OnTakingAcquisition (
    const int            deviceHandle,
    void*                pContext,
    const IBSU_ImageType imageType)
{
    if (imageType == ENUM_IBSU_ROLL_SINGLE_FINGER)
    {
        printf("\nWhen done remove finger from sensor!\n ");
        fflush(stdout);
    }
}

/*******************************************************************************
 * OnCompleteAcquisition()
 *
 * DESCRIPTION:
 *     The callback that is invoked when rolling acquisition completes.
 ******************************************************************************/

static void CALLBACK OnCompleteAcquisition (
    const int            deviceHandle,
    void*                pContext,
    const IBSU_ImageType imageType)
{
    printf("\nCompleteAcquisition\n ");
    fflush(stdout);
}

/*******************************************************************************
 * OnResultImageAvailableEx()
 *
 * DESCRIPTION:
 *     The callback that is invoked when the extended result image is available.
 ******************************************************************************/

static void CALLBACK OnResultImageAvailableEx (
    const int                   deviceHandle,
    void*                       pContext,
    const int                   imageStatus,
    const IBSU_ImageData        image,
    const IBSU_ImageType        imageType,
    const int                   detectedFingerCount,
    const int                   segmentImageArrayCount,
    const IBSU_ImageData*       pSegmentImageArray,
    const IBSU_SegmentPosition* pSegmentPositionArray)
{
    char        saveFileName[128];
    const char *pszImgTypeName;
    double      framesPerSecond;
    int         nRc;
    int         nfiqScore;

    /* Calculate preview frames captured per second during scan. */
    framesPerSecond = FramesPerSecond();
    printf("\nStopped. %1.1f frames per second\n", framesPerSecond);

    /* Inform user that acquisition is complete. */
    pszImgTypeName = "Unknown"; 
    if (imageStatus == IBSU_STATUS_OK)
    {
        switch( imageType )
        {
            case ENUM_IBSU_ROLL_SINGLE_FINGER:
                pszImgTypeName = "Rolling single finger"; 
                break;
            case ENUM_IBSU_FLAT_SINGLE_FINGER:
                pszImgTypeName = "Flat single finger"; 
                break;
            case ENUM_IBSU_FLAT_TWO_FINGERS:
                pszImgTypeName = "Flat two fingers"; 
                break;
            case ENUM_IBSU_FLAT_FOUR_FINGERS:
                pszImgTypeName = "Flat four fingers"; 
                break;
            case ENUM_IBSU_FLAT_THREE_FINGERS:
                pszImgTypeName = "Flat three fingers"; 
                break;
            default:
                pszImgTypeName = "Unknown"; 
                break;
        }
        printf("\n%s Image acquisition complete\n", pszImgTypeName);
    }
    else
    {
        printf("\nImage acquisition completed with status %d\n", imageStatus);
    }

    /* Save result image. */
    printf("\nSaving bitmap,WSQ, PNG and JPEG-2000 images...\n");
    fflush(stdout);
    sprintf(saveFileName, "ResultImage_%s.bmp", pszImgTypeName);
    nRc = IBSU_SaveBitmapImage(saveFileName, (BYTE*)image.Buffer, image.Width,
        image.Height, image.Pitch, image.ResolutionX, image.ResolutionY);
    if (nRc != IBSU_STATUS_OK)
    {
        printf("Failed to save bitmap image!");
        return;
    }
    printf("-- Saved %s\n", saveFileName);
    fflush(stdout);

    sprintf(saveFileName, "ResultImage_%s.wsq", pszImgTypeName);
    nRc = IBSU_WSQEncodeToFile(saveFileName, (BYTE*)image.Buffer, image.Width,
        image.Height, image.Pitch, image.BitsPerPixel, image.ResolutionX, 0.75, "");
    if (nRc != IBSU_STATUS_OK)
    {
        printf("Failed to save WSQ image!");
        return;
    }
    printf("-- Saved %s\n", saveFileName);
    fflush(stdout);

    sprintf(saveFileName, "ResultImage_%s.png", pszImgTypeName);
    nRc = IBSU_SavePngImage(saveFileName, (BYTE*)image.Buffer, image.Width,
        image.Height, image.Pitch, image.ResolutionX, image.ResolutionY);
    if (nRc != IBSU_STATUS_OK)
    {
        printf("Failed to save PNG image!");
        return;
    }
    printf("-- Saved %s\n", saveFileName);
    fflush(stdout);

    sprintf(saveFileName, "ResultImage_%s.jp2", pszImgTypeName);
    nRc = IBSU_SaveJP2Image(saveFileName, (BYTE*)image.Buffer, image.Width,
        image.Height, image.Pitch, image.ResolutionX, image.ResolutionY, 80);
    if (nRc != IBSU_STATUS_OK)
    {
        printf("Failed to save JPEG-2000 image!");
        return;
    }
    printf("-- Saved %s\n", saveFileName);
    fflush(stdout);

    /* Save each image segment for multi-finger scan. */
    if (segmentImageArrayCount > 1)
    {
        for (int i = 0; i < segmentImageArrayCount; i++)
        {
            sprintf(saveFileName, "ResultImage__Segment_%02d.bmp", i);
            nRc = IBSU_SaveBitmapImage(saveFileName, (BYTE*)(pSegmentImageArray+i)->Buffer,
                (pSegmentImageArray+i)->Width,(pSegmentImageArray+i)->Height, (pSegmentImageArray+i)->Pitch, 
                (pSegmentImageArray+i)->ResolutionX, (pSegmentImageArray+i)->ResolutionY);

            if (nRc != IBSU_STATUS_OK) 
            {
                printf("Failed to save segment image!");
                return;
            }
            printf("-- Saved %s\n", saveFileName);
            fflush(stdout);

            sprintf(saveFileName, "ResultImage__Segment_%02d.wsq", i);
            nRc = IBSU_WSQEncodeToFile(saveFileName, (BYTE*)(pSegmentImageArray+i)->Buffer,
                (pSegmentImageArray+i)->Width,(pSegmentImageArray+i)->Height, (pSegmentImageArray+i)->Pitch,
                (pSegmentImageArray+i)->BitsPerPixel, (pSegmentImageArray+i)->ResolutionX, 0.75, "");
            if (nRc != IBSU_STATUS_OK)
            {
                printf("Failed to save segment WSQ image!");
                return;
            }
            printf("-- Saved %s\n", saveFileName);
            fflush(stdout);
        }
    }
	
    //if (m_options[1] == 1)
	//{
		printf("\nCalculating NFIQ score...\n");
		fflush(stdout);
		/* Calculate NFIQ score for result image. */
		nRc = IBSU_GetNFIQScore(deviceHandle, (const BYTE*)image.Buffer, image.Width, 
			image.Height, image.BitsPerPixel, &nfiqScore);
		if (nRc == IBSU_STATUS_OK)
		{
			printf("-- NFIQ score is %d\n", nfiqScore);
		}
		else
		{
			printf("Failed to get NFIQ score\n");
		}
	//}
    
    m_acquisitionCompleted = TRUE;
    printf("\n\nAcquisition completed!\n");
    
    fflush(stdout);
}

/*******************************************************************************
 * OpenCaptureDevice()
 *
 * DESCRIPTION:
 *     Open a capture device and register callbacks.
 ******************************************************************************/
 
static BOOL OpenCaptureDevice() 
{
    char minSDKVersion[IBSU_MAX_STR_LEN]={0};

    /* Open the device. */
    int nRc = IBSU_OpenDeviceEx(0, (LPCSTR)"./", FALSE, &m_deviceHandle);
    if (nRc < IBSU_STATUS_OK) 
    {
		switch (nRc)
		{
		case IBSU_ERR_DEVICE_ACTIVE:
			printf("[Error code = %d] Device initialization failed because in use by another thread/process.\n", nRc );
			break;
		case IBSU_ERR_USB20_REQUIRED:
			printf("[Error code = %d] Device initialization failed because SDK only works with USB 2.0.\n", nRc );
			break;
        case IBSU_ERR_DEVICE_HIGHER_SDK_REQUIRED:
            IBSU_GetRequiredSDKVersion(0, minSDKVersion);
            printf("[Error code = %d] Devcie initialization failed because SDK Version %s is required at least.\n", nRc, minSDKVersion );
            break;
		default:
			printf("[Error code = %d] Device initialization failed\n", nRc );
			break;
		}
        m_deviceHandle = -1;
        return FALSE;
    }

    /* Register callbacks for device events. */
    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_ESSENTIAL_EVENT_PREVIEW_IMAGE, 
        (void *)OnPreviewImageAvailable, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering preview image callback: %d\n", nRc);
        return FALSE;
    }

    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_OPTIONAL_EVENT_FINGER_COUNT, 
        (void *)OnFingerCountChange, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering finger count callback: %d\n", nRc);
        return FALSE;
    }

    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_OPTIONAL_EVENT_FINGER_QUALITY, 
        (void *)OnFingerQualityChange, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering finger quality callback: %d\n", nRc);
        return FALSE;
    }

    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_ESSENTIAL_EVENT_COMMUNICATION_BREAK, 
        (void *)OnDeviceComunicationBreak, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering communication break callback: %d\n", nRc);
        return FALSE;
    }

    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_ESSENTIAL_EVENT_TAKING_ACQUISITION, 
        (void *)OnTakingAcquisition, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering taking acquisition callback: %d\n", nRc);
        return FALSE;
    }

    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_ESSENTIAL_EVENT_COMPLETE_ACQUISITION, 
        (void *)OnCompleteAcquisition, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering complete acquisition callback: %d\n", nRc);
        return FALSE;
    }

    nRc = IBSU_RegisterCallbacks(m_deviceHandle, ENUM_IBSU_ESSENTIAL_EVENT_RESULT_IMAGE_EX, 
        (void *)OnResultImageAvailableEx, m_context );
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering extended result image callback: %d\n", nRc);
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
 * FramesPerSecond()
 *
 * DESCRIPTION:
 *     Calculate the number of preview frames captured per second.
 ******************************************************************************/
 
static float FramesPerSecond() 
{
   struct timeval tv;
   double elapsed = 0.0;

   gettimeofday(&tv, NULL);
   elapsed = (tv.tv_sec - m_startTime.tv_sec) +
     (tv.tv_usec - m_startTime.tv_usec) / 1000000.0;
   return ((float)m_previewCount / elapsed);
}

/*******************************************************************************
 * StartCapture()
 *
 * DESCRIPTION:
 *     Start capture for the specified image type. 
 ******************************************************************************/
 
static BOOL StartCapture(IBSU_ImageType imageType)
{
    BOOL isAvailable = FALSE;
    int  captureOptions;
    int  nRc;

    if(!OpenCaptureDevice()) 
    {
        printf("Problem OpenCaptureDevice\n");
        return FALSE;
    }

    /* Determine whether capture mode is available for scanner. */
    IBSU_IsCaptureAvailable(m_deviceHandle, imageType, 
        ENUM_IBSU_IMAGE_RESOLUTION_500, &isAvailable);
    if(!isAvailable) 
    {
		if(imageType == ENUM_IBSU_FLAT_SINGLE_FINGER)
		{
			printf("Capture mode FLAT_SINGLE_FINGER not available\n");
		}
		else if(imageType == ENUM_IBSU_FLAT_TWO_FINGERS)
		{
			printf("Capture mode FLAT_TWO_FINGERS not available\n");
		}
		else if(imageType == ENUM_IBSU_ROLL_SINGLE_FINGER)
		{
			printf("Capture mode ROLL_SINGLE_FINGER not available\n");
		}
		else
		{
			printf("Invalid capture mode\n");
		}
        return FALSE;
    }

    /* Format capture options. */
    captureOptions  = 0;
    captureOptions |= IBSU_OPTION_AUTO_CONTRAST;
    captureOptions |= IBSU_OPTION_AUTO_CAPTURE;

    /* Begin image capture. */
    nRc = IBSU_BeginCaptureImage(m_deviceHandle, imageType, 
        ENUM_IBSU_IMAGE_RESOLUTION_500, captureOptions);
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Problem starting capture: %d\n", nRc);
        return FALSE;
    }

    printf("Setting up for scan with callback...Displayed 'C'=Image callback.\n");

    /* Iniitialize variables for calculating framerate. */
    m_previewCount = 0;
    gettimeofday(&m_startTime, NULL);

    return TRUE;
}

/*******************************************************************************
 * RunCaptureTest()
 *
 * DESCRIPTION:
 *     Run a capture test.
 ******************************************************************************/
 
static void RunCaptureTest()
{
    StartCapture(ENUM_IBSU_FLAT_TWO_FINGERS);
    
    while(!m_acquisitionCompleted)
    {
        usleep(1000);
    }
}

/*******************************************************************************
 * main()
 * 
 * DESCRIPTION:
 *     Application entry point
 ******************************************************************************/
 
int main(int argc, char* argv[])
{
    int             deviceCount = 0;
    IBSU_SdkVersion version;
    int             nRc;
    
    /* Get the SDK version. */
    nRc = IBSU_GetSDKVersion(&version);
    if (nRc < IBSU_STATUS_OK) 
    {
        printf("Error %d Retrieving version info\n", nRc);
        exit(1);
    }
    printf("IBScanUltimate Product version: %s, File version: %s\n", 
        version.Product, version.File);

    /* Get the number of attached scanners. */
    GetConfiguration(&deviceCount);
    if (deviceCount ==0 ) 
    {
        printf("No IB Scan devices attached... exiting\n");
        exit(1);
    }

    /* Register callbacks for notifications. */
    nRc = IBSU_RegisterCallbacks(0, ENUM_IBSU_ESSENTIAL_EVENT_DEVICE_COUNT, 
        (void *)OnDeviceCountChange, m_context);
    if (nRc < IBSU_STATUS_OK ) 
    {
        printf("Problem registering device count change callback: %d\n", nRc);
        exit(1);
    }
    nRc = IBSU_RegisterCallbacks(0, ENUM_IBSU_ESSENTIAL_EVENT_INIT_PROGRESS, 
        (void *)OnInitProgressChange, m_context);
    if(nRc < IBSU_STATUS_OK) 
    {
        printf("Problem registering init progress change callback: %d\n", nRc);
        exit(1);
    }

	memset(m_options, 0, sizeof(m_options));

    /* 
     * For each scanner, run test. 
     */
    for (int i = 0; i < deviceCount; i++) 
    {
        IBSU_DeviceDesc devDesc;
        int             rc;
        string          strDevice;
        
        printf("\nIBScan 0.0.3.\n");

        /* Get device description. */
        rc = IBSU_GetDeviceDescription(i, &devDesc);
        if (rc < IBSU_STATUS_OK) 
        {
            printf("Error %d Retrieving device description, index # %d\n", rc, i);
            continue;
        }

        /* Format description of scanner. */
        if (devDesc.productName[0] == 0)
        {
            strDevice = "unknown device";
        }
        else 
        {
            strDevice  = devDesc.productName;
            strDevice += "_";
            strDevice += devDesc.fwVersion;
            strDevice += " S/N(";
            strDevice += devDesc.serialNumber;
            strDevice += ") on ";
            strDevice += devDesc.interfaceType;
        }
        cout << strDevice << endl;

        /* Run test. */
        RunCaptureTest();

        /* Close the device handle, if still open. */
        if (m_deviceHandle >= 0) 
        {
            rc = IBSU_CloseDevice(m_deviceHandle);
            if (rc < IBSU_STATUS_OK)
            {
                printf("Error %d closing device\n", rc);
            }
        }

        printf("\nFinished.\n");
    }
}

