//
//  Camera.cpp
//  ToysPhotoBoothOFApp
//
//  Created by mg on 21/05/17.
//
//

#include "Camera.h"
#include "CameraBrowser.h"




const EdsImageQuality imageQualities[] =
{
    EdsImageQuality_LJF,	/* Jpeg Large Fine - 5760x3240 */
    EdsImageQuality_LJN,	/* Jpeg Large Normal - 5760x3240 */
    EdsImageQuality_MJF,	/* Jpeg Middle Fine - 3840x2160 */
    EdsImageQuality_MJN,	/* Jpeg Middle Normal - 3840x2160 */
    EdsImageQuality_S1JF,	/* Jpeg Small1 Fine - 2880x1624 */
    EdsImageQuality_S1JN,	/* Jpeg Small1 Normal - 2880x1624 */
    EdsImageQuality_S2JF,	/* Jpeg Small2 - 1920x1080 */
    EdsImageQuality_S3JF,	/* Jpeg Small3 - 720x400 */ }
;

const EdsEvfAFMode evfAFModes[] =
{
    Evf_AFMode_Quick,
    Evf_AFMode_Live,
    Evf_AFMode_LiveFace,
    Evf_AFMode_LiveMulti
};

/*
const EdsUInt32 ISOSpeeds[] =
{
    0x00000000, // Auto
    0x00000040, // 50
    0x00000048, // 100
    0x0000004b, // 125
    0x0000004d, // 160
    0x00000050, // 200
    0x00000053, // 250
    0x00000055, // 320
    0x00000058, // 400
    0x0000005b, // 500
    0x0000005d, // 640
    0x00000060, // 800
    0x00000063, // 1000
    0x00000065, // 1250
    0x00000068, // 1600
    0x0000006b, // 2000
    0x0000006d, // 2500
    0x00000070, // 3200
    0x00000073, // 4000
    0x00000075, // 5000
    0x00000078, // 6400
    0x0000007b, // 8000
    0x0000007d, // 10000
    0x00000080, // 12800
    0x00000088, // 25600
    0x00000090, // 51200
    0x00000098  // 102400
};
*/



/*
 This controls the size of livePixelsMiddle. If you are running at a low fps
 (lower than camera fps), then it will effectively correspond to the latency of the
 camera. If you're running higher than camera fps, it will determine how many frames
 you can miss without dropping one. For example, if you are running at 60 fps
 but one frame happens to last 200 ms, and your buffer size is 4, you will drop
 2 frames if your camera is running at 30 fps.
 */
#define OFX_EDSDK_BUFFER_SIZE 1

#define OFX_EDSDK_JPG_FORMAT 14337
#define OFX_EDSDK_MOV_FORMAT 45316

#if defined(TARGET_WIN32)
#define _WIN32_DCOM
#include <objbase.h>
#endif

namespace ofxEdsdkCiPort {
    
#pragma mark CAMERA FILE
    
    CameraFileRef CameraFile::create(const EdsDirectoryItemRef& directoryItem) {
        return CameraFileRef(new CameraFile(directoryItem))->shared_from_this();
    }
    
    CameraFile::CameraFile(const EdsDirectoryItemRef& directoryItem) {
        if (!directoryItem) {
            ofLogError() << "don't have this directory" << endl;
            //throw Exception();
        }
        
        EdsRetain(directoryItem);
        mDirectoryItem = directoryItem;
        
        EdsError error = EdsGetDirectoryItemInfo(mDirectoryItem, &mDirectoryItemInfo);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get directory item info" << endl;
            
        }
    }
    
    CameraFile::~CameraFile() {
        EdsRelease(mDirectoryItem);
        mDirectoryItem = NULL;
    }
    
#pragma mark - CAMERA
    
    CameraRef Camera::create(const EdsCameraRef& camera) {
        return CameraRef(new Camera(camera))->shared_from_this();
    }
    
    Camera::Camera(const EdsCameraRef& camera) : mHasOpenSession(false), mIsLiveViewActive(false) {
        
        if (!camera) {
            ofLogError() << "camera is null" << endl;
            //throw Exception();
        }
        
        EdsRetain(camera);
        mCamera = camera;
        
        needToTakeContinuousPhotos = false;
        
        EdsError error = EdsGetDeviceInfo(mCamera, &mDeviceInfo);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get device info" << endl;
            // TODO - NULL out mDeviceInfo
        }
        
        // set event handlers
        error = EdsSetObjectEventHandler(mCamera, kEdsObjectEvent_All, Camera::handleObjectEvent, this);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set object event handler" << std::endl;
        }
        error = EdsSetPropertyEventHandler(mCamera, kEdsPropertyEvent_All, Camera::handlePropertyEvent, this);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set property event handler" << std::endl;
        }
        error = EdsSetCameraStateEventHandler(mCamera, kEdsStateEvent_All, Camera::handleStateEvent, this);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set object event handler" << std::endl;
        }
        
        
        livePixelsBack = new ofPixels();
        liveBufferBack = new ofBuffer();
    }
    
    
    
    Camera::~Camera() {
        mRemovedHandler = NULL;
        needToTakeContinuousPhotos = false;
        
        mFileAddedHandler = NULL;
        
        if (mIsLiveViewActive) {
            requestStopLiveView();
        }
        
        if (mHasOpenSession) {
            requestCloseSession();
        }
        
        // NB - starting with EDSDK 2.10, this release will cause an EXC_BAD_ACCESS (code=EXC_I386_GPFLT) at the end of the runloop
        //    EdsRelease(mCamera);
        mCamera = nullptr;
    }
    
