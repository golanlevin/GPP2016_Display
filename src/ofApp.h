#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	public:
	
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);


	ofVideoPlayer			proxyVideoPlayer;
	ofxCvColorImage			proxyColorImage;
	
	int		proxyCaptureW;
	int		proxyCaptureH;
	int		minBlobSize;
	int		maxBlobSize;
	int		maxNBlobs;
	bool	bGetHoles;

	bool	bProxyVideoPlayerPaused;
	bool	bUseProxyVideoInsteadOfOSC;
	vector<vector<cv::Point>> obtainRawContours();
	void	computeContoursFromProxyVideo();
	void	filterContoursBeforeReconstitution(vector<vector<cv::Point>> contours);
	void	reconstituteBlobsFromContours(vector<vector<cv::Point>> contours, int w, int h);
	void	computeSkeletonImageFromBlobs(vector<vector<cv::Point>> contours, int w, int h);
	

	

	ofxCv::ContourFinder myOfxCvContourFinder;
	int		nCurrentPositiveContours;

	
	vector<vector<cv::Point>>	theContoursi;
	vector<ofPolyline>			theContoursf;
	cv::Mat						filledContourMat;
	ofImage						filledContourImage;
	
	
	//-----------------------------------------------
	// Skeletonization.
	void	skeletonize();
	inline int thin (int pass, unsigned char *table);
	unsigned char* inputBuff;
	unsigned char* tmpBuffer;
	int		skeletonBufW;
	int		skeletonBufH;
	float	skeletonizationDuration;
	float	skeletonScale;
	bool	bScaleProxyToKinect;
	int		bufW, bufH;
	int		roiMinX, roiMaxX;
	int		roiMinY, roiMaxY;
	
	unsigned char* skeletonBuffer;
	ofImage	skeletonImage;


	int		displayW;
	int		displayH;
	int		displayM;
	float	displayScale;
	
	
	// Settings for inputs
	ofxPanel		inputGuiPanel;
	ofxIntSlider	proxyThreshold;
	ofxFloatSlider	inputLineSmoothing;
	ofxFloatSlider	inputLineResample;
	ofxIntSlider	contourThickness;
	ofxToggle		bSmoothHolesToo; 
	

	

};
