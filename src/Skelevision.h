#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxXmlSettings.h"
#include "ofxZip.h"

#include "SkeletonOptimizer.h" // for PolylinePlus struct.

class Skelevision {

	public:
	
	void	initialize();
	void	loadXMLRecording (string &xmlFilename, bool bFileIsZipped);
	void	transferFromXmlToCurrentDrawing();
	
	void	transferCurrentRecordingToXML();
	void	saveXMLRecording (string &xmlFilename, bool bSaveAsZipped);
	
	void	generateBogusFrames();
	void	drawCurrentPlaybackFrame();
	
	ofxXmlSettings *readXML;
	ofxXmlSettings *saveXML;
	
	vector<vector<PolylinePlus>> currentPlaybackFrames;  // the currently re-playing drawing
	vector<vector<PolylinePlus>> currentRecordingFrames; // the drawing currently being recorded
	
	int		currentPlaybackFrameIndex;
	bool	bLoadedFileFromXML;
	int		outputFileCounter;

};