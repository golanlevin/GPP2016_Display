#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxXmlSettings.h"
#include "ofxZip.h"
#include "XMLThread.h"

#include "SkeletonOptimizer.h" // Included for PolylinePlus struct.

class SkeletonLoaderSaver {//: public ofThread {

	public:
	
	
	SkeletonLoaderSaver();
	~SkeletonLoaderSaver();
    
    
//    void    start();
//    void    stop();
//    void    threadedFunction();
    
    // ofEvent CallBack loading XML
    void recordingSaved(string & filename);
    void recordingLoaded(ofxXmlSettings & xml);
	
	void	initialize(int w, int h);
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
	void	retrieveAndAdvanceCurrentPlaybackDrawing();
	int		getCurrentRecordingLength(); 
	bool	bCurrentlyRecording;
	bool	bDeleteOldestFramesWhenRecording;
	bool	bUseZippedFiles;
	int		maxNRecordingFrames;
	
	
	ofxXmlSettings readXML;
	ofxXmlSettings saveXML;
	
	vector<PolylinePlus>		 currentPlaybackDrawing; // the current frame of the now-playing sequence
	vector<vector<PolylinePlus>> currentPlaybackFrames;  // the sequence of currently re-playing drawings
	vector<vector<PolylinePlus>> currentRecordingFrames; // the drawings currently being recorded
	
	
	int		currentPlaybackFrameIndex;
	bool	bLoadedFileFromXML;
	bool	bPlaybackPaused;
	int		outputFileCounter;
	
	bool	bUseNormalizedDrawings;
	int		buffer_w;
	int		buffer_h;

    vector<string> savedFiles;
    
    XMLThread xmlThread;
    
    
};