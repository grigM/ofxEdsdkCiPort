#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
	ofSetVerticalSync(true);
    ofSetDataPathRoot("../Resources/data/");
    
    CameraBrowser::instance()->connectAddedHandler(&ofApp::browserDidAddCamera, this);
    CameraBrowser::instance()->connectRemovedHandler(&ofApp::browserDidRemoveCamera, this);
    CameraBrowser::instance()->connectEnumeratedHandler(&ofApp::browserDidEnumerateCameras, this);
    CameraBrowser::instance()->start();
    
}

void ofApp::exit() {
    
}

void ofApp::update() {
    
    if(cams.size()>0){
        if(cams[0]!=NULL){
            if (cams[0] && cams[0]->hasOpenSession() && cams[0]->isLiveViewActive()) {
                cams[0]->requestLiveViewImage([&](EdsError error, ofPixels livePixels) {
                    if (error == EDS_ERR_OK) {
                        
                        if(liveTexture.getWidth() != livePixels.getWidth() ||
                           liveTexture.getHeight() != livePixels.getHeight()) {
                            liveTexture.allocate(livePixels.getWidth(), livePixels.getHeight(), GL_RGB8);
                            
                        }
                        liveTexture.loadData(livePixels);
                        
                        
                        videoScale = ofGetHeight()/liveTexture.getHeight();
                        
                        videoPos.x = (ofGetWidth()/2)-((liveTexture.getWidth()*videoScale)/2);
                        videoPos.y = (ofGetHeight()/2)-((liveTexture.getHeight()*videoScale)/2);
                        
                        
                    }
                });
            }
        }
        
        if(shotImage.isAllocated()){
            imageScale =  ofGetHeight()/shotImage.getHeight();
            imagePos.x = (ofGetWidth()/2)-((shotImage.getWidth()*imageScale)/2);
            imagePos.y = (ofGetHeight()/2)-((shotImage.getHeight()*imageScale)/2);
        }
    }
}

void ofApp::draw() {
    
    if(cams.size()>0){
        if(cams[0]!=NULL){
            if(liveTexture.isAllocated()){
                liveTexture.draw(videoPos.x,videoPos.y, liveTexture.getWidth()*videoScale, liveTexture.getHeight()*videoScale);
                
            }
        }
    }
    
    if(shotImage.isAllocated()){
        ofSetColor(255, 255, 255, 100);
        shotImage.draw(imagePos.x, imagePos.y, shotImage.getWidth()*imageScale, shotImage.getHeight()*imageScale);
        
    }
    
}



//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs& key){
    cout << "key:" << key.key << endl;
    
    
    if(key.key == ' ') {
        for(int i = 0; i < cams.size(); i++){
            if(cams[i] && cams[i]->hasOpenSession()){
                //cams[i]->requestTakePicture();
                cams[i]->requestTakePictureNonAF();
            }
        }
        
        //ofSleepMillis(100);
        
        for(int i = 0; i < cams.size(); i++){
            if(cams[i] && cams[i]->hasOpenSession()){
                cams[i]->requestHalfWayshutterReleaseButton();
            }
        }
    }
    
    if(key.key == 'r'){
        for(int i = 0; i < cams.size(); i++){
            if(cams[i] && cams[i]->hasOpenSession()){
                cams[i]->requestHalfWayshutterPressButton();
            }
            /*
            if(cams[i] && cams[i]->hasOpenSession()){
                cams[i]->requestHalfWayshutterReleaseButton();
            }
            */
        }
    }
    
    if(key.key == 't'){
        for(int i = 0; i < cams.size(); i++){
            if(cams[i] && cams[i]->hasOpenSession()){
                cams[i]->requestHalfWayshutterReleaseButton();
            }
        }
    }
    
    if(key.key == 'l'){
        
        if (cams[0] && cams[0]->hasOpenSession()) {
            //mCamera->toggleLiveView();
            cams[0]->startLiveView();
        }
        
    }
    
    if(key.key == 's') {
        if (cams[0] && cams[0]->hasOpenSession()) {
            //mCamera->toggleLiveView();
            cams[0]->stopLiveView();
        }
    }
    
    if(key.key == OF_KEY_LEFT){
        cams[0]->setShutterSpeed(25);
        //cams[0]->setIsoSpeed(Camera::iso_400);
    }
    
    if(key.key == OF_KEY_RIGHT){
        cams[0]->setShutterSpeed(1.0 / 640.0);
        //cams[0]->setIsoSpeed(Camera::iso_800);
        
    }
    
    if(key.key == OF_KEY_DOWN){
        //cams[0]->setImageQuality(Camera::quality_S1);
        //cams[0]->setWhiteBalance(Camera::wb_Daylight);
        //cams[0]->setAperture(4.5);
        cams[0]->setAEMode(ofxEdsdkCiPort::AEmodes::ae_mode_AV);
    }
    
    
    if(key.key == OF_KEY_UP){
        //cams[0]->setImageQuality(Camera::quality_S2);
        //cams[0]->setWhiteBalance(Camera::wb_Shade);
        //cams[0]->setAperture(7.1);
        cams[0]->setAEMode(ofxEdsdkCiPort::AEmodes::ae_mode_Manual);
    }
    
    ofLog() << ofxEdsdkCiPort::AEmodes().all[1] ;
}



//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------


#pragma mark - CAMERA BROWSER

void ofApp::browserDidAddCamera(CameraRef camera) {
    
    ofLog() << "added a camera: " << camera->getName() << std::endl;
    camera->connectRemovedHandler(&ofApp::didRemoveCamera, this);
    camera->connectFileAddedHandler(&ofApp::didAddFile, this);
    ofLog() << "CanonTestApp " << camera->getName() << " portname:" << camera->getPortName() << std::endl;
    
    ofxEdsdkCiPort::Camera::Settings settings = ofxEdsdkCiPort::Camera::Settings();
    // settings.setPictureSaveLocation(kEdsSaveTo_Host);
    // settings.setShouldKeepAlive(false);
    EdsError error = camera->requestOpenSession(settings);
    if (error == EDS_ERR_OK) {
        ofLog() << "session opened" << std::endl;
        string camSerial = camera->requestCameraSerial();
        ofLog() << "camSerial:" << camSerial << endl;
        
        cams.push_back(camera);
        
    }
    
}


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------


void ofApp::didAddFile(CameraRef camera, CameraFileRef file) {
    
    
    string fileName = ofGetTimestampString();
    string saveImageFolder = "photos/";
    
    camera->requestDownloadFile(file, saveImageFolder, [&](EdsError error, string outputFilePath) {
        if (error == EDS_ERR_OK) {
            ofLog() << "image downloaded to '" << outputFilePath << "'" << std::endl;
            
            string loadFilePath = fileName+".jpg";
            shotImage.load(loadFilePath);
                
            
            
        }
        
    },  fileName);
    
}





//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------


void ofApp::browserDidRemoveCamera(CameraRef camera) {
    // NB - somewhat redundant as the camera will notify handler first
    ofLog() << "removed a camera: " << camera->getName() << std::endl;
    
    ofLog() << "our camera was disconnected" << std::endl;
    
}


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------



void ofApp::browserDidEnumerateCameras() {
    ofLog() << "enumerated cameras" << std::endl;
}


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

#pragma mark - CAMERA WAS REMOVED

void ofApp::didRemoveCamera(CameraRef camera) {
    ofLog() << "removed a camera: " << camera->getName() << std::endl;
    ofLog() << "our camera was disconnected" << std::endl;
    
}




