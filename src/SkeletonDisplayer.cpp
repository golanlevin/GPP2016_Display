#include "SkeletonDisplayer.h"

//--------------------------------------------------------------
void SkeletonDisplayer::initialize(int w, int h){
	bUseNormalizedDrawings = true;
	bShowPathBetweenBones = true;
	buffer_w = w;
	buffer_h = h;
	
	bDoQuadWarping		= true; 
	bClosedTSP			= false;
	tspElapsed			= 0;
	nOptimizePasses		= 2;
	maxNBonesForTSP		= 60;
	
	combinedDrawing.clear();
	warpedCombinedDrawing.clear();
	optimizedDrawing.clear();
	
	DSM = new DisplaySettingsManager();
	DQW = new DisplayQuadWarper (DSM); 
}

//--------------------------------------------------------------
void SkeletonDisplayer::givePointers(SkeletonTracer *ST, SkeletonLoaderSaver *SLS){
	mySkeletonTracer = ST;
	mySkeletonLoaderSaver = SLS;
}

//--------------------------------------------------------------
void SkeletonDisplayer::update(){
	
	// Update the playback head of the mySkeletonLoaderSaver.
	mySkeletonLoaderSaver->retrieveAndAdvanceCurrentPlaybackDrawing();
	
	// Merge, then optimize, the live & playback drawings into a single drawing
	compileFinalDrawing();
}

//--------------------------------------------------------------
void SkeletonDisplayer::compileFinalDrawing(){
	
	//--------------------
	// 1. Fetch the "live" drawing (if any) from SkeletonTracer;
	vector<PolylinePlus> &theRawDrawing				= mySkeletonTracer->theRawDrawing;
	
	//--------------------
	// 2. Fetch the "playback" drawing (if any) from SkeletonLoaderSaver
	vector<PolylinePlus> &currentPlaybackDrawing	= mySkeletonLoaderSaver->currentPlaybackDrawing;
	
	//--------------------
	// 3. Combine these into combinedDrawing, producing: vector<PolylinePlus>
	combinedDrawing.clear();
	
	// Apply offsets (scale & translation) to various parts NOW
	
	// Add mySkeletonTracer's theRawDrawing;
	if (theRawDrawing.size() > 0){
		for (int i=0; i<theRawDrawing.size(); i++){
			// PolylinePlus ithPP = theRawDrawing[i];
			combinedDrawing.push_back(theRawDrawing[i]); //ithPP);
		}
	}
	
	// Add mySkeletonLoaderSaver's currentPlaybackDrawing;
	if (currentPlaybackDrawing.size() > 0){
		for (int i=0; i<currentPlaybackDrawing.size(); i++){
			// PolylinePlus ithPP = currentPlaybackDrawing[i];
			combinedDrawing.push_back(currentPlaybackDrawing[i]); //ithPP);
		}
	}
	
	// Count total number of points in combinedDrawing
	countTotalPoints();
	
	//--------------------
	// 4. Apply quad warping to combinedDrawing, producing warpedCombinedDrawing
	computeQuadWarpedDrawing (combinedDrawing);
	
	//--------------------
	// 5. Optimize the order of the PolylinePlus's using SkeletonOptimizer
	// Producing: vector<PolylinePlus> optimizedDrawing
	optimallyReorderDrawing (warpedCombinedDrawing);

}


//------------------------------------------------------------
void SkeletonDisplayer::computeQuadWarpedDrawing (vector<PolylinePlus> &drawingToWarp){
	
	if (bDoQuadWarping){
		
		warpedCombinedDrawing.clear();
		int nPolylinePluses = drawingToWarp.size();
		for (int i=0; i<nPolylinePluses; i++){
			
			PolylinePlus ithPP = drawingToWarp[i];
			ofPolyline aPolylineToWarp = ithPP.polyline;
			
			PolylinePlus	aWarpedPolylinePlus;
			ofPolyline		aWarpedPolyline;

			aWarpedPolyline.clear();
			int nPointsInPolyline = aPolylineToWarp.size();
			for (int p=0; p<nPointsInPolyline; p++){
				ofPoint aPointToWarp = aPolylineToWarp[p];
				float inx = aPointToWarp.x;
				float iny = aPointToWarp.y;
				
				float* warpedFloatPair = DQW->warpPointInRect (inx, iny, 1.0, 1.0);
				
				float outx = warpedFloatPair[0];
				float outy = warpedFloatPair[1];
				aWarpedPolyline.addVertex(outx, outy);
			}
			
			aWarpedPolylinePlus.polyline = aWarpedPolyline;
			aWarpedPolylinePlus.r = ithPP.r;
			aWarpedPolylinePlus.g = ithPP.g;
			aWarpedPolylinePlus.b = ithPP.b;
			
			warpedCombinedDrawing.push_back (aWarpedPolylinePlus);
		}
		
	} else {
		
		// If bDoQuadWarping is false
		warpedCombinedDrawing.clear();
		int nPolylinePluses = drawingToWarp.size();
		for (int i=0; i<nPolylinePluses; i++){
			PolylinePlus ithPP = drawingToWarp[i];
			warpedCombinedDrawing.push_back (ithPP);
		}
	}
}



