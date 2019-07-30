#pragma once

#include "ofMain.h"

#include "ofxEdsdkCiPort.h"


using namespace ofxEdsdkCiPort;

class ofApp : public ofBaseApp {
public:
	void setup();
    void exit();
	void update();
	void draw();
	void keyPressed(ofKeyEventArgs& key);
    
    
    void browserDidAddCamera(CameraRef camera);
    void browserDidRemoveCamera(CameraRef camera);
    void browserDidEnumerateCameras();
    void didRemoveCamera(CameraRef camera);
    void didAddFile(CameraRef camera, CameraFileRef file);
    
    
    vector<CameraRef> cams;
    
    
    
    ofTexture liveTexture;
    ofImage shotImage;
    
    float videoScale;
    ofPoint videoPos;
    
    float imageScale;
    ofPoint imagePos;
};
