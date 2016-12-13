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
	minDistanceForBezier = 0.03;
	minDistanceForConnection = 0.01;
	
	combinedDrawing.clear();
	warpedCombinedDrawing.clear();
	optimizedDrawing.clear();
	
	DSM = new DisplaySettingsManager();
	DQW = new DisplayQuadWarper (DSM);
	
	/*
	ofSetLogLevel(OF_LOG_VERBOSE);
	etherdream.setup();
	etherdream.setPPS(30000);
	 */
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
	
	// 6. Add black connective PolylinePlus's between the drawn lines.
	// Producing: vector<PolylinePlus> finalDrawing
	computeFinalDrawing (optimizedDrawing);
	
	// 7. Add finalDrawing to ildaFrame (for laser).
	generateIldaFrame();

}


//------------------------------------------------------------
void SkeletonDisplayer::generateIldaFrame(){
	//
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
void SkeletonDisplayer::computeFinalDrawing (vector<PolylinePlus> &aDrawing){
	// Add black PolylinePlus's between PolylinePlus's.
	// Assume bClosedTSP.
	
	finalDrawing.clear();
	float bezierStrength = 3;
	int bezierResolution = 10;
	int nPtsForShortLine = 3;
	
	int nPolylinePlusses = aDrawing.size();
	if (nPolylinePlusses > 0){
		
		for (int i=0; i<nPolylinePlusses; i++){
			PolylinePlus ithPP = aDrawing[i];
			finalDrawing.push_back(ithPP); //------------------------------ ADD PP I
		
			PolylinePlus jthPP = aDrawing[(i+1)%nPolylinePlusses];
			ofPolyline iBone = ithPP.polyline;
			ofPolyline jBone = jthPP.polyline;
			
			int nPointsInBonei = iBone.size();
			int nPointsInBonej = jBone.size();
			if ((nPointsInBonei >= 2) && (nPointsInBonej >= 2)){
				float x0 = iBone[nPointsInBonei-1].x;
				float y0 = iBone[nPointsInBonei-1].y;
				float x3 = jBone[0].x;
				float y3 = jBone[0].y;
				float dist = ofDist(x0,y0, x3,y3);
				
				ofPolyline connectivePolyline;
				connectivePolyline.clear();
				
				if (dist < minDistanceForConnection){
					; // Don't add anything.
					
				} else if (dist < minDistanceForBezier){
					// Generate a short line with some interpolated points.
					for (int j=0; j<=nPtsForShortLine; j++){
						float px = ofMap(j,0,nPtsForShortLine, x0,x3);
						float py = ofMap(j,0,nPtsForShortLine, y0,y3);
						connectivePolyline.addVertex(px, py);
					}
					
				} else {
					// Generate a bezier curve.
					float x1 = x0 + bezierStrength*(x0 - iBone[nPointsInBonei-2].x);
					float y1 = y0 + bezierStrength*(y0 - iBone[nPointsInBonei-2].y);
					float x2 = x3 + bezierStrength*(x3 - jBone[1].x);
					float y2 = y3 + bezierStrength*(y3 - jBone[1].y);
					
					connectivePolyline.addVertex(x0, y0);
					connectivePolyline.bezierTo(x1,y1, x2,y2, x3,y3, bezierResolution);
				}
				
				if (connectivePolyline.size() >= 2){
					PolylinePlus connectivePolylinePlus;
					connectivePolylinePlus.polyline = connectivePolyline;
					connectivePolylinePlus.r = 0;
					connectivePolylinePlus.g = 0;//255
					connectivePolylinePlus.b = 0;
					
					finalDrawing.push_back(connectivePolylinePlus); //---- ADD PP connective
				}
				
			}
		}
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
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_ONE, GL_SRC_ALPHA); // Some other time, perhaps. 
	ofPushMatrix();
	
	if (bUseNormalizedDrawings){ ofScale(buffer_w, buffer_w); }
	renderVectorOfPolylinePluses (finalDrawing); //optimizedDrawing);
	
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
		
		for (int i=0; i<nPolylinePlusses; i++){
			PolylinePlus ithPP = aDrawing[i];
			ofPolyline aBone = ithPP.polyline;
			int nPointsInBone = aBone.size();
			if (nPointsInBone >= 2){
				ofSetLineWidth(2.0);
				ofSetColor(ithPP.r, ithPP.g, ithPP.b);
				aBone.draw();
			}
		}
	}
}

//--------------------------------------------------------------
void SkeletonDisplayer::renderVectorOfPolylinePlusesWithConnectors (vector<PolylinePlus> &aDrawing){
	// Render a provided vector of PolylinePluses to the screen.
	
	int nPolylinePlusses = aDrawing.size();
	if (nPolylinePlusses > 0){
		
		bool bUseBezierConnectors = true;
		if (bUseBezierConnectors && bShowPathBetweenBones){
			
			// Assume bClosedTSP.
			for (int i=0; i<nPolylinePlusses; i++){
				PolylinePlus ithPP = aDrawing[(i                   )];
				PolylinePlus jthPP = aDrawing[(i+1)%nPolylinePlusses];
				
				ofPolyline iBone = ithPP.polyline;
				ofPolyline jBone = jthPP.polyline;
				
				int nPointsInBonei = iBone.size();
				int nPointsInBonej = jBone.size();
				
				if ((nPointsInBonei >= 2) && (nPointsInBonej >= 2)){
				
					ofSetLineWidth(2.0);
					ofSetColor(ithPP.r, ithPP.g, ithPP.b);
					iBone.draw();
					
					float bezierStrength = 3;
					int bezierResolution = 10;
					
					float x0 = iBone[nPointsInBonei-1].x;
					float y0 = iBone[nPointsInBonei-1].y;
					
					float x3 = jBone[0].x;
					float y3 = jBone[0].y;
					
					ofSetLineWidth(1.0);
					ofSetColor(0,144,0);
					
					if (ofDist(x0,y0, x3,y3) < minDistanceForBezier){
						ofDrawLine(x0,y0, x3,y3);
					
					} else {
					
						float x1 = x0 + bezierStrength*(x0 - iBone[nPointsInBonei-2].x);
						float y1 = y0 + bezierStrength*(y0 - iBone[nPointsInBonei-2].y);
						
						float x2 = x3 + bezierStrength*(x3 - jBone[1].x);
						float y2 = y3 + bezierStrength*(y3 - jBone[1].y);
						
						ofPolyline connectivePolyline;
						connectivePolyline.clear();
						connectivePolyline.addVertex(x0, y0);
						connectivePolyline.bezierTo(x1,y1, x2,y2, x3,y3, bezierResolution);
						connectivePolyline.draw();
					}
				}
			}
			
			
			
		} else {
			
			// Use straight lines to connect.
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
}

/*
 
 
 int curveResolution = 8;
 
 ofPolyline connectivePolyline;
 connectivePolyline.clear();
 connectivePolyline.addVertex(x0, y0);
 connectivePolyline.bezierTo(cxa,cya, cxb,cyb, x1,y1, curveResolution);

 */


