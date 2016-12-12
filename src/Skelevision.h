#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxXmlSettings.h"
#include "ofxZip.h"
#include "ofThread.h"

#include "SkeletonOptimizer.h" // Included for PolylinePlus struct.

class Skelevision /* : public ofThread */ {

	public:
	
	/*
	Skelevision();
	~Skelevision();
	 */
	
	void	initialize();
	void	loadXMLRecording (string &xmlFilename, bool bFileIsZipped);
	void	transferFromXmlToCurrentDrawing();
	void	loadAndInitiatePlaybackOfRecording(int which);
	
	void	transferCurrentRecordingToXML();
	void	saveXMLRecording (string &xmlFilename, bool bSaveAsZipped);
	
	void	generateBogusFrames();
	void	drawCurrentPlaybackFrame();
	void	togglePlaybackPaused();
	
	bool	isRecording();
	void	toggleRecording(); 
	void	startRecording();
	void	addFrameToCurrentRecording (vector<PolylinePlus> &theRawDrawing);
	void	stopRecording();
	void	saveCurrentRecording(); 
	int		getCurrentRecordingLength(); 
	bool	bCurrentlyRecording;
	bool	bDeleteOldestFramesWhenRecording;
	bool	bUseZippedFiles;
	int		maxNRecordingFrames;
	
	
	ofxXmlSettings *readXML;
	ofxXmlSettings *saveXML;
	
	vector<PolylinePlus>		 currentPlaybackDrawing; // the currently re-playing drawing
	vector<vector<PolylinePlus>> currentPlaybackFrames;  // the currently re-playing drawings
	vector<vector<PolylinePlus>> currentRecordingFrames; // the drawings currently being recorded
	
	
	int		currentPlaybackFrameIndex;
	bool	bLoadedFileFromXML;
	bool	bPlaybackPaused;
	int		outputFileCounter;

};