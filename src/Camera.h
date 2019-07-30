//
//  Camera.hpp
//  ToysPhotoBoothOFApp
//
//  Created by mg on 21/05/17.
//
//

#pragma once


#include "ofMain.h"
#include "Utils.h"

#if defined(TARGET_OSX)
#define __MACOS__
#endif

#include "EDSDK.h"
//#include "EdsWrapper.h"
//#include "RateTimer.h"
//#include "FixedQueue.h"



#define EDS_DRIVE_MODE_SINGLE_FRAME 0x00000000L
#define EDS_DRIVE_MODE_CONTINUOUS_SHOOTING 0x00000001L
#define EDS_DRIVE_MODE_VIDEO 0x00000002L
#define EDS_DRIVE_MODE_HIGH_SPEED_CONTINUOUS_SHOOTING 0x00000004L
#define EDS_DRIVE_MODE_LOW_SPEED_CONTINUOUS_SHOOTING 0x00000005L
#define EDS_DRIVE_MODE_SILENT_SINGLE_SHOOTING 0x00000006L
#define EDS_DRIVE_MODE_10SEC_SELF_TIMER_AND_CONTINUOUS_SHOOTING 0x00000007L
#define EDS_DRIVE_MODE_10SEC_SELF_TIMER 0x00000010L
#define EDS_DRIVE_MODE_2SEC_SELF_TIMER 0x00000011L



namespace ofxEdsdkCiPort {
    
typedef shared_ptr<class Camera> CameraRef;
    
typedef std::shared_ptr<class CameraFile> CameraFileRef;

class CameraFile : public std::enable_shared_from_this<CameraFile> {
public:
    static CameraFileRef create(const EdsDirectoryItemRef& directoryItem);
    ~CameraFile();
    
    inline std::string getName() const {
        return std::string(mDirectoryItemInfo.szFileName);
    }
    inline uint32_t getSize() const {
        return mDirectoryItemInfo.size;
    }
    
    
private:
    CameraFile(const EdsDirectoryItemRef& directoryItem);
    
    EdsDirectoryItemRef mDirectoryItem;
    EdsDirectoryItemInfo mDirectoryItemInfo;
    
    friend class Camera;
};
    

    
    
    
        
    
    
    
    
//class Camera : public ofThread {
class Camera : public enable_shared_from_this<Camera> {
    public:
    
    
    
    
    
    
    
    
    struct Settings {
        Settings() : mShouldKeepAlive(true), mPictureSaveLocation(kEdsSaveTo_Host) {}
        
        Settings& setShouldKeepAlive(bool flag) {
            mShouldKeepAlive = flag; return *this;
        }
        inline bool getShouldKeepAlive() const {
            return mShouldKeepAlive;
        }
        Settings& setPictureSaveLocation(EdsUInt32 location)  {
            mPictureSaveLocation = location; return *this;
        }
        inline EdsUInt32 getPictureSaveLocation() const {
            return mPictureSaveLocation;
        }
        
                
        
    private:
        bool mShouldKeepAlive;
        EdsUInt32 mPictureSaveLocation;
    };
    
    public:
    static CameraRef create(const EdsCameraRef& camera);
    ~Camera();
    
    template<typename T, typename Y>
    inline void connectRemovedHandler(T handler, Y* obj) { connectRemovedHandler(std::bind(handler, obj, std::placeholders::_1)); }
    void connectRemovedHandler(const std::function<void(CameraRef)>& handler);
    template<typename T, typename Y>
    inline void connectFileAddedHandler(T handler, Y* obj) { connectFileAddedHandler(std::bind(handler, obj, std::placeholders::_1, std::placeholders::_2)); }
    void connectFileAddedHandler(const std::function<void(CameraRef, CameraFileRef)>& handler);
    
    std::string requestCameraSerial();
    inline std::string getName() const {
        return std::string(mDeviceInfo.szDeviceDescription);
    }
    inline std::string getPortName() const {
        return std::string(mDeviceInfo.szPortName);
    }
    
    inline bool hasOpenSession() const {
        return mHasOpenSession;
    }
    EdsError requestOpenSession(const Settings& settings = Settings());
    EdsError requestCloseSession();
    
