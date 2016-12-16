#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"

#include "SkeletonDefs.h"
#include "SkeletonOptimizer.h"
#include "SkeletonLoaderSaver.h"
#include "SkeletonTracer.h"

#include "DisplaySettingsManager.h"
#include "DisplayQuadWarper.h"
#include "ofxEtherdream.h"
#include "ofxIldaFrame.h"

class SkeletonDisplayer {
	
	public:
	
	bool			bUsingLaser;
	ofxIlda::Frame	ildaFrame;	// Data structure to store and manage ILDA frame drawings
	ofxEtherdream	etherdream; // The interface to the etherdream device
	void			generateAndSendIldaFrame();
	vector<ofxIlda::Poly>	laserPolys;
	bool			bDoFlipX;
	bool			bDoFlipY;
	bool			bFadeColorsAtEdges;
	bool			bFadeColorsAtEnds;
	bool			bAddTestPattern;
	ofFloatColor	liveColor;

	
	bool			bUseBeziersForConnectives;
	bool			bConnectToYesterframeLastPoint;
	ofPoint			yesterframeLastPoint;
	
	
	void	initialize(int w, int h);
	void	givePointers(SkeletonTracer *ST, SkeletonLoaderSaver *SLS);
	
	void	update();
	void	compileFinalDrawing();
	void	addDrawing (vector<PolylinePlus> &aDrawing, ofFloatColor &drawingColor,
						bool bPaused, int whichType, float timePercent, int whichRecording);
	void	addDrawingSimple (vector<PolylinePlus> &aDrawing);
	void	addTestPattern();
	void	renderToScreen();
	void	renderRawInputsToScreen();
	void	renderDisplayQuadWarper();
	void	renderToLaser();
	void	renderVectorOfPolylinePluses (vector<PolylinePlus> &aDrawing);
	void	renderVectorOfPolylinePlusesWithConnectors (vector<PolylinePlus> &aDrawing);
	void	setToHomePosition();
	
	float	overallScaleX;
	float	overallScaleY;
	
	
	vector<PolylinePlus> combinedDrawing;		// 1. Combination of live & playback
	vector<PolylinePlus> warpedCombinedDrawing;	// 2. Quad-warped combination drawing
	vector<PolylinePlus> optimizedDrawing;		// 3. Optimized drawing (order & direction)
	vector<PolylinePlus> finalDrawing;			// 4. Final drawing: Optimized, + black connections.
	
	
	SkeletonTracer		*mySkeletonTracer;
	SkeletonLoaderSaver	*mySkeletonLoaderSaver;
	
	bool	bUseNormalizedDrawings;
	bool	bShowPathBetweenBones;
	int		buffer_w;
	int		buffer_h;
	
	void	countTotalPoints();
	int		totalNPoints;
	
	DisplaySettingsManager	*DSM;
	DisplayQuadWarper		*DQW;
	void					computeQuadWarpedDrawing (vector<PolylinePlus> &drawingToWarp);
	bool					bDoQuadWarping;
	
	SkeletonOptimizer		mySkeletonOptimizer;
	void					optimallyReorderDrawing (vector<PolylinePlus> &drawingToReorder);
	void					computeFinalDrawing (vector<PolylinePlus> &optimizedDrawing);
	bool					bDoOptimizeTSP;
	bool					bClosedTSP;
	int						nCombinedPolylinePluses;
	int						maxNBonesForTSP;
	int						nOptimizePasses;
	float					tspElapsed;
	float					optimizationAmount;
	float					minDistanceForBezier;
	float					minDistanceForConnection;
	int						connectorResolution;
	
	float					MIN_POINT_VALUE;
	float					MAX_POINT_VALUE;
	float					function_TukeyWindow (float x, float a);

};