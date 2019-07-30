//
//  CameraBrowser.cpp
//  ToysPhotoBoothOFApp
//
//  Created by mg on 21/05/17.
//
//


#include "CameraBrowser.h"



namespace ofxEdsdkCiPort {
    
    
    CameraBrowserRef CameraBrowser::sInstance = 0;
    
    CameraBrowserRef CameraBrowser::instance() {
        if (!sInstance) {
            sInstance = CameraBrowserRef(new CameraBrowser())->shared_from_this();
        }
        return sInstance;
    }
    
    CameraBrowser::CameraBrowser() : mIsBrowsing(false) {
                        
        EdsError error = EdsInitializeSDK();
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to initialize SDK" << endl;
            //throw Exception();
        }
    }
    
    CameraBrowser::~CameraBrowser() {
        mAddedHandler = NULL;
        mRemovedHandler = NULL;
        mEnumeratedHandler = NULL;
        
        mCameras.clear();
        
        
        
        /*
        EdsError error = EdsTerminateSDK();
         if (error != EDS_ERR_OK) {
         cout << "ERROR - failed to terminate SDK cleanly" << endl;
         //throw Exception();
         }
        */
    }

    #pragma mark -
    
        
    void CameraBrowser::connectAddedHandler(const std::function<void(CameraRef)>& handler) {
        mAddedHandler = handler;
    }
    
    void CameraBrowser::connectRemovedHandler(const std::function<void(CameraRef)>& handler) {
        mRemovedHandler = handler;
    }
    
    void CameraBrowser::connectEnumeratedHandler(const std::function<void(void)>& handler) {
        mEnumeratedHandler = handler;
    }
    
    //bool CameraBrowser::isBrowsing() const {
    //    return mIsBrowsing;
    //}
    
    void CameraBrowser::start() {
        if (mIsBrowsing) {
            return;
        }
        
        mIsBrowsing = true;
        
        EdsError error = EdsSetCameraAddedHandler(CameraBrowser::handleCameraAdded, this);
        if (error != EDS_ERR_OK) {
            cout << "ERROR - failed to set camera added handler" << std::endl;
        }
        
        enumerateCameraList();
        if (mEnumeratedHandler) {
            mEnumeratedHandler();
        }
    }
    
    
#pragma mark - PRIVATE
    
    void CameraBrowser::enumerateCameraList() {
        EdsCameraListRef cameraList = NULL;
        EdsError error = EdsGetCameraList(&cameraList);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get camera list" << std::endl;
            EdsRelease(cameraList);
            return;
        }
        
        EdsUInt32 cameraCount = 0;
        error = EdsGetChildCount(cameraList, &cameraCount);
        if (error != EDS_ERR_OK) {
            ofLogError() << "ERROR - failed to get camera count" << std::endl;
            EdsRelease(cameraList);
            return;
        }
        
        for (uint32_t idx = 0; idx < cameraCount; idx++) {
            EdsCameraRef cam = NULL;
            error = EdsGetChildAtIndex(cameraList, idx, &cam);
            if (error != EDS_ERR_OK) {
                ofLogError() << "ERROR - failed to get camera: " << idx << std::endl;
                continue;
            }
            
            CameraRef camera = nullptr;
            try {
                camera = Camera::create(cam);
            } catch (...) {
                EdsRelease(cam);
                continue;
            }
            EdsRelease(cam);
            
            // add if previously unknown
            if (std::none_of(mCameras.begin(), mCameras.end(), [camera](CameraRef c) { return c->getPortName().compare(camera->getPortName()) == 0; })) {
                mCameras.push_back(camera);
                if (mAddedHandler) {
                    mAddedHandler(camera);
                }
            }
        }
        
        EdsRelease(cameraList);
    }
    
    void CameraBrowser::removeCamera(const CameraRef& camera) {
        auto it = std::find_if(mCameras.begin(), mCameras.end(), [camera](CameraRef c) { return c->getPortName().compare(camera->getPortName()) == 0; });
        if (it == mCameras.end()) {
            ofLogError() << "ERROR - failed to find removed camera:" << camera->getName() << " in camera browser's list" << std::endl;
            return;
        }
        
        CameraRef c = mCameras[it - mCameras.begin()];
        mCameras.erase(it);
        if (mRemovedHandler) {
            mRemovedHandler(camera);
        }
    }
    
    CameraRef CameraBrowser::cameraForPortName(const std::string& name) const {
        CameraRef camera = nullptr;
        auto it = std::find_if(mCameras.begin(), mCameras.end(), [name](CameraRef c){ return c->getPortName().compare(name) == 0; });
        if (it != mCameras.end()) {
            camera = mCameras[it - mCameras.begin()];
        }
        return camera;
    }
    
#pragma mark - CALLBACKS
    
    EdsError EDSCALLBACK CameraBrowser::handleCameraAdded(EdsVoid* context) {
        // we are left to our own devices to determine which camera was added
        ((CameraBrowser*)context)->enumerateCameraList();
        return EDS_ERR_OK;
    }

}