    EdsError requestTakePicture();
    EdsError requestTakePictureNonAF();
    EdsError requestHalfWayshutterPressButton();
    EdsError requestHalfWayshutterReleaseButton();
    
    
    EdsError requestStartFileRec();
    EdsError requestStopFileRec();
    
    bool needToTakeContinuousPhotos;
    bool needToStopContinuousPhotos;
    
    std::string continuousPhotosFileNamePrefix;
    int continuousPhotosCounter;
    
    bool continuousPhotosDone;
    int continuousPhotosNeedNumber;
    EdsError requestTakeContinuousPicture(int count);
    EdsError requestStopContinuousPicture();
    
    void requestDownloadFile(const CameraFileRef& file, const string destinationFolderPath, const std::function<void(EdsError error, string outputFilePath)>& callback, std::string fileName);
    //void requestReadFile(const CameraFileRef& file, const std::function<void(EdsError error, ci::SurfaceRef surface)>& callback);
    
    inline bool isLiveViewActive() const {
        return mIsLiveViewActive;
    };
    EdsError requestStartLiveView();
    EdsError requestStopLiveView();
    
    void startLiveView();
    void stopLiveView();
    
    void toggleLiveView();
    void requestLiveViewImage(const std::function<void(EdsError error, ofPixels livePixels)>& callback);
    
    EdsError setIsoSpeed(EdsUInt32 value);
    EdsError setImageQuality(EdsUInt32 value);
    EdsError setWhiteBalance(EdsUInt32 value);
    
    EdsError setAEMode(EdsUInt32 value);
    
    EdsError setAperture(float aperture);
    
    EdsError setShutterSpeed(EdsUInt32 value);
    
private:
    Camera(const EdsCameraRef& camera);
    
    static EdsError EDSCALLBACK handleObjectEvent(EdsUInt32 event, EdsBaseRef ref, EdsVoid* context);
    static EdsError EDSCALLBACK handlePropertyEvent(EdsUInt32 event, EdsUInt32 propertyID, EdsUInt32 param, EdsVoid* context);
    static EdsError EDSCALLBACK handleStateEvent(EdsUInt32 event, EdsUInt32 param, EdsVoid* context);
    
    std::function<void (CameraRef)> mRemovedHandler;
    std::function<void (CameraRef, CameraFileRef)> mFileAddedHandler;
    EdsCameraRef mCamera;
    EdsDeviceInfo mDeviceInfo;
    bool mHasOpenSession;
    bool mShouldKeepAlive;
    bool mIsLiveViewActive;
    
    
    
    ofBuffer* liveBufferBack;
    ofPixels* livePixelsBack;
    //FixedQueue<ofPixels*> livePixelsMiddle;
    
//    ofPixels* livePixelsFront;
    mutable ofPixels livePixels;
    mutable ofTexture liveTexture;
    
    
    
};
    
    
    
class AEmodes
{
public:
    enum Constants {
        ae_mode_Manual = kEdsAEMode_Manual,
        ae_mode_AV = kEdsAEMode_Av,
        ae_mode_TV = kEdsAEMode_Tv,
        ae_mode_P = kEdsAEMode_Program,
        ae_mode_CreativeAuto = kEdsAEMode_CreativeAuto,
        ae_mode_FlashOff = kEdsAEMode_FlashOff
        
    };
    vector<string> all_str = { "Manual", "AV", "TV", "P", "Creative_Auto", "FlashOff_Auto"};
    vector<Constants> all = { ae_mode_Manual, ae_mode_AV, ae_mode_TV, ae_mode_P, ae_mode_CreativeAuto, ae_mode_FlashOff};
};

    
class ISOSpeeds
{
public:
    enum Constants {
        iso_auto = 0x00000000,
        iso_50 = 0x00000040,
        iso_100 = 0x00000048,
        iso_125 = 0x0000004b,
        iso_160 = 0x0000004d,
        iso_200 = 0x00000050,
        iso_250 = 0x00000053,
        iso_320 = 0x00000055,
        iso_400 = 0x00000058,
        iso_500 = 0x0000005b,
        iso_640 = 0x0000005d,
        iso_800 = 0x00000060,
        iso_1000 = 0x00000063,
        iso_1250 = 0x00000065,
        iso_1600 = 0x00000068,
        iso_2000 = 0x0000006b,
        iso_2500 = 0x0000006d,
        iso_3200 = 0x00000070,
        iso_4000 = 0x00000073,
        iso_5000 = 0x00000075,
        iso_6400 = 0x00000078,
        iso_8000 = 0x0000007b,
        iso_10000 = 0x0000007d,
        iso_12800 = 0x00000080,
        iso_25600 = 0x00000088,
        iso_51200 = 0x00000090,
        iso_102400 = 0x00000098,
        count = 25
    };
    
    
    vector<string> all_str = { "auto", "100", "200", "400", "800", "1600", "3200", "6400"};
    vector<Constants> all = { iso_auto, iso_100, iso_200, iso_400, iso_800, iso_1600, iso_3200, iso_6400};
    
    
};

class ImageQualities
{
public:
    enum Constants {
        quality_S3 = EdsImageQuality_S3JF, // 720x400
        quality_S2 = EdsImageQuality_S2JF, // 1920x1080
        quality_S1N = EdsImageQuality_S1JN, // Normal - 2880x1624
        quality_S1F = EdsImageQuality_S1JF, // Fine - 2880x1624
        quality_MN = EdsImageQuality_MJN,  // Middle Normal - 3840x2160
        quality_MF = EdsImageQuality_MJF,  // Middle Fine - 3840x2160
        quality_LN = EdsImageQuality_LJN,  // Large Normal - 5760x3240
        quality_LF = EdsImageQuality_LJF   // Large Fine - 5760x3240
        
    };
    
