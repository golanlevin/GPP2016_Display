#include "SkeletonDisplayer.h"

//--------------------------------------------------------------
void SkeletonDisplayer::initialize(int w, int h){
	bUseNormalizedDrawings = true;
	bShowPathBetweenBones = true;
	buffer_w = w;
	buffer_h = h;
	
	bDoQuadWarping		= true; 
	bClosedTSP			= true;
	bDoFlipX			= true;
	bDoFlipY			= false;
	bFadeColorsAtEdges	= true;
	bAddTestPattern		= false;
	bUseBeziersForConnectives = false;
	connectorResolution = 9;
	
	overallScaleX		= 0.25;
	overallScaleY		= 0.25;
	liveColor.set(1.0, 0.0, 0.0);
	
	bConnectToYesterframeLastPoint = true;
	yesterframeLastPoint.set(0.5, 0.5);
	
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
	
	bUsingLaser = true;
	if (bUsingLaser){
		ofSetLogLevel(OF_LOG_VERBOSE);
		etherdream.setup();
		etherdream.setPPS(30000);
		ofSetLogLevel(OF_LOG_NOTICE);
	}
	
	MIN_POINT_VALUE = 2.0/(pow(2, 16));
	MAX_POINT_VALUE = 1.0 - MIN_POINT_VALUE;
	
	

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
	// 3. Combine these into combinedDrawing, producing: vector<PolylinePlus>.
	// Apply offsets (scale & translation) to various parts NOW
	
	combinedDrawing.clear();
	bool bPaused = mySkeletonLoaderSaver->bPlaybackPaused;
	float timePercent = mySkeletonLoaderSaver->currentPlaybackFramePercent;
	int whichRecording = mySkeletonLoaderSaver->recordingIndex;
	addDrawing (theRawDrawing,liveColor, bPaused, THE_LIVE_DRAWING, 0.5, 0);
	addDrawing (currentPlaybackDrawing, mySkeletonLoaderSaver->replayColor,
				bPaused, PLAYBACK_DRAWING, timePercent, whichRecording);
	addTestPattern();

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
	
	// 7. Add finalDrawing to ildaFrame, and send points to the etherdream DAC
	generateAndSendIldaFrame();
	

}

//------------------------------------------------------------
void SkeletonDisplayer::addDrawingSimple (vector<PolylinePlus> &aDrawing){
	if (aDrawing.size() > 0){
		for (int i=0; i<aDrawing.size(); i++){
			combinedDrawing.push_back(aDrawing[i]);
		}
	}
}

//------------------------------------------------------------
void SkeletonDisplayer::addDrawing (vector<PolylinePlus> &aDrawing,
									ofFloatColor &drawingColor,
									bool bPaused, int whichType,
									float timePercent, int whichRecording){
	
	// Add mySkeletonLoaderSaver's currentPlaybackDrawing;
	// vector<PolylinePlus> &aDrawing = aSLS->currentPlaybackDrawing;
	if (aDrawing.size() > 0){
		
		// Animation of scale & position of the playback happens here.
		float scaleX = overallScaleX;
		float scaleY = overallScaleY;
		float transX = 0.0;
		float transY = 0.0;
		
		if ((whichType == PLAYBACK_DRAWING) && (!bPaused)){
			float t = ofGetElapsedTimeMillis()/10000.0;
			transX += 0.750 * (ofNoise(t+whichRecording) - 0.5);
			transX += ofMap((whichRecording % 10),0,9, -0.25,0.25);
			transY += 0.060 * (ofNoise(t+whichRecording+10) - 0.5);
		}
		
		if (whichType == THE_LIVE_DRAWING){
			scaleX *= 1.15;
			scaleY *= 1.15;
		}
		
		
		for (int i=0; i<aDrawing.size(); i++){
			PolylinePlus ithPP = aDrawing[i];
			ofPolyline ithPolyline = ithPP.polyline;
			
			ofPolyline scaledShiftedPolyline;
			scaledShiftedPolyline.clear();
			
			int nPoints = ithPolyline.size();
			for (int p=0; p<nPoints; p++){
				ofPoint pthPoint = ithPolyline[p];
				float px = pthPoint.x;
				float py = pthPoint.y;
				
				px -= 0.5;
				py -= 0.5;
				px *= scaleX;
				py *= scaleY;
				px += 0.5;
				py += 0.5;
				px += transX;
				py += transY;
				
				px = ofClamp(px, MIN_POINT_VALUE,MAX_POINT_VALUE);
				py = ofClamp(py, MIN_POINT_VALUE,MAX_POINT_VALUE);
				px += ofRandom(-0.000001,0.000001); // paranoid epsilon to avoid divzero
				py += ofRandom(-0.000001,0.000001);
				scaledShiftedPolyline.addVertex(px, py);
			}
			
			PolylinePlus scaledShiftedPP;
			scaledShiftedPP.polyline = scaledShiftedPolyline;
			
			// Fade the colors depending on
			// (A) whether the drawing is too close to the edge of the screen, and
			// (B) at the start and end of the drawing loop.
			float colorFade = 1.0;
			if (bFadeColorsAtEdges){
				float tukeyWindowAmount = 0.08;
				ofPoint centroid = scaledShiftedPolyline.getCentroid2D();
				float centX01 = ofClamp(centroid.x, 0,1);
				float centY01 = ofClamp(centroid.y, 0,1);
				float fadeX = function_TukeyWindow (centX01, tukeyWindowAmount);
				float fadeY = function_TukeyWindow (centY01, tukeyWindowAmount);
				colorFade *= fadeX * fadeY;
			}
			if (bFadeColorsAtEnds && (whichType == PLAYBACK_DRAWING)){
				float tukeyWindowAmount = 0.12;
				float fadeT = function_TukeyWindow(timePercent, tukeyWindowAmount);
				colorFade *= fadeT;
			}
			
			
			scaledShiftedPP.r = 255.0 * colorFade * drawingColor.r; //aSLS->replayColor.r;
			scaledShiftedPP.g = 255.0 * colorFade * drawingColor.g; //aSLS->replayColor.g;
			scaledShiftedPP.b = 255.0 * colorFade * drawingColor.b; //aSLS->replayColor.b;

			combinedDrawing.push_back(scaledShiftedPP);
		}
	}
}



