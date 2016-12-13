#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"

#include "SkeletonOptimizer.h"
#include "SkeletonLoaderSaver.h"
#include "SkeletonTracer.h"

class SkeletonDisplayer {
	
	public:
	
	void	initialize(int w, int h);
	void	givePointers(SkeletonTracer *ST, SkeletonLoaderSaver *SLS);
	
	void	update();
	void	compileFinalDrawing();
	void	renderToScreen();
	void	renderRawInputsToScreen();
	void	renderToLaser();
	void	renderVectorOfPolylinePluses (vector<PolylinePlus> &aDrawing);
	
	vector<PolylinePlus> combinedDrawing;
	vector<PolylinePlus> optimizedDrawing;
	
	SkeletonTracer		*mySkeletonTracer;
	SkeletonLoaderSaver	*mySkeletonLoaderSaver;
	
	bool	bUseNormalizedDrawings;
	bool	bShowPathBetweenBones;
	int		buffer_w;
	int		buffer_h;
	
	
	SkeletonOptimizer		mySkeletonOptimizer;
	void					optimallyReorderRawDrawing();
	bool					bDoOptimizeTSP;
	bool					bClosedTSP;
	int						nCombinedPolylinePluses;
	int						maxNBonesForTSP;
	int						nOptimizePasses;
	float					tspElapsed;
	float					optimizationAmount;

};