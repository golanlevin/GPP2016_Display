#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxXmlSettings.h"

class Skelevision {

	public:
	
	void initialize(); 
	void testBuffer();
	void generateBogusXMLData();
	void drawCurrentPlaybackFrame();
	
	ofxXmlSettings *XML;
	vector<vector<ofPolyline>> currentPlaybackFrames;
	int currentPlaybackFrameIndex;

};