//------------------------------------------------------------
void SkeletonDisplayer::countTotalPoints(){
	totalNPoints = 0;
	int nPolylinePluses = combinedDrawing.size();
	for (int i=0; i<nPolylinePluses; i++){
		totalNPoints += (combinedDrawing[i].polyline).size();
	}
}

//------------------------------------------------------------
void SkeletonDisplayer::optimallyReorderDrawing (vector<PolylinePlus> &drawingToReorder){
	long then = ofGetElapsedTimeMicros();
	
	optimizedDrawing.clear();
	nCombinedPolylinePluses = drawingToReorder.size();
	
	if (nCombinedPolylinePluses > 0){
		if ((bDoOptimizeTSP) && (nCombinedPolylinePluses < maxNBonesForTSP)){
			
			mySkeletonOptimizer.optimallyReorderBones(drawingToReorder, nOptimizePasses, bClosedTSP);
			optimizedDrawing = mySkeletonOptimizer.theOptimizedDrawing;
			optimizationAmount = mySkeletonOptimizer.optimizationAmount;
			
		} else {
			// Either bDoOptimizeTSP is disabled, or maxNBonesForTSP was exceeded.
			// Copy directly into theOptimizedDrawing instead,
			// with no reordering-based (TSP) optimization.
			for (int i=0; i<nCombinedPolylinePluses; i++){
				PolylinePlus aPolylinePlus = drawingToReorder[i];
				optimizedDrawing.push_back(aPolylinePlus);
			}
		}
	}
	
	long now = ofGetElapsedTimeMicros();
	float A = 0.98; float B = 1.0-A;
	tspElapsed = A*tspElapsed + B*(now-then);
}



//--------------------------------------------------------------
void SkeletonDisplayer::renderToScreen(){
	// Render the optimizedDrawing to the screen.
	// Assumes that update() has been called. 
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);
	ofPushMatrix();
	
	if (bUseNormalizedDrawings){ ofScale(buffer_w, buffer_w); }
	renderVectorOfPolylinePluses (optimizedDrawing);
	
	ofPopMatrix();
	glPopAttrib(); // Restore blend mode defaults
}

//--------------------------------------------------------------
void SkeletonDisplayer::renderDisplayQuadWarper(){
	ofPushMatrix();
	if (bUseNormalizedDrawings){ ofScale(buffer_w, buffer_w); }
	DQW->drawBoundary();
	ofPopMatrix();
}

//--------------------------------------------------------------
void SkeletonDisplayer::renderRawInputsToScreen(){
	// Render the (uncombined) raw inputs to the screen.
	
	vector<PolylinePlus> &theRawDrawing				= mySkeletonTracer->theRawDrawing;
	vector<PolylinePlus> &currentPlaybackDrawing	= mySkeletonLoaderSaver->currentPlaybackDrawing;
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);
	ofPushMatrix();
	
	if (bUseNormalizedDrawings){ ofScale(buffer_w, buffer_w); }
	renderVectorOfPolylinePluses (theRawDrawing);
	renderVectorOfPolylinePluses (currentPlaybackDrawing);

	ofPopMatrix();
	glPopAttrib(); // Restore blend mode default.
}

//--------------------------------------------------------------
void SkeletonDisplayer::renderVectorOfPolylinePluses (vector<PolylinePlus> &aDrawing){
	// Render a provided vector of PolylinePluses to the screen.
	
	int nPolylinePlusses = aDrawing.size();
	if (nPolylinePlusses > 0){
		
		float x0 = 0;
		float y0 = 0;
		float x1 = 0;
		float y1 = 0;
		if (bClosedTSP){
			PolylinePlus ithPP = aDrawing[nPolylinePlusses-1];
			ofPolyline aBone = ithPP.polyline;
			x0 = aBone[aBone.size()-1].x;
			y0 = aBone[aBone.size()-1].y;
		}
		
		for (int i=0; i<nPolylinePlusses; i++){
			PolylinePlus ithPP = aDrawing[i];
			ofPolyline aBone = ithPP.polyline;
			int nPointsInBone = aBone.size();
			
			if (nPointsInBone >= 2){
				ofSetLineWidth(2.0);
				ofSetColor(ithPP.r, ithPP.g, ithPP.b);
				aBone.draw();
				
				if (bShowPathBetweenBones){
					x1 = aBone[0].x;
					y1 = aBone[0].y;
					if ((i > 0) || (bClosedTSP)){
						ofSetLineWidth(1.0);
						ofSetColor(0,144,0);
						ofDrawLine(x0,y0, x1,y1);
					}
					x0 = aBone[nPointsInBone-1].x;
					y0 = aBone[nPointsInBone-1].y;
				}
			}
		}
	}
}


