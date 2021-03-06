#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxXmlSettings.h"
#include "ofxZip.h"
#include "XMLThread.h"

#include "SkeletonDefs.h"
#include "SkeletonOptimizer.h" // Included for PolylinePlus struct.

class SkeletonLoaderSaver {

	public:
	
	
	SkeletonLoaderSaver();
	~SkeletonLoaderSaver();
    
    // ofEvent CallBack loading XML
    void recordingSaved(string & filename);
    void recordingLoaded(ofxXmlSettings & xml);
	
	void	initialize(int w, int h);
	void	loadXMLRecording (string &xmlFilename, bool bFileIsZipped);
	void	transferFromXmlToCurrentDrawing();
	void	loadAndInitiatePlaybackOfRecording(int which);
	void	loadAndInitiatePlaybackOfRandomRecording();
	void	loadAndInitiatePlaybackOfNextRecording();

	void	saveXMLRecording (string &xmlFilename, bool bSaveAsZipped);
	
	void	generateBogusFrames();
	void	togglePlaybackPaused();
	
	bool	isRecording();
	void	clear(); 
	void	toggleRecording(); 
	void	startRecording();
	void	addFrameToCurrentRecording (vector<PolylinePlus> &theRawDrawing);
	void	stopRecording();
	void	saveCurrentRecording();
	void	retrieveAndAdvanceCurrentPlaybackDrawing();
	long	recordingStartTime; //ms
	
	int		getCurrentRecordingLength();
	int		getCurrentPlaybackLength();
	int		getCurrentPlaybackRecordingIndex();
	int		getCurrentPlaybackFrameIndex();
	
	bool	bCurrentlyRecording;
	bool	bDeleteOldestFramesWhenRecording;
	bool	bUseZippedFiles;
	bool	bPlayBackAndForth;
	int		playbackDirection;
	int		minNRecordingFrames;
	int		maxNRecordingFrames;
	int		maxNRecordings;
	
	
	ofxXmlSettings readXML;
	ofxXmlSettings saveXML;
	
	vector<PolylinePlus>		 currentPlaybackDrawing; // the current frame of the now-playing sequence
	vector<vector<PolylinePlus>> currentPlaybackFrames;  // the sequence of currently re-playing drawings
	vector<vector<PolylinePlus>> currentRecordingFrames; // the drawings currently being recorded
	
	int		playbackRecordingIndex;
	int		currentPlaybackFrameIndex;
	float	currentPlaybackFramePercent;
	bool	bLoadedFileFromXML;
	bool	bPlaybackPaused;
	static int	outputFileCounter;
	
	
	bool	bUseNormalizedDrawings;
	int		buffer_w;
	int		buffer_h;

    vector<string> savedFiles;
    
    XMLThread				xmlThread;
	ofDirectory				recordingsDirectory;
	ofFloatColor			replayColor;
    
    
};