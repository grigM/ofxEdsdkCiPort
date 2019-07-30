#pragma once
//
//  CameraManager.h
//  example
//
//  Created by mg on 16/05/17.
//
//

#include "Camera.h"



namespace ofxEdsdkCiPort {
    
    
    typedef shared_ptr<class CameraBrowser> CameraBrowserRef;
    
    
    
    class CamsObjectEventArgs : public ofEventArgs {
    public:
        int next_state;
        string data_from_state;
    };
    
    
    class CameraBrowser : public enable_shared_from_this<CameraBrowser>, private boost::noncopyable {
        
        
    public:
    
        static CameraBrowserRef instance();
        ~CameraBrowser();
        
        template<typename T, typename Y>
        inline void connectAddedHandler(T handler, Y* obj) { connectAddedHandler(bind(handler, obj, placeholders::_1)); }
        void connectAddedHandler(const function<void(CameraRef)>& handler);
        
        
        template<typename T, typename Y>
        inline void connectRemovedHandler(T handler, Y* obj) { connectRemovedHandler(bind(handler, obj, placeholders::_1)); }
        void connectRemovedHandler(const function<void(CameraRef)>& handler);
        
        template<typename T, typename Y>
        inline void connectEnumeratedHandler(T handler, Y* obj) { connectEnumeratedHandler(bind(handler, obj)); }
        void connectEnumeratedHandler(const function<void(void)>& handler);

        
        
        void start();
        //    void stop();
        
        inline std::vector<CameraRef>& getCameras() {
            return mCameras;
        }
        
    private:
        
        CameraBrowser();
        void enumerateCameraList();
        void removeCamera(const CameraRef& camera);
        CameraRef cameraForPortName(const string& name) const;
        
        static EdsError EDSCALLBACK handleCameraAdded(EdsVoid* context);
        
        static CameraBrowserRef sInstance;
        function<void (CameraRef)> mAddedHandler;
        function<void (CameraRef)> mRemovedHandler;
        function<void (void)> mEnumeratedHandler;
        vector<CameraRef> mCameras;
        bool mIsBrowsing;
        
        friend class Camera;
    
    };


}