#pragma mark -

    
    void Camera::connectRemovedHandler(const std::function<void(CameraRef)>& handler) {
        mRemovedHandler = handler;
    }
    
    void Camera::connectFileAddedHandler(const std::function<void(CameraRef, CameraFileRef)>& handler) {
        mFileAddedHandler = handler;
    }
    
    EdsError Camera::requestOpenSession(const Settings &settings) {
        if (mHasOpenSession) {
            return EDS_ERR_SESSION_ALREADY_OPEN;
        }
        
        EdsError error = EdsOpenSession(mCamera);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to open camera session" << std::endl;
            return error;
        }
        mHasOpenSession = true;
        
        mShouldKeepAlive = settings.getShouldKeepAlive();
        EdsUInt32 saveTo = kEdsSaveTo_Host;//settings.getPictureSaveLocation();
        error = EdsSetPropertyData(mCamera, kEdsPropID_SaveTo, 0, sizeof(saveTo) , &saveTo);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set save destination host/device" << std::endl;
            return error;
        }
        
        //if(error == EDS_ERR_OK){
            //error = EdsSendStatusCommand(mCamera, kEdsCameraStatusCommand_UILock, 0);
        //}
    
    
        if(error == EDS_ERR_OK && saveTo == kEdsSaveTo_Host) {
            // ??? - requires UI lock?
            EdsCapacity capacity = {0x7FFFFFFF, 0x1000, 1};
            error = EdsSetCapacity(mCamera, capacity);
            if (error != EDS_ERR_OK) {
                ofLogError() << "ERROR - failed to set capacity of host" << std::endl;
                return error;
            }
        }
        
        return EDS_ERR_OK;
    }
    
    EdsError Camera::requestCloseSession() {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsCloseSession(mCamera);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to close camera session" << std::endl;
            return error;
        }
        
        mHasOpenSession = false;
        return EDS_ERR_OK;
    }
    
    EdsError Camera::requestTakePicture() {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        //
        
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_TakePicture, 0);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to take picture" << std::endl;
        }
        return error;
    }
    
    
    
    EdsError Camera::requestTakePictureNonAF(){
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_Completely_NonAF);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to send: ShutterButton_Halfway" << std::endl;
        }else{
            //requestHalfWayshutterReleaseButton();
        }
        return error;
    }
    
    
    EdsError Camera::requestHalfWayshutterPressButton(){
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_Halfway);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to send: ShutterButton_Halfway" << std::endl;
        }
        return error;
    }
    
    EdsError Camera::requestHalfWayshutterReleaseButton(){
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_OFF);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to send: ShutterButton_OFF" << std::endl;
        }
        return error;
    }
    
    
    
    EdsError Camera::requestStartFileRec(){
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsUInt32 saveTo = kEdsSaveTo_Camera;
        EdsSetPropertyData(mCamera, kEdsPropID_SaveTo, 0, sizeof(saveTo) , &saveTo);
        
        EdsUInt32 record_start = 4; // Begin movie shooting
        EdsSetPropertyData(mCamera, kEdsPropID_Record, 0, sizeof(record_start), &record_start);
       
            
        
    }
    
    
     EdsError Camera::requestStopFileRec(){
         if (!mHasOpenSession) {
             return EDS_ERR_SESSION_NOT_OPEN;
         }
         
        EdsUInt32 record_stop = 0; // End movie shooting
        EdsSetPropertyData(mCamera, kEdsPropID_Record, 0, sizeof(record_stop), &record_stop);
            
         
    }
    
    EdsError Camera::requestStopContinuousPicture(){
        
        needToTakeContinuousPhotos = false;
        continuousPhotosCounter = 0;
        continuousPhotosNeedNumber = 0;
        
        EdsUInt32 ShutterButtonMode = kEdsCameraCommand_ShutterButton_OFF;
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_PressShutterButton, ShutterButtonMode);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to take picture" << std::endl;
        }
        return error;
        
        
    }
    
    EdsError Camera::requestTakeContinuousPicture(int count) {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        
        ;
        
        needToTakeContinuousPhotos = true;
        continuousPhotosCounter = 0;
        continuousPhotosNeedNumber = count;
        
        
        //EdsUInt32 ShutterButtonMode = kEdsCameraCommand_ShutterButton_OFF;
        //Eds::SendCommand(camera, kEdsCameraCommand_PressShutterButton, ShutterButtonMode);
        
        
        
        EdsUInt32 ShutterButtonMode = kEdsCameraCommand_ShutterButton_Completely;
        
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_PressShutterButton, ShutterButtonMode);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to take picture" << std::endl;
        }
        return error;
    }
    
    
    void Camera::requestDownloadFile(const CameraFileRef& file, const string destinationFolderPath, const std::function<void(EdsError error, string outputFilePath)>& callback, std::string fileName) {
        // check if destination exists and create if not
        
        
        ofDirectory dir(destinationFolderPath);
        
        if(!dir.exists()){
            bool status = dir.create(true);
            
            if (!status) {
                ofLogError() << "ERROR - failed to create destination folder path '" << destinationFolderPath << "'" << std::endl;
                return callback(EDS_ERR_INTERNAL_ERROR, "");
            }
        }
        
        std::ostringstream sstream;
        //sstream << index << "_" << file->getName();
        sstream << fileName << ".jpg";
        std::string query = sstream.str();
        
        
        string filePath = destinationFolderPath + query;
        
        
        EdsStreamRef stream = NULL;
        EdsError error = EdsCreateFileStream(filePath.c_str(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to create file stream" << std::endl;
            goto download_cleanup;
        }
        
        
        
        error = EdsDownload(file->mDirectoryItem, file->getSize(), stream);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to download" << std::endl;
            goto download_cleanup;
        }
        
        error = EdsDownloadComplete(file->mDirectoryItem);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to mark download as complete" << std::endl;
            goto download_cleanup;
        }
        
    download_cleanup:
        if (stream) {
            EdsRelease(stream);
        }
        
        callback(error, filePath);
    }
    /*
    void Camera::requestReadFile(const CameraFileRef& file, const std::function<void(EdsError error, SurfaceRef surface)>& callback) {
        BufferRef buffer = nullptr;
        SurfaceRef surface = nullptr;
        
        EdsStreamRef stream = NULL;
        EdsError error = EdsCreateMemoryStream(0, &stream);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to create memory stream" << std::endl;
            goto read_cleanup;
        }
        
        error = EdsDownload(file->mDirectoryItem, file->getSize(), stream);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to download" << std::endl;
            goto read_cleanup;
        }
        
        error = EdsDownloadComplete(file->mDirectoryItem);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to mark download as complete" << std::endl;
            goto read_cleanup;
        }
        
        void* data;
        error = EdsGetPointer(stream, (EdsVoid**)&data);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get pointer from stream" << std::endl;
            goto read_cleanup;
        }
        
        EdsUInt64 length;
        error = EdsGetLength(stream, &length);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get stream length" << std::endl;
            goto read_cleanup;
        }
        
        buffer = Buffer::create(data, length);
        surface = Surface::create(loadImage(DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg"));
        
    read_cleanup:
        if (stream) {
            EdsRelease(stream);
        }
        
        callback(error, surface);
    }
    */
    
    EdsError Camera::requestStartLiveView() {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        if (mIsLiveViewActive) {
            return EDS_ERR_INTERNAL_ERROR;
        }
        
        EdsUInt32 device;
        EdsError error = EdsGetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get output device for Live View" << std::endl;
            return error;
        }
        
        // connect PC to Live View output device
        device |= kEdsEvfOutputDevice_PC;
        error = EdsSetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set output device to connect PC to Live View output device" << std::endl;
            return error;
        }
        
        mIsLiveViewActive = true;
        
        return EDS_ERR_OK;
        
        
        
        
        
        
        
        
    }
    
    EdsError Camera::requestStopLiveView() {
        
        
        if (!mIsLiveViewActive) {
            return EDS_ERR_INTERNAL_ERROR;
        }
        
        EdsUInt32 device;
        EdsError error = EdsGetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get output device for Live View" << std::endl;
            return error;
        }
        
        // disconnect PC from Live View output device
        
        //kEdsEvfOutputDevice_PC
        //kEdsEvfOutputDevice_TFT
        
        EdsUInt32 outputDevice = 0;
        
        error = EdsSetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(outputDevice), &outputDevice);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set output device to disconnect PC from Live View output device" << std::endl;
            return error;
        }
        
        mIsLiveViewActive = false;
        return EDS_ERR_OK;
        
    }
    
    std::string Camera::requestCameraSerial() {
        EdsDataType propType;
        EdsUInt32 propSize = 0;
        
        
        EdsError error = EdsGetPropertySize(mCamera, kEdsPropID_BodyIDEx, 0, &propType, &propSize);
        if (error == EDS_ERR_OK && propSize > 0)
        {
            char uidCString[propSize];
            
            error = EdsGetPropertyData(mCamera, kEdsPropID_BodyIDEx, 0, propSize, &uidCString);
            
            if (error == EDS_ERR_OK)
            {
                //ofLogError() << "camera serial:" << std::string(uidCString) << std::endl;
                return std::string(uidCString);
            }
        }
        
        return "";
    }
    
    
    
    
    void Camera::startLiveView(){
        if (!mIsLiveViewActive) {
            requestStartLiveView();
        }
    }
    
    void Camera::stopLiveView(){
        if (mIsLiveViewActive) {
            requestStopLiveView();
        }
    }
    
    void Camera::toggleLiveView() {
        if (mIsLiveViewActive) {
            requestStopLiveView();
        } else {
            requestStartLiveView();
        }
    }
    
    
    
    EdsError Camera::setImageQuality(EdsUInt32 value)
    {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsSetPropertyData(mCamera, kEdsPropID_ImageQuality, 0, sizeof(value), &value);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set ImageQuality" << std::endl;
            return error;
        }
        
        return EDS_ERR_OK;
    }
    
    EdsError Camera::setIsoSpeed(EdsUInt32 value)
    {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsSetPropertyData(mCamera, kEdsPropID_ISOSpeed, 0, sizeof(value), &value);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set IsoSpeed" << std::endl;
            return error;
        }
        
        return EDS_ERR_OK;
    }
    
    
    EdsError Camera::setAEMode(EdsUInt32 value)
    {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsSetPropertyData(mCamera, kEdsPropID_AEModeSelect, 0, sizeof(value), &value);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set AEMode" << std::endl;
            return error;
        }
        
        return EDS_ERR_OK;
    }
    
    
    EdsError Camera::setWhiteBalance(EdsUInt32 value)
    {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsSetPropertyData(mCamera, kEdsPropID_WhiteBalance, 0, sizeof(value), &value);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to set WhiteBalance" << std::endl;
            return error;
        }
        
        return EDS_ERR_OK;
    }
    
    
    
    
    
    
    EdsError Camera::setAperture(float aperture)
    {
        auto encoded = encodeAperture(aperture);
        
        if (encoded != 0xFFFFFFFF) {
            if (!mHasOpenSession) {
                return EDS_ERR_SESSION_NOT_OPEN;
            }
            
            EdsError error = EdsSetPropertyData(mCamera, kEdsPropID_Av, 0, sizeof(encoded), &encoded);
            if (error != EDS_ERR_OK) {
                ofLogError() << "ERROR - failed to set Aperture" << std::endl;
                return error;
            }
        }
        
        return EDS_ERR_OK;
    }
    
    
    
    EdsError Camera::setShutterSpeed(EdsUInt32 value)
    {
        //auto encoded = encodeShutterSpeed(shutterSpeed);
        //if (encoded != 0xFFFFFFFF) {
            if (!mHasOpenSession) {
                return EDS_ERR_SESSION_NOT_OPEN;
            }
            
            EdsError error = EdsSetPropertyData(mCamera, kEdsPropID_Tv, 0, sizeof(value), &value);
            if (error != EDS_ERR_OK) {
                ofLogError() << "ERROR - failed to set ShutterSpeed" << std::endl;
                return error;
            }
        //}
        
        return EDS_ERR_OK;
    }
    
    
    
    
    
    
    void Camera::requestLiveViewImage(const std::function<void(EdsError error, ofPixels livePixels)>& callback) {
        EdsError error = EDS_ERR_OK;
        EdsStreamRef stream = NULL;
        EdsEvfImageRef evfImage = NULL;
        
                
        if (!mIsLiveViewActive) {
            error = EDS_ERR_INTERNAL_ERROR;
            goto cleanup;
        }
        
        error = EdsCreateMemoryStream(0, &stream);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to create memory stream" << std::endl;
            goto cleanup;
        }
        
        error = EdsCreateEvfImageRef(stream, &evfImage);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to create Evf image" << std::endl;
            goto cleanup;
        }
        
        error = EdsDownloadEvfImage(mCamera, evfImage);
        if (error != EDS_ERR_OK) {
            if (error == EDS_ERR_OBJECT_NOTREADY) {
                ofLogError() << "ERROR - failed to download Evf image, not ready yet" << std::endl;
            } else {
                ofLogError() << "ERROR - failed to download Evf image" << std::endl;
            }
            goto cleanup;
        }
        
        EdsUInt64 length;
        error = EdsGetLength(stream, &length);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get Evf image length" << std::endl;
            goto cleanup;
        }
        if (length == 0) {
            goto cleanup;
        }
        
        char* data;
        error = EdsGetPointer(stream, (EdsVoid**)&data);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get pointer from stream" << std::endl;
            goto cleanup;
        }
        
        
        liveBufferBack->set(data, length);
        ofLoadImage(*livePixelsBack, *liveBufferBack);
       
        
        
        
        livePixels = *livePixelsBack;
        
        /*
        if(liveTexture.getWidth() != livePixels.getWidth() ||
           liveTexture.getHeight() != livePixels.getHeight()) {
            liveTexture.allocate(livePixels.getWidth(), livePixels.getHeight(), GL_RGB8);
        }
        liveTexture.loadData(livePixels);
        */
        
        
        //buffer = Buffer::create(data, length);
        //surface = Surface::create(loadImage(DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg"));
        
    cleanup:
        if (stream) {
            EdsRelease(stream);
        }
        if (evfImage) {
            EdsRelease(evfImage);
        }
        
        
        callback(error, livePixels);
        //callback(error, liveTexture);
    }
    
    