//------------------------------------------------------------
void SkeletonDisplayer::addTestPattern(){
	if (bAddTestPattern){
		
		ofPolyline testpatternPolyline;
		testpatternPolyline.clear();
		
		int nPointsPerSide = 20;
		for (int i=0; i<nPointsPerSide; i++){
			float x0 = MIN_POINT_VALUE; float y0 = MIN_POINT_VALUE;
			float x1 = MIN_POINT_VALUE; float y1 = MAX_POINT_VALUE;
			float px = MIN_POINT_VALUE;
			float py = ofMap(i, 0,nPointsPerSide, y0,y1);
			testpatternPolyline.addVertex(px,py);
		}
		for (int i=0; i<nPointsPerSide; i++){
			float x0 = MIN_POINT_VALUE; float y0 = MAX_POINT_VALUE;
			float x1 = MAX_POINT_VALUE; float y1 = MAX_POINT_VALUE;
			float px = ofMap(i, 0,nPointsPerSide, x0,x1);
			float py = MAX_POINT_VALUE;
			testpatternPolyline.addVertex(px,py);
		}
		for (int i=0; i<nPointsPerSide; i++){
			float x0 = MAX_POINT_VALUE; float y0 = MAX_POINT_VALUE;
			float x1 = MAX_POINT_VALUE; float y1 = MIN_POINT_VALUE;
			float px = MAX_POINT_VALUE;
			float py = ofMap(i, 0,nPointsPerSide, y0,y1);
			testpatternPolyline.addVertex(px,py);
		}
		for (int i=0; i<nPointsPerSide; i++){
			float x0 = MAX_POINT_VALUE; float y0 = MIN_POINT_VALUE;
			float x1 = MIN_POINT_VALUE; float y1 = MIN_POINT_VALUE;
			float px = ofMap(i, 0,nPointsPerSide, x0,x1);
			float py = MIN_POINT_VALUE;
			testpatternPolyline.addVertex(px,py);
		}
		testpatternPolyline.addVertex(MIN_POINT_VALUE+0.00004,MIN_POINT_VALUE+0.00004);
		
		PolylinePlus testpatternPP;
		testpatternPP.polyline = testpatternPolyline;
		testpatternPP.r = 255;
		testpatternPP.g = 255;
		testpatternPP.b = 0;
		
		combinedDrawing.push_back(testpatternPP);
	}
}



