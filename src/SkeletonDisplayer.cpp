#include "SkeletonDisplayer.h"

//--------------------------------------------------------------
void SkeletonDisplayer::initialize(int w, int h){
	bUseNormalizedDrawings = true;
	bShowPathBetweenBones = true;
	buffer_w = w;
	buffer_h = h;
	
	bClosedTSP			= false;
	tspElapsed			= 0;
	nOptimizePasses		= 2;
	maxNBonesForTSP		= 60;
	
	combinedDrawing.clear();
	optimizedDrawing.clear();
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
	
	// 1. Fetch the "live" drawing (if any) from SkeletonTracer;
	vector<PolylinePlus> &theRawDrawing				= mySkeletonTracer->theRawDrawing;
	
	// 2. Fetch the "playback" drawing (if any) from SkeletonLoaderSaver
	vector<PolylinePlus> &currentPlaybackDrawing	= mySkeletonLoaderSaver->currentPlaybackDrawing;
	
	// 3. Combine these into combinedDrawing, producing: vector<PolylinePlus>
	combinedDrawing.clear();
	
	// Add mySkeletonTracer's theRawDrawing;
	if (theRawDrawing.size() > 0){
		for (int i=0; i<theRawDrawing.size(); i++){
			PolylinePlus ithPP = theRawDrawing[i];
			combinedDrawing.push_back(ithPP);
		}
	}
	
	// Add mySkeletonLoaderSaver's currentPlaybackDrawing;
	if (currentPlaybackDrawing.size() > 0){
		for (int i=0; i<currentPlaybackDrawing.size(); i++){
			PolylinePlus ithPP = currentPlaybackDrawing[i];
			combinedDrawing.push_back(ithPP);
		}
	}
	
	// 4. Optimize the order of the PolylinePlus's using SkeletonOptimizer
	// Producing: vector<PolylinePlus> optimizedDrawing
	optimallyReorderRawDrawing();

}


//------------------------------------------------------------
void SkeletonDisplayer::optimallyReorderRawDrawing(){
	long then = ofGetElapsedTimeMicros();
	
	optimizedDrawing.clear();
	nCombinedPolylinePluses = combinedDrawing.size();
	
	if (nCombinedPolylinePluses > 0){
		if ((bDoOptimizeTSP) && (nCombinedPolylinePluses < maxNBonesForTSP)){
			
			mySkeletonOptimizer.optimallyReorderBones(combinedDrawing, nOptimizePasses, bClosedTSP);
			optimizedDrawing = mySkeletonOptimizer.theOptimizedDrawing;
			optimizationAmount = mySkeletonOptimizer.optimizationAmount;
			
		} else {
			// Either bDoOptimizeTSP is disabled, or maxNBonesForTSP was exceeded.
			// Copy directly into theOptimizedDrawing instead,
			// with no reordering-based (TSP) optimization.
			for (int i=0; i<nCombinedPolylinePluses; i++){
				PolylinePlus aPolylinePlus = combinedDrawing[i];
				optimizedDrawing.push_back(aPolylinePlus);
			}
		}
	}
	
	long now = ofGetElapsedTimeMicros();
	float A = 0.95; float B = 1.0-A;
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
	glPopAttrib(); // Restore blend mode default.
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
			if (aBone.size() >= 2){
				ofSetLineWidth(2.0);
				ofSetColor(ithPP.r, ithPP.g, ithPP.b);
				aBone.draw();
				
				if (bShowPathBetweenBones){
					x1 = aBone[0].x;
					y1 = aBone[0].y;
					if ((i > 0) || (bClosedTSP)){
						ofSetLineWidth(1.0);
						ofSetColor(0,80,0);
						ofDrawLine(x0,y0, x1,y1);
					}
					x0 = aBone[aBone.size()-1].x;
					y0 = aBone[aBone.size()-1].y;
				}
			}
		}
	}
}