#pragma mark - CALLBACKS
    
EdsError EDSCALLBACK Camera::handleObjectEvent(EdsUInt32 event, EdsBaseRef ref, EdsVoid* context) {
    Camera* c = (Camera*)context;
    CameraRef camera = CameraBrowser::instance()->cameraForPortName(c->getPortName());
    switch (event) {
        case kEdsObjectEvent_DirItemRequestTransfer: {
            
            EdsDirectoryItemRef directoryItem = (EdsDirectoryItemRef)ref;
            CameraFileRef file = nullptr;
            try {
                file = CameraFile::create(directoryItem);
            } catch (...) {
                EdsRelease(directoryItem);
                break;
            }
            EdsRelease(directoryItem);
            directoryItem = NULL;
            if (camera->mFileAddedHandler) {
                camera->mFileAddedHandler(camera, file);
            }
            ofSleepMillis(200);
            if(camera->needToTakeContinuousPhotos == true){
                camera->continuousPhotosCounter++;
                ofLogError() << " camera->continuousPhotosCounter:" << camera->continuousPhotosCounter <<std::endl;
                if(camera->continuousPhotosCounter==camera->continuousPhotosNeedNumber){
                    camera->requestStopContinuousPicture();
                }
            }
            break;
        }
        default:
            if (ref) {
                EdsRelease(ref);
                ref = NULL;
            }
            break;
    }
    return EDS_ERR_OK;
}


