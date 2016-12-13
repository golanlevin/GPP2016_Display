#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"

#include "SkeletonOptimizer.h"
#include "SkeletonLoaderSaver.h"
#include "SkeletonTracer.h"

#include "DisplaySettingsManager.h"
#include "DisplayQuadWarper.h"


class SkeletonDisplayer {
	
	public:
	
	void	initialize(int w, int h);
	void	givePointers(SkeletonTracer *ST, SkeletonLoaderSaver *SLS);
	
	void	update();
	void	compileFinalDrawing();
	void	renderToScreen();
	void	renderRawInputsToScreen();
	void	renderDisplayQuadWarper();
	void	renderToLaser();
	void	renderVectorOfPolylinePluses (vector<PolylinePlus> &aDrawing);
	
	vector<PolylinePlus> combinedDrawing;		// 1. Combination of live & playback
	vector<PolylinePlus> warpedCombinedDrawing;	// 2. Quad-warped combination drawing
	vector<PolylinePlus> optimizedDrawing;		// 3. Final optimized drawing
	
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
	void	computeQuadWarpedDrawing (vector<PolylinePlus> &drawingToWarp);
	bool					bDoQuadWarping;
	
	SkeletonOptimizer		mySkeletonOptimizer;
	void					optimallyReorderDrawing (vector<PolylinePlus> &drawingToReorder);
	bool					bDoOptimizeTSP;
	bool					bClosedTSP;
	int						nCombinedPolylinePluses;
	int						maxNBonesForTSP;
	int						nOptimizePasses;
	float					tspElapsed;
	float					optimizationAmount;

};