//------------------------------------------------------------
void SkeletonDisplayer::generateAndSendIldaFrame(){
	if (bUsingLaser){
		
		ildaFrame.clear();
		int nPolylinePluses = finalDrawing.size();
		for (int i=0; i<nPolylinePluses; i++){
			PolylinePlus ithPP = finalDrawing[i];
			ofFloatColor ithPPColor;
			ithPPColor.set(ithPP.r, ithPP.g, ithPP.b);
			
			ildaFrame.addPoly(ithPP.polyline, ithPPColor);
		}
		
		ildaFrame.params.output.transform.doFlipX = bDoFlipX;
		ildaFrame.params.output.transform.doFlipY = bDoFlipY;
		ildaFrame.update();
		etherdream.setPoints(ildaFrame);
	}
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
	int nPtsForShortLine = 4;
	
	int nPolylinePlusses = aDrawing.size();
	if (nPolylinePlusses > 0){
		
		
		if (bConnectToYesterframeLastPoint){
			// What to do when the current frame starts at a
			// different place than where last frame left off.
			// Must do this!
			
			float lastX = yesterframeLastPoint.x;
			float lastY = yesterframeLastPoint.y;
			
			PolylinePlus firstPP = aDrawing[0];
			ofPolyline firstPl = firstPP.polyline;
			ofPoint firstPt = firstPl[0];
			float firstX = firstPt.x;
			float firstY = firstPt.y;

			float distSinceYesterframe = ofDist(lastX,lastY, firstX,firstY);
			if (distSinceYesterframe > minDistanceForConnection){
				
				ofPolyline connectivePolyline;
				connectivePolyline.clear();
				for (int j=0; j<=nPtsForShortLine; j++){
					float px = ofMap(j,0,nPtsForShortLine, lastX,firstX);
					float py = ofMap(j,0,nPtsForShortLine, lastY,firstY);
					connectivePolyline.addVertex(px, py);
				}
				
				PolylinePlus connectivePolylinePlus;
				connectivePolylinePlus.polyline = connectivePolyline;
				connectivePolylinePlus.r = 0;
				connectivePolylinePlus.g = 0; 
				connectivePolylinePlus.b = 0;
				
				finalDrawing.push_back(connectivePolylinePlus); //---- ADD yester connective
			}
		}

		
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
					
				} else if (bUseBeziersForConnectives == false){
					// Generate a straight line, don't bother with bezier.
					for (int j=0; j<=connectorResolution; j++){
						float px = ofMap(j,0,connectorResolution, x0,x3);
						float py = ofMap(j,0,connectorResolution, y0,y3);
						connectivePolyline.addVertex(px, py);
					}
					
				} else {
					// Generate a bezier curve.
					float x1 = x0 + bezierStrength*(x0 - iBone[nPointsInBonei-2].x);
					float y1 = y0 + bezierStrength*(y0 - iBone[nPointsInBonei-2].y);
					float x2 = x3 + bezierStrength*(x3 - jBone[1].x);
					float y2 = y3 + bezierStrength*(y3 - jBone[1].y);
					
					connectivePolyline.addVertex(x0, y0);
					connectivePolyline.bezierTo(x1,y1, x2,y2, x3,y3, connectorResolution);
				}
				
				if (connectivePolyline.size() >= 2){
					// Clamp points in connectivePolyline to (0...1);
					for (int p=0; p<connectivePolyline.size(); p++){
						float px = connectivePolyline[p].x;
						float py = connectivePolyline[p].y;
						connectivePolyline[p].x = ofClamp(px, MIN_POINT_VALUE,MAX_POINT_VALUE);
						connectivePolyline[p].y = ofClamp(py, MIN_POINT_VALUE,MAX_POINT_VALUE);
					}
					
					PolylinePlus connectivePolylinePlus;
					connectivePolylinePlus.polyline = connectivePolyline;
					connectivePolylinePlus.r = 0;
					connectivePolylinePlus.g = 0;
					connectivePolylinePlus.b = 0;
					
					finalDrawing.push_back(connectivePolylinePlus); //---- ADD PP connective
				}
				
			}
		}
		
		if (bConnectToYesterframeLastPoint){
			PolylinePlus lastPP = finalDrawing.back();
			ofPolyline lastPl = lastPP.polyline;
			int nPts = lastPl.size();
			ofPoint lastPt = lastPl[nPts-1];
			float lastX = lastPt.x;
			float lastY = lastPt.y;
			yesterframeLastPoint.set(lastX, lastY);
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
						connectivePolyline.bezierTo(x1,y1, x2,y2, x3,y3, connectorResolution);
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

//--------------------------------------------------------------
void SkeletonDisplayer::setToHomePosition(){
	
	if (bUsingLaser){
		ildaFrame.clear();
		
		ofPolyline centerPolyline;
		for (int i=0; i<1000; i++){
			centerPolyline.addVertex(0.5, 0.5);
		}
	
		ildaFrame.addPoly(centerPolyline);
		ildaFrame.update();
		etherdream.setPoints(ildaFrame);
	}
}




//------------------------------------------------------------------
float SkeletonDisplayer::function_TukeyWindow (float x, float a) {
	
	// functionName = "Tukey Window";
	// http://en.wikipedia.org/wiki/Window_function
	// The Tukey window, also known as the tapered cosine window,
	// can be regarded as a cosine lobe of width \tfrac{\alpha N}{2}
	// that is convolved with a rectangle window of width \left(1 -\tfrac{\alpha}{2}\right)N.
	// At alpha=0 it becomes rectangular, and at alpha=1 it becomes a Hann window.
	
	float ah = a/2.0;
	float omah = 1.0 - ah;
	
	float y = 1.0;
	if (x <= ah) {
		y = 0.5 * (1.0 + cos(PI* ((2*x/a) - 1.0)));
	}
	else if (x > omah) {
		y = 0.5 * (1.0 + cos(PI* ((2*x/a) - (2/a) + 1.0)));
	}
	return y;
}