EdsError EDSCALLBACK Camera::handlePropertyEvent(EdsUInt32 event, EdsUInt32 propertyID, EdsUInt32 param, EdsVoid* context) {
    if (propertyID == kEdsPropID_Evf_OutputDevice && event == kEdsPropertyEvent_PropertyChanged) {
        //        ofLogError() << "output device changed, Live View possibly ready" << std::endl;
    }
    return EDS_ERR_OK;
}

EdsError EDSCALLBACK Camera::handleStateEvent(EdsUInt32 event, EdsUInt32 param, EdsVoid* context) {
    Camera* c = (Camera*)context;
    CameraRef camera = c->shared_from_this();
    switch (event) {
        case kEdsStateEvent_WillSoonShutDown:
            if (camera->mHasOpenSession && camera->mShouldKeepAlive) {
                EdsError error = EdsSendCommand(camera->mCamera, kEdsCameraCommand_ExtendShutDownTimer, 0);
                if (error != EDS_ERR_OK) {
                    ofLogError() << "ERROR - failed to extend shut down timer" << std::endl;
                }
            }
            break;
        case kEdsStateEvent_Shutdown:
            camera->requestCloseSession();
            // send handler and browser removal notices
            if (camera->mRemovedHandler) {
                camera->mRemovedHandler(camera);
            }
            CameraBrowser::instance()->removeCamera(camera);
            break;
        default:
            break;
    }
    return EDS_ERR_OK;
}

    /*
    EdsError EDSCALLBACK Camera::handleObjectEvent(EdsObjectEvent event, EdsBaseRef object, EdsVoid* context) {
        ofLogVerbose() << "object event " << Eds::getObjectEventString(event);
        if(object) {
            
            // This event used to be received when `takePhoto` was called.
            // This is because in the past, images were saved to the camera
            // and then downloaded. Now however, `takePhoto` should save the
            // image directly to the host.
            if(event == kEdsObjectEvent_DirItemCreated) {
                ((Camera*) context)->setDownloadImage(object);
                
                // Received when the camera has taken a photo and wants to
                // transfer the data directly to the host. This is the event
                // that currently gets called after calling the `takePhoto`
                // method.
            } else if(event == kEdsObjectEvent_DirItemRequestTransfer){
                Camera* camera = (Camera*) context;
                EdsDirectoryItemRef PictureFromCamBuf = object;
                try {
                    Eds::DownloadImage(PictureFromCamBuf, camera->photoBuffer);
                }
                catch (Eds::Exception& e) {
                    ofLogError() << "Error while downloading image from camera buffer: " << e.what();
                }
                camera->photoNew = true;
                camera->needToDecodePhoto = true;
            }
            else if(event == kEdsObjectEvent_DirItemRemoved) {
                // no need to release a removed item
            } else {
                try {
                    Eds::SafeRelease(object);
                } catch (Eds::Exception& e) {
                    ofLogError() << "Error while releasing EdsBaseRef inside handleObjectEvent()";
                }
            }
        }
        return EDS_ERR_OK;
    }
    
    EdsError EDSCALLBACK Camera::handlePropertyEvent(EdsPropertyEvent event, EdsPropertyID propertyId, EdsUInt32 param, EdsVoid* context) {
        ofLogVerbose() << "property event " << Eds::getPropertyEventString(event) << ": " << Eds::getPropertyIDString(propertyId) << " / " << param;
        if(propertyId == kEdsPropID_Evf_OutputDevice) {
            ofLogNotice() << "setLiveViewReady(true)";
            ((Camera*) context)->setLiveViewReady(true);
        }
        return EDS_ERR_OK;
    }
    
    EdsError EDSCALLBACK Camera::handleCameraStateEvent(EdsStateEvent event, EdsUInt32 param, EdsVoid* context) {
        ofLogVerbose() << "camera state event " << Eds::getStateEventString(event) << ": " << param;
        if(event == kEdsStateEvent_WillSoonShutDown) {
            ((Camera*) context)->setSendKeepAlive();
        }
        return EDS_ERR_OK;
    }
    */
    
    /*
    Camera::Camera() :
    deviceId(0),
    orientationMode(0),
    bytesPerFrame(0),
    connected(false),
    liveViewReady(false),
    liveDataReady(false),
    frameNew(false),
    needToTakePhoto(false),
    photoNew(false),
    needToStartRecording(false),
    needToStopRecording(false),
    movieNew(false),
    useLiveView(false),
    needToDecodePhoto(false),
    needToUpdatePhoto(false),
    photoDataReady(false),
    needToSendKeepAlive(false),
    needToDownloadImage(false),
    resetIntervalMinutes(15) {
        livePixelsMiddle.resize(OFX_EDSDK_BUFFER_SIZE);
        for(int i = 0; i < livePixelsMiddle.maxSize(); i++) {
            livePixelsMiddle[i] = new ofPixels();
        }
        livePixelsFront = new ofPixels();
        livePixelsBack = new ofPixels();
        liveBufferBack = new ofBuffer();
    }
    */
    /*
    void Camera::setDeviceID(int deviceId) {
        this->deviceId = deviceId;
    }
    
    void Camera::setOrientationMode(int orientationMode) {
        this->orientationMode = orientationMode;
    }
    
    void Camera::setLiveView(bool useLiveView) {
        this->useLiveView = useLiveView;
        // If not using live view, we assume we're taking photos.
        // This sets up the camera to save directly to the host.
        if (!useLiveView) {
            EdsUInt32 saveTo = kEdsSaveTo_Host;
            EdsError err = EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0, sizeof(saveTo) , &saveTo);
            if (err != EDS_ERR_OK) {
                ofLogError() << "Failed to set property data: " << err;
            }
            EdsCapacity capacity;// = {0x7FFFFFFF, 0x1000, 1};
            capacity.reset = 1;
            capacity.bytesPerSector = 512*8;
            capacity.numberOfFreeClusters = 36864*9999;
            err = EdsSetCapacity(camera, capacity);
            if (err != EDS_ERR_OK) {
                ofLogError() << "Failed to set capacity: " << err;
            }
        }
    }
    
    void Camera::setup() {
        initialize();
        startCapture();
    }
    
    void Camera::start() {
        //startThread();
        
    }
    
    bool Camera::close() {
        //stopThread();
        
        // for some reason waiting for the thread keeps it from
        // completing, but sleeping then stopping capture is ok.
        ofSleepMillis(100);
        stopCapture();
    }
    
    Camera::~Camera() {
        if(connected) {
            ofLogError() << "You must call close() before destroying the camera.";
        }
        for(int i = 0; i < livePixelsMiddle.maxSize(); i++) {
            delete livePixelsMiddle[i];
        }
        delete livePixelsFront;
        delete livePixelsBack;
    }
    
    void Camera::update() {
        if(connected){
            //lock();
            if(livePixelsMiddle.size() > 0) {
                // decoding the jpeg in the main thread allows the capture thread to run in a tighter loop.
                //swap(livePixelsFront, livePixelsMiddle.front());
                livePixelsMiddle.pop();
                //unlock();
                
                livePixels = *livePixelsFront;
                
                livePixels.rotate90(orientationMode);
                if(liveTexture.getWidth() != livePixels.getWidth() ||
                   liveTexture.getHeight() != livePixels.getHeight()) {
                    liveTexture.allocate(livePixels.getWidth(), livePixels.getHeight(), GL_RGB8);
                }
                liveTexture.loadData(livePixels);
                //lock();
                liveDataReady = true;
                frameNew = true;
                //unlock();
            } else {
                //unlock();
            }
        }
    }
    
    bool Camera::isFrameNew() {
        if(frameNew) {
            frameNew = false;
            return true;
        } else {
            return false;
        }
    }
    
    bool Camera::isPhotoNew() {
        if(photoNew) {
            photoNew = false;
            return true;
        } else {
            return false;
        }
    }
    
    bool Camera::isPhotoNewAndDecoded() {
        if(photoNew && !needToDecodePhoto) {
            photoNew = false;
            return true;
        } else {
            return false;
        }
    }
    
    bool Camera::isMovieNew() {
        if(movieNew) {
            movieNew = false;
            return true;
        } else {
            return false;
        }
    }
    
    float Camera::getFrameRate() {
        float frameRate;
        frameRate = fps.getFrameRate();
        return frameRate;
    }
    
    float Camera::getBandwidth() {
        float bandwidth = bytesPerFrame * fps.getFrameRate();
        return bandwidth;
    }
    
    void Camera::takePhoto(bool blocking) {
        needToTakePhoto = true;
        if(blocking) {
            while(!photoNew) {
                ofSleepMillis(10);
            }
        }
    }
    
    void Camera::beginMovieRecording()
    {
        needToStartRecording = true;
        
    }
    
    void Camera::endMovieRecording()
    {
        needToStopRecording = true;
    }
    
    const ofPixels& Camera::getLivePixels() const {
        return livePixels;
    }
    
    const ofPixels& Camera::getPhotoPixels() const {
        if(needToDecodePhoto) {
            ofLoadImage(photoPixels, photoBuffer);
            photoPixels.rotate90(orientationMode);
            needToDecodePhoto = false;
        }
        return photoPixels;
    }
    
    unsigned int Camera::getWidth() const {
        return livePixels.getWidth();
    }
    
    unsigned int Camera::getHeight() const {
        return livePixels.getHeight();
    }
    
    void Camera::draw(float x, float y) {
        draw(x, y, getWidth(), getHeight());
    }
    
    void Camera::draw(float x, float y, float width, float height) {
        if(liveDataReady) {
            liveTexture.draw(x, y, width, height);
        }
    }
    
    const ofTexture& Camera::getLiveTexture() const {
        return liveTexture;
    }
    
    void Camera::drawPhoto(float x, float y) {
        if(photoDataReady) {
            const ofPixels& photoPixels = getPhotoPixels();
            draw(x, y, getWidth(), getHeight());
        }
    }
    
    void Camera::drawPhoto(float x, float y, float width, float height) {
        if(photoDataReady) {
            getPhotoTexture().draw(x, y, width, height);
        }
    }
    
    const ofTexture& Camera::getPhotoTexture() const {
        if(photoDataReady) {
            const ofPixels& photoPixels = getPhotoPixels();
            if(needToUpdatePhoto) {
                if(photoTexture.getWidth() != photoPixels.getWidth() ||
                   photoTexture.getHeight() != photoPixels.getHeight()) {
                    photoTexture.allocate(photoPixels.getWidth(), photoPixels.getHeight(), GL_RGB8);
                }
                photoTexture.loadData(photoPixels);
                needToUpdatePhoto = false;
            }
        }
        return photoTexture;
    }
    
    bool Camera::isLiveDataReady() const {
        return liveDataReady;
    }
    
    // this is only used by the EDSDK callback
    void Camera::setLiveViewReady(bool liveViewReady) {
        this->liveViewReady = liveViewReady;
    }
    
    void Camera::setDownloadImage(EdsDirectoryItemRef directoryItem) {
        this->directoryItem = directoryItem;
        needToDownloadImage = true;
        
    }
    
    void Camera::setSendKeepAlive() {
        
        needToSendKeepAlive = true;
    
    }
    
    bool Camera::savePhoto(string filename) {
        return ofBufferToFile(filename, photoBuffer, true);
    }
    
    void Camera::resetLiveView() {
        if(connected) {
            if (useLiveView) {
                Eds::StartLiveview(camera);
            }
            lastResetTime = ofGetElapsedTimef();
            
        }
    }
    
    void Camera::initialize() {
        try {
            
            EdsCameraListRef cameraList;
            Eds::GetCameraList(&cameraList);
            
            EdsUInt32 cameraCount;
            Eds::GetChildCount(cameraList, &cameraCount);
            
            if(cameraCount > 0) {
                EdsInt32 cameraIndex = deviceId;
                Eds::GetChildAtIndex(cameraList, cameraIndex, &camera);
                //Eds::SetObjectEventHandler(camera, kEdsObjectEvent_All, handleObjectEvent, this);
                Eds::SetPropertyEventHandler(camera, kEdsPropertyEvent_All, handlePropertyEvent, this);
                Eds::SetCameraStateEventHandler(camera, kEdsStateEvent_All, handleCameraStateEvent, this);
                
                EdsDeviceInfo info;
                Eds::GetDeviceInfo(camera, &info);
                Eds::SafeRelease(cameraList);
                ofLogNotice("ofxEdsdk::setup") << "connected camera model: " <<  info.szDeviceDescription << " " << info.szPortName;
            } else {
                ofLogError() << "No cameras are connected for ofxEds::Camera::setup().";
            }
        } catch (Eds::Exception& e) {
            ofLogError() << "There was an error during Camera::setup(): " << e.what();
        }
    }
    
    void Camera::startCapture() {
        try {
            Eds::OpenSession(camera);
            connected = true;
            if(useLiveView) {
                Eds::StartLiveview(camera);
            }
        } catch (Eds::Exception& e) {
            ofLogError() << "There was an error opening the camera, or starting live view: " << e.what();
            return;
        }
        lastResetTime = ofGetElapsedTimef();
    }
    
    void Camera::stopCapture() {
        if(connected) {
            try {
                if(liveViewReady) {
                    Eds::EndLiveview(camera);
                    liveViewReady = false;
                }
                Eds::CloseSession(camera);
                connected = false;
                //Eds:EdsTerminateSDK();
            } catch (Eds::Exception& e) {
                ofLogError() << "There was an error closing ofxEds::Camera: " << e.what();
            }
        }
    }
    
    
    void Camera::captureLoop() {
        if(liveViewReady && useLiveView) {
            if(Eds::DownloadEvfData(camera, *liveBufferBack)) {
                ofLoadImage(*livePixelsBack, *liveBufferBack);
                
                fps.tick();
                bytesPerFrame = ofLerp(bytesPerFrame, livePixelsBack->size(), .01);
                
                swap(livePixelsBack, livePixelsMiddle.back());
                livePixelsMiddle.push();
                
            }
        }
        
        if(needToTakePhoto) {
            try {
                Eds::SendCommand(camera, kEdsCameraCommand_TakePicture, 0);
                needToTakePhoto = false;
                
            } catch (Eds::Exception& e) {
                ofLogError() << "Error while taking a picture: " << e.what();
            }
        }
        
        if(needToStartRecording) {
            try {
                EdsUInt32 saveTo = kEdsSaveTo_Camera;
                EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0, sizeof(saveTo) , &saveTo);
                
                EdsUInt32 record_start = 4; // Begin movie shooting
                EdsSetPropertyData(camera, kEdsPropID_Record, 0, sizeof(record_start), &record_start);
                needToStartRecording = false;
                
            } catch (Eds::Exception& e) {
                ofLogError() << "Error while beginning to record: " << e.what();
            }
        }
        
        if(needToStopRecording) {
            try {
                EdsUInt32 record_stop = 0; // End movie shooting
                EdsSetPropertyData(camera, kEdsPropID_Record, 0, sizeof(record_stop), &record_stop);
                
                needToStopRecording = false;
                
            } catch (Eds::Exception& e) {
                ofLogError() << "Error while stopping to record: " << e.what();
            }
        }
        
        if(needToSendKeepAlive) {
            try {
                // always causes EDS_ERR_DEVICE_BUSY, even with live view disabled or a delay
                // but if it's not here, then the camera shuts down after 5 minutes.
                Eds::SendStatusCommand(camera, kEdsCameraCommand_ExtendShutDownTimer, 0);
            } catch (Eds::Exception& e) {
                ofLogError() << "Error while sending kEdsCameraCommand_ExtendShutDownTimer with Eds::SendStatusCommand: " << e.what();
            }
            
            needToSendKeepAlive = false;
            
        }
        
        if(needToDownloadImage) {
            try {
                EdsDirectoryItemInfo dirItemInfo = Eds::DownloadImage(directoryItem, photoBuffer);
                ofLogVerbose() << "Downloaded item: " << (int) (photoBuffer.size() / 1024) << " KB";
                
                photoDataReady = true;
                needToDecodePhoto = true;
                needToUpdatePhoto = true;
                needToDownloadImage = false;
                
                if (dirItemInfo.format == OFX_EDSDK_JPG_FORMAT) {
                    photoNew = true;
                } else if (dirItemInfo.format == OFX_EDSDK_MOV_FORMAT) {
                    movieNew = true;
                }
                
                
            } catch (Eds::Exception& e) {
                ofLogError() << "Error while downloading item: " << e.what();
            }
        }
        
        if(needToDecodePhoto) {
            ofLogVerbose() << "Decoding photo";
            ofLoadImage(photoPixels, photoBuffer);
            photoPixels.rotate90(orientationMode);
            needToDecodePhoto = false;
            
        }
        
        float timeSinceLastReset = ofGetElapsedTimef() - lastResetTime;
        if(timeSinceLastReset > resetIntervalMinutes * 60) {
            resetLiveView();
        }
        
    }
    
    void Camera::threadedFunction() {
#if defined(TARGET_WIN32)
        CoInitializeEx(NULL, 0x0); // COINIT_APARTMENTTHREADED in SDK docs
#endif
        //while(isThreadRunning()) {
            captureLoop();
            ofSleepMillis(16);
        //}
#if defined(TARGET_WIN32)
        CoUninitialize();
#endif
    }
     
     */
}
