#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxKinect.h"
#include "ofxXmlSettings.h"
#include "Skeletonizer.h"
#include "SkeletonTracer.h"
#include "SkeletonLoaderSaver.h"
#include "SkeletonDisplayer.h"
#include "LaserMain.h" // Bryce



class ofApp : public ofBaseApp{

	public:
	
	void 	setup();
	void 	update();
	void 	draw();
	void	exit();
	
	void 	keyPressed(int key);
	void 	keyReleased(int key);
	void 	mouseMoved(int x, int y );
	void 	mouseDragged(int x, int y, int button);
	void 	mousePressed(int x, int y, int button);
	void 	mouseReleased(int x, int y, int button);
	void 	mouseEntered(int x, int y);
	void 	mouseExited(int x, int y);
	void 	windowResized(int w, int h);
	void 	dragEvent(ofDragInfo dragInfo);
	void 	gotMessage(ofMessage msg);
	bool	bFullScreen;
	
	int		displayW;
	int		displayH;
	int		displayM;
	float	displayScale;

    ofxOscReceiver receiver;
	//-----------------------------------------------
	// Proxy video processing.
	// The "real" live app obtains contours over OSC from a PC running KinectV2.
	// However, for proxy development, we load a local video file.
	//
	ofVideoPlayer			proxyVideoPlayer;
	ofxCvColorImage			proxyColorImage;
	string *proxyVideoFilenames;
	int		proxyCaptureW;
	int		proxyCaptureH;
	int		minBlobSize;
	int		maxBlobSize;
	int		maxNBlobs;
	bool	bGetHoles;
	int		nRawContours;
	
	bool	bBlankProxyVideo;
	bool	bProxyVideoPlayerPaused;
	void	captureProxyVideo(); 
	void	computeContoursFromProxyVideo();
	void	sequenceSkeletonPlayback();
	
	//-----------------------------------------------
	// Contour calculation and filtering
	//
	vector<vector<cv::Point>> obtainRawContours();
	void	filterContoursBeforeReconstitution(vector<vector<cv::Point>> &contours);
    void	filterContoursBeforeReconstitutionOSC(vector<vector<cv::Point>> &contours);
	void	reconstituteBlobsFromContours(vector<vector<cv::Point>> &contours, int w, int h);
    void	reconstituteBlobsFromContoursOSC(vector<vector<cv::Point>> &contours, int w, int h);
	void	handleAbsenceOfIncomingContours();
	
	ofxCv::ContourFinder myOfxCvContourFinder;
	int		nCurrentPositiveContours;

	vector<vector<cv::Point>>	theContoursi;
	vector<ofPolyline>			theContoursf;
	cv::Mat						filledContourMat;
	ofImage						filledContourImage;
    vector<int>                isHole;
	//-----------------------------------------------
	// Skeletonization.
	//
	
	Skeletonizer				mySkeletonizer;
	int		kinectV2DepthW;
	int		kinectV2DepthH;
	int		kinectV1DepthW;
	int		kinectV1DepthH;
	int		skeletonBufW;
	int		skeletonBufH;
	float	skeletonScale;
	bool	bScaleProxyToKinect;
	
	
	//-----------------------------------------------
	// GUI Sliders and other controls
	//
	void			initializeGui();
	void			propagateGui();
	
	ofxToggle		bSaveOnExit;
	ofxToggle		bUseProxyVideoInsteadOfOSC;
	ofxPanel		inputGuiPanel1;
	ofxPanel		inputGuiPanel2;
	
	ofxIntSlider	proxyThreshold;
	ofxFloatSlider	inputLineSmoothing;
	ofxFloatSlider	inputLineResample;
	ofxIntSlider	contourThickness;
	ofxToggle		bSmoothHolesToo;
	ofxToggle		bDrawGrayProxy;
	
	ofxFloatSlider	boneResampling;
	ofxFloatSlider	boneSmoothSigma;
	ofxIntSlider	boneSmoothKernW;
	ofxIntSlider	maxNBonesForTSP;
	ofxIntSlider	nOptimizePasses; 
	ofxToggle		bDoMergeBones; 
	ofxToggle		bDoOptimizeTSP;
	ofxToggle		bClosedTSP;
	ofxToggle		bEnableRecording; 
	
	ofxIntSlider	targetPointCount;
	ofxIntSlider	preBlankCount;
	ofxIntSlider	preRepeatCount;
	ofxIntSlider	postRepeatCount;
	ofxIntSlider	postBlankCount;
	
	ofxFloatSlider	overallScaleX;
	ofxFloatSlider	overallScaleY;
	ofxFloatSlider	overallTransX;
	ofxFloatSlider	overallTransY;
	ofxFloatSlider	liveDrawingScale;
	ofxFloatSlider	noiseDiv;
	ofxFloatSlider	noiseAmt;
	ofxFloatSlider	positionAmt; 
	
	ofxToggle		bFadeColorsAtEdges;
	ofxToggle		bFadeColorsAtEnds;
	ofxToggle		bAddTestPattern;
	ofxToggle		bVSync;
	
	ofxFloatSlider	replayR;
	ofxFloatSlider	replayG;
	ofxFloatSlider	replayB;
	ofxFloatSlider	liveR;
	ofxFloatSlider	liveG;
	ofxFloatSlider	liveB;
	
	float			presenceFractionAvg;
	
	ofxIntSlider	refreshPeriod;
	int				refreshPeriodRandomized;
	long			lastProxySwitchTime; 
	int				whichRefresh;
	long			lastRefreshTime;
	void			refreshSkeletonPlaybacker(int which);
	
	
	
	ofxFloatSlider	recordStartThresh;
	ofxFloatSlider	recordStopThresh;
	ofxFloatSlider	tooMuchPresenceThresh;
	
	bool			bRecording;
	void			toggleRecording();
	void			startRecording();
	void			stopRecording();
	void			stopAndSaveRecording();
	
	//-----------------------------------------------
	// Skeleton tracing produces ofPolylines.
	//
	SkeletonTracer	*mySkeletonTracer;
	bool bGotAProxyFrame;
	
	SkeletonLoaderSaver		*mySkeletonPlaybackers[2];
	SkeletonLoaderSaver		*mySkeletonRecorder;
	
	SkeletonDisplayer		mySkeletonDisplayer;
	float					mainDisplayAreaScale;


};