    vector<string> all_str = { "S3 - 720x400", "S2 - 1920x1080", "S1 Normal - 2880x1624", "S1 Fine - 2880x1624", "MN - 3840x2160", "MF - 3840x2160", "LN - 5760x3240", "LF - 5760x3240"};
    vector<Constants> all = { quality_S3, quality_S2, quality_S1N, quality_S1F, quality_MN, quality_MF, quality_LN, quality_LF};
    
};
    
class WhiteBalanceModes
{
public:
    enum Constants {
        wb_Auto = kEdsWhiteBalance_Auto,
        wb_Daylight = kEdsWhiteBalance_Daylight,
        wb_Cloudy = kEdsWhiteBalance_Cloudy,
        wb_Tangsten = kEdsWhiteBalance_Tangsten,
        wb_Fluorescent = kEdsWhiteBalance_Fluorescent,
        wb_Strobe = kEdsWhiteBalance_Strobe,
        wb_WhitePaper = kEdsWhiteBalance_WhitePaper,
        wb_Shade = kEdsWhiteBalance_Shade,
        wb_ColorTemp = kEdsWhiteBalance_ColorTemp,
        wb_PCSet1 = kEdsWhiteBalance_PCSet1,
        wb_PCSet2 = kEdsWhiteBalance_PCSet2,
        wb_PCSet3 = kEdsWhiteBalance_PCSet3,
        wb_WhitePaper2 = kEdsWhiteBalance_WhitePaper2,
        wb_WhitePaper3 = kEdsWhiteBalance_WhitePaper3,
        wb_WhitePaper4 = kEdsWhiteBalance_WhitePaper4,
        wb_WhitePaper5 = kEdsWhiteBalance_WhitePaper5,
        wb_PCSet4 = kEdsWhiteBalance_PCSet4,
        wb_PCSet5 = kEdsWhiteBalance_PCSet5,
        wb_AwbWhite = kEdsWhiteBalance_AwbWhite,
        wb_Click = kEdsWhiteBalance_Click,
        wb_Pasted = kEdsWhiteBalance_Pasted
    };
    
    vector<string> all_str = { "Auto", "Daylight 5200k", "Cloudy 6000k", "Tangsten 3200k", "Fluorescent 4000k", "Shade 7000k", "Strobe" };
    vector<Constants> all = { wb_Auto, wb_Daylight, wb_Cloudy, wb_Tangsten, wb_Fluorescent, wb_Shade, wb_Strobe};
    
    
};
    
class ApertureModes
{
public:
    vector<string> all_str = { "3.5","4.0","4.5", "5.6", "6.3", "_6.7", "7.1", "8.0", "9.0", "_9.5", "10.0", "11.0", "13.0", "14.0", "16.0", "18.0", "19.0",  "20.0",  "22.0", "25.0", "_27.0", "29.0", "32.0", "36.0"};
};
    
    
class ShutterSpeeds
{
public:
    
    enum Constants {
        
        
        sh_5000 = 0x9B ,
        sh_4000 = 0x98 ,
        sh_3200 = 0x95 ,
        sh_3000 = 0x94 ,
        sh_2500 = 0x90 ,
        sh_2000 = 0x90 ,
        sh_1600 = 0x8D ,
        sh_1500 = 0x8C ,
        sh_1250 = 0x8B ,
        sh_1000 = 0x88 ,
        sh_800 = 0x85 ,
        sh_750 = 0x84 ,
        sh_640 = 0x83 ,
        sh_500 = 0x80,
        sh_400 = 0x7D ,
        sh_350 = 0x7C,
        sh_320 = 0x7B,
        sh_250 = 0x78,
        sh_200 = 0x75,
        sh_180 = 0x74,
        sh_160 = 0x73,
        sh_125 = 0x70,
        sh_100 = 0x6D,
        sh_90 = 0x6C,
        sh_80 = 0x6B,
        sh_60 = 0x68,
        sh_50 = 0x65,
        sh_45 = 0x64,
        sh_40 = 0x63,
        sh_30 = 0x60,
        sh_25 = 0x5D,
        sh_20_ = 0x5C,
        sh_20 = 0x5B,
        sh_15 = 0x58,
        sh_10_ = 0x54,
        sh_10 = 0x53 ,
        sh_8 = 0x50 ,
        sh_6_ = 0x4D ,
        sh_6 = 0x4C ,
        sh_4 = 0x48 ,
        sh_0_3_s = 0x45 ,
        sh_0_3s = 0x44 ,
        sh_0_5s = 0x40 ,
        sh_0_6s = 0x3D ,
        sh_0_7s = 0x3C ,
        sh_1s = 0x38 ,
        sh_1_5s = 0x34 ,
        sh_2s = 0x30 ,
        sh_3s = 0x2C ,
        sh_4s = 0x28 ,
        sh_6_s = 0x24 ,
        sh_6s = 0x23 ,
        sh_8s = 0x20 ,
        sh_10_s = 0x1D ,
        sh_10s = 0x1C ,
        sh_15s = 0x18 ,
        sh_20_s = 0x15 ,
        sh_20s = 0x14 ,
        sh_30s = 0x10 ,
        sh_Bulb = 0x0c
        
    };
    

    vector<Constants> all = {sh_5000,
        sh_4000,
        sh_3200,
        sh_3000,
        sh_2500,
        sh_2000,
        sh_1600,
        sh_1500,
        sh_1250,
        sh_1000,
        sh_800,
        sh_750,
        sh_640,
        sh_500,
        sh_400,
        sh_350,
        sh_320,
        sh_250,
        sh_200,
        sh_180,
        sh_160,
        sh_125,
        sh_100,
        sh_90,
        sh_80,
        sh_60,
        sh_50,
        sh_45,
        sh_40,
        sh_30,
        sh_25,
        sh_20_,
        sh_20,
        sh_15,
        sh_10_,
        sh_10,
        sh_8,
        sh_6_,
        sh_6 ,
        sh_4,
        sh_0_3_s,
        sh_0_3s,
        sh_0_5s,
        sh_0_6s,
        sh_0_7s,
        sh_1s,
        sh_1_5s,
        sh_2s,
        sh_3s,
        sh_4s,
        sh_6_s,
        sh_6s,
        sh_8s,
        sh_10_s,
        sh_10s,
        sh_15s,
        sh_20_s,
        sh_20s,
        sh_30s,
        sh_Bulb};
    
    
    
    vector<string> all_str={ "5000","4000","3200","3000","2500","2000","1600","1500","1250","1000","800","750","640","500","400","350","320","250","200","180","160","125","100","90","80","60","50","45","40","30","25","20_","20","15","10_","10","8","6_","6","4","0_3_s","0_3s","0_5s","0_6s","0_7s","1s","1_5s","2s","3s","4s","6_s","6s","8s","10_s","10s","15s","20_s","20s","30s","Bulb"};
    
};
    
    
};
