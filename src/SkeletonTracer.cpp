//
//  SkeletonTracer.cpp
//  GPP2016_Display
//
//  Created by GL on 11/27/16.
//
//

#include "SkeletonTracer.h"

/*using namespace bryce_tsp;*/
//------------------------------------------------------------
void SkeletonTracer::initialize (int w, int h){
	
	buffer_w = w;
	buffer_h = h;
	tmpBuffer        = new unsigned char[buffer_w * buffer_h];
	pixelStateBuffer = new unsigned char[buffer_w * buffer_h];
	pixelStateImage.allocate(buffer_w, buffer_h, OF_IMAGE_GRAYSCALE);
	
	boundary0 = buffer_w+1;
	boundary1 = (buffer_w * buffer_h)-buffer_w-1;
	
	bonesRawTraced.clear();
	bonesRawMerged.clear();
	bonesRawSmooth.clear();
	bonesReordered.clear();
	
	nBranchPointIndices = 0;
	branchPointIndices = new int[SKEL_MAX_N_BRANCH_POINTS];
	for (int i=0; i<SKEL_MAX_N_BRANCH_POINTS; i++){
		branchPointIndices[i] = SKEL_INVALID;
	}
	
	boneResampling = 2.5;
	boneSmoothSigma = 0.9;
	boneSmoothKernW = 2;
	maxNBonesForTSP	= 50;
	nOptimizePasses = 2;
	bDoMergeBones	= true;
	bDoOptimizeTSP	= true;
	bClosedTSP		= false;
	
	replayColor		= 0x0000FF;
	liveColor		= 0xFF0000;
}

//------------------------------------------------------------
void SkeletonTracer::exportVectorGraphics(){
	
	string epsFilename = "renders/skeleton_" + ofToString(ofGetHours()) + "_" +
		ofToString(ofGetMinutes()) + "_" + ofToString(ofGetSeconds()) + ".ps";
	
	myVectorGraphics.beginEPS(epsFilename);
	myVectorGraphics.noFill();
	myVectorGraphics.setColor(0x000000);
	bool bCloseShapes = false;
	
	int nBones = bonesRawSmooth.size();
	for (int b=0; b<nBones; b++){
		ofPolyline aBone = bonesRawSmooth[b];
		int nPoints = aBone.size();
		if (nPoints > 4){
			myVectorGraphics.beginShape();
			for (int p=0; p<nPoints; p++){
				float px = aBone[p].x;
				float py = aBone[p].y;
				if ((p==0) || (p == (nPoints-1))){
					myVectorGraphics.curveVertex(px,py);
				}
				myVectorGraphics.curveVertex(px, py);
			}
			myVectorGraphics.endShape(bCloseShapes);
			
		} else if (nPoints >= 2){
			myVectorGraphics.beginShape();
			for (int p=0; p<nPoints; p++){
				float px = aBone[p].x;
				float py = aBone[p].y;
				myVectorGraphics.polyVertex(px, py);
			}
			myVectorGraphics.endShape(bCloseShapes);
		}
	}
	
	myVectorGraphics.endEPS();
}


//------------------------------------------------------------
void SkeletonTracer::computeVectorSkeleton (unsigned char* skeletonPixelBuffer, int nRawContours){
	int nPixels = buffer_w * buffer_h;
	
	if (nRawContours > 0){
		memcpy(pixelStateBuffer, skeletonPixelBuffer, nPixels);
		
		traceVectorSkeletonFromSkeletonImage();
		mergeBones();
		smoothBones();
		trackBones();
		optimallyReorderBones();
		
	} else {
		
		memset(pixelStateBuffer, 0, nPixels);
		bonesRawTraced.clear();
		bonesRawMerged.clear();
		bonesRawSmooth.clear();
		bonesReordered.clear();
		theRawDrawing.clear();
		theOptimizedDrawing.clear();
		currentBone.clear();
	}
}

//------------------------------------------------------------
void SkeletonTracer::traceVectorSkeletonFromSkeletonImage(){
	
	nBranchPointIndices = 0;
	bonesRawTraced.clear();
	currentBone.clear();

	int  	branchLoc;
	int 	locOfFirstUnlookedAtNeighbor;
	int  	nCurrentLocUnlookedAtNeighbors = 0;
	bool 	b_haveNeighboringBranchPoint = false;
	bool 	b_previousLocWasEndOfLine    = false;
	
	int 	nSkeletons  = 0;
	int 	nUnlookedAt = countUnlookedAtPixels();
	float	px,py;
	
	
	if ((nUnlookedAt > 0) && (nUnlookedAt > SKEL_N_WORTHWHILE_PIXELS_THRESH)) {
		int currentLoc = getLocOfFirstSkeletonPixel();
		
		while ((nUnlookedAt > 0) && (currentLoc != SKEL_INVALID)){
			nSkeletons++;
			
			do {
				nCurrentLocUnlookedAtNeighbors = countUnlookedAtNeighbors (currentLoc);
				switch (nCurrentLocUnlookedAtNeighbors){
						
					case 0: //------------------------------------------------------------------
						do { // NO UNLOOKED-AT NEIGHBORS: END OF THE LINE
							markLocationWithState (currentLoc, SKEL_TYPE_LOOKED_AT);
							addPointToCurrentBoneAtPixelIndex (currentLoc);
							
							if (bonesRawTraced.size() < SKEL_MAX_N_BONES){
								bonesRawTraced.push_back(currentBone);
							}
							currentBone.clear();
							b_previousLocWasEndOfLine = true;
							if (nBranchPointIndices > 0){
								
								branchLoc = branchPointIndices[nBranchPointIndices-1];
								nBranchPointIndices--;
								currentLoc = branchLoc;
								nCurrentLocUnlookedAtNeighbors = countUnlookedAtNeighbors (currentLoc);
							}
						} while ((nCurrentLocUnlookedAtNeighbors == 0) && (nBranchPointIndices > 0));
						break;
						
					case 1: // CONTINUATION-----------------------------------------------------
						if (b_previousLocWasEndOfLine == true){
							if (bonesRawTraced.size() < SKEL_MAX_N_BONES){
								bonesRawTraced.push_back(currentBone);
							}
							currentBone.clear();
						}
						b_previousLocWasEndOfLine = false;
						locOfFirstUnlookedAtNeighbor = getLocOfFirstUnlookedAtNeighbor (currentLoc);
						if (getTypeOfPixelAtLoc(currentLoc) == SKEL_TYPE_UNLOOKED_AT) {
							markLocationWithState (currentLoc, SKEL_TYPE_LOOKED_AT);
							addPointToCurrentBoneAtPixelIndex (currentLoc);
						}
						if (locOfFirstUnlookedAtNeighbor != SKEL_INVALID){
							currentLoc = locOfFirstUnlookedAtNeighbor;
						}
						break;
						
					case 2: //------------------------------------------------------------------
					default: // ARRIVED AT A BRANCH POINT. END CURRENT BONE, START NEW ONE
						addPointToCurrentBoneAtPixelIndex (currentLoc);
						locOfFirstUnlookedAtNeighbor = getLocOfFirstUnlookedAtNeighbor (currentLoc);
						markLocationWithState (currentLoc, SKEL_TYPE_BRANCH_POINT);
						if (nBranchPointIndices < SKEL_MAX_N_BRANCH_POINTS){
							branchPointIndices[nBranchPointIndices] = currentLoc;
							nBranchPointIndices++;
						}
						
						if (currentBone.size() > 0){
							if (bonesRawTraced.size() < SKEL_MAX_N_BONES){
								bonesRawTraced.push_back(currentBone);
							}
						}
						currentBone.clear();
						addPointToCurrentBoneAtPixelIndex (currentLoc);
						
						if (locOfFirstUnlookedAtNeighbor != SKEL_INVALID){
							currentLoc = locOfFirstUnlookedAtNeighbor;
							addPointToCurrentBoneAtPixelIndex (currentLoc);
						}
						b_previousLocWasEndOfLine = false;
						break;
						
				}
			} while ((nCurrentLocUnlookedAtNeighbors > 0) && (currentLoc != SKEL_INVALID));
			
			if (currentBone.size() > 0){
				if (bonesRawTraced.size() < SKEL_MAX_N_BONES){
					addPointToCurrentBoneAtPixelIndex (currentLoc);
					bonesRawTraced.push_back(currentBone);
				}
			}
			
			nUnlookedAt = countUnlookedAtPixels();
			currentLoc = getLocOfFirstSkeletonPixel();
			currentBone.clear();
			//addPointToCurrentBoneAtPixelIndex (currentLoc); ///
		}
	}
	// printf("nUnlookedAt: %d | nSkels %d | nBones %d \n", nUnlookedAt, nSkeletons, (int) bonesRawTraced.size());
}

//------------------------------------------------------------
void SkeletonTracer::mergeBones(){
	
	if (bDoMergeBones == false){
		bonesRawMerged.clear();
		int nRawBones = bonesRawTraced.size();
		for (int i=0; i<nRawBones; i++){
			ofPolyline ithBone = bonesRawTraced[i];
			bonesRawMerged.push_back(ithBone);
		}
	
	} else {
	
		int tooShort = 4;
		vector<BoneMergeCouplet> coupletsOfMergedBones;
		coupletsOfMergedBones.clear();
		BoneMergeCouplet aCouplet;
		
		int nRawBones = bonesRawTraced.size();
		for (int i=0; i<nRawBones; i++){
			
			// For every long-enough bone
			ofPolyline ithBone = bonesRawTraced[i];
			int ithBoneSize = ithBone.size();
			if (ithBoneSize > tooShort){
				ofVec3f piS = ithBone[0]; // start
				ofVec3f piE = ithBone[ithBoneSize-1]; // end
				
				// Comparing with every too-short bone;
				for (int j=0; j<nRawBones; j++){
					if (j != i){
						
						// (Except for those which have already been merged)
						bool jthBoneAlreadyMerged = false;
						for (int k=0; k<coupletsOfMergedBones.size(); k++){
							BoneMergeCouplet kthCouplet = coupletsOfMergedBones[k];
							if (kthCouplet.boneJIndex == j){
								jthBoneAlreadyMerged = true;
							}
						}
						
						// Check to see if the Start or End of the long-enough bone
						// is exactly coincident with the Start or End of the too-short bone
						if (jthBoneAlreadyMerged == false){
							ofPolyline jthBone = bonesRawTraced[j];
							int jthBoneSize = jthBone.size();
							if ((jthBoneSize >= 2) && (jthBoneSize <= tooShort)){
								ofVec3f pjS = jthBone[0];
								ofVec3f pjE = jthBone[jthBoneSize-1];
								
								int distiEjS = (int) ofDistSquared(piE.x,piE.y, pjS.x,pjS.y);
								int distiEjE = (int) ofDistSquared(piE.x,piE.y, pjE.x,pjE.y);
								int distiSjS = (int) ofDistSquared(piS.x,piS.y, pjS.x,pjS.y);
								int distiSjE = (int) ofDistSquared(piS.x,piS.y, pjE.x,pjE.y);
								
								// If it is, then stash this couplet.
								if ((distiEjS <= 1) || (distiEjE <= 1) || (distiSjS <= 1) || (distiSjE <= 1) ){

									if        (distiEjS <= 1){
										aCouplet.boneITerminus	= BONE_END;
										aCouplet.boneJTerminus	= BONE_START;
									} else if (distiEjE <= 1) {
										aCouplet.boneITerminus	= BONE_END;
										aCouplet.boneJTerminus	= BONE_END;
									} else if (distiSjS <= 1){
										aCouplet.boneITerminus	= BONE_START;
										aCouplet.boneJTerminus	= BONE_START;
									} else if (distiSjE <= 1){
										aCouplet.boneITerminus	= BONE_START;
										aCouplet.boneJTerminus	= BONE_END;
									}
									
									aCouplet.boneIIndex	= i;
									aCouplet.boneJIndex	= j;
									coupletsOfMergedBones.push_back(aCouplet);
								}
								
							}
						}
					}
				}
			}
		}
		
		/*
		printf("--------------------\n");
		for (int k=0; k<coupletsOfMergedBones.size(); k++){
			BoneMergeCouplet kthCouplet = coupletsOfMergedBones[k];
			int i = kthCouplet.boneIIndex;
			int j = kthCouplet.boneJIndex;
			printf("%d	%d\n", i, j);
		}
		*/
		
		bonesRawMerged.clear();
		for (int i=0; i<nRawBones; i++){
			
			// For every long-enough bone
			ofPolyline ithBone = bonesRawTraced[i];
			int ithBoneSize = ithBone.size();
			if (ithBoneSize > tooShort){
				
				// check to see if it appears in a couplet.
				bool bIthBoneAppearsInACouplet = false;
				for (int k=0; k<coupletsOfMergedBones.size(); k++){
					BoneMergeCouplet kthCouplet = coupletsOfMergedBones[k];
					int iIndex = kthCouplet.boneIIndex;
					if (i == iIndex){
						bIthBoneAppearsInACouplet = true;
					}
				}
				
				if (bIthBoneAppearsInACouplet == false){
					// If it does not appear in a couplet, simply add it to bonesRawMerged
					bonesRawMerged.push_back(ithBone);
					
				} else {
					// If it does appear in a couplet, construct a merged polyline, then add to bonesRawMerged
					ofPolyline mergedBone;
					mergedBone.clear();
					bool bAddedIthBoneBulk = false;

					// First handle the J to I-start, if any
					for (int k=0; k<coupletsOfMergedBones.size(); k++){
						BoneMergeCouplet kthCouplet = coupletsOfMergedBones[k];
						if ((kthCouplet.boneIIndex == i) && (kthCouplet.boneITerminus == BONE_START)){
							int j = kthCouplet.boneJIndex;
							ofPolyline jthBone = bonesRawTraced[j];
							if (jthBone.size() >= 2) { // safety
							
								if (kthCouplet.boneJTerminus == BONE_START){
									// Now add all the points of jthBone, backwards from the end
									// Remember: the jthBone meets the ithBone at the jthBone's Start
									for (int p=(jthBone.size()-1); p>=0; p--){
										mergedBone.addVertex(jthBone[p]);
									}
								} else if (kthCouplet.boneJTerminus == BONE_END){
									// Now add all the points of jthBone, in order from the start
									// Remember: the jthBone meets the ithBone at the jthBone's End
									for (int p=0; p<jthBone.size(); p++){
										mergedBone.addVertex(jthBone[p]);
									}
								}
							}
							
							// Now add all the points of ithBone, in order from the start
							bAddedIthBoneBulk = true;
							for (int p=0; p<ithBone.size(); p++){
								mergedBone.addVertex(ithBone[p]);
							}
							break;
						}
					}
					
					// Now append the I-end to J, if any
					for (int k=0; k<coupletsOfMergedBones.size(); k++){
						BoneMergeCouplet kthCouplet = coupletsOfMergedBones[k];
						if ((kthCouplet.boneIIndex == i) && (kthCouplet.boneITerminus == BONE_END)){
							
							// Add all the points of ithBone, in order from the start, if we haven't already
							if (bAddedIthBoneBulk == false){
								for (int p=0; p<ithBone.size(); p++){
									mergedBone.addVertex(ithBone[p]);
								}
							}
							
							int j = kthCouplet.boneJIndex;
							ofPolyline jthBone = bonesRawTraced[j];
							if (jthBone.size() >= 2) { // safety
								if (kthCouplet.boneJTerminus == BONE_START){
									// Now add all the points of jthBone, backwards from the end
									// Remember: the jthBone meets the ithBone at the jthBone's Start
									for (int p=(jthBone.size()-1); p>=0; p--){
										mergedBone.addVertex(jthBone[p]);
									}
								} else if (kthCouplet.boneJTerminus == BONE_END){
									// Now add all the points of jthBone, in order from the start
									// Remember: the jthBone meets the ithBone at the jthBone's End
									for (int p=0; p<jthBone.size(); p++){
										mergedBone.addVertex(jthBone[p]);
									}
								}
							}
							break;
						}
					}
					
					bonesRawMerged.push_back(mergedBone);
					mergedBone.clear();
					
				}
			}
		}

		coupletsOfMergedBones.clear();
	}
}


//------------------------------------------------------------
void SkeletonTracer::smoothBones(){

	float small	= 12.0; //px or npts

	bonesRawSmooth.clear();
	int nBones = bonesRawMerged.size();
	for (int i=0; i<nBones; i++){
		ofPolyline rawBone = bonesRawMerged[i];
		float rawBoneLength = rawBone.getPerimeter();
		int nRawBonePoints = rawBone.size();
		
		if (nRawBonePoints > 3){
			if ((nRawBonePoints < small) || (rawBoneLength < small)){
				ofPolyline resampledBone = rawBone.getResampledByCount(5);
				bonesRawSmooth.push_back(resampledBone);
				
			} else {
				ofPolyline resampledBone = rawBone.getResampledBySpacing (boneResampling);
				// ofPolyline smoothedBone = resampledBone.getSmoothed (boneSmoothSigma);
				ofPolyline smoothedBone = getSmoothed (resampledBone);
				bonesRawSmooth.push_back(smoothedBone);
			}
			
		} else {
			bonesRawSmooth.push_back(rawBone);
		}
	}
}

//------------------------------------------------------------
ofPolyline SkeletonTracer::getSmoothed (ofPolyline inputBone){
	
	//---------------------------
	// Construct a gaussian kernel to smooth the polyline.
	int kernelW			= MIN(19,MAX(3,(boneSmoothKernW*2)+1)); // an odd number
	int kernelCenter	= (int)kernelW/2;
	int kLeft			= 0-kernelCenter;
	int kRight			= kernelCenter;
	float sigma			= MAX(0.01, boneSmoothSigma);
	float sum			= 0;
	
	float* kernel = new float[kernelW];
	for (int x=0; x<kernelW; ++x) {
		float val = (x-kernelCenter)/sigma;
		kernel[x] = exp(-0.5 * (val*val)) / (TWO_PI*sigma*sigma);
		sum += kernel[x];
	}
	for (int x=0; x<kernelW; ++x) {
		kernel[x] /= sum; // normalize the kernel values.
	}

	//---------------------------
	// Copy (weighted) points into tempBone
	tempBone.clear();
	int nPoints = inputBone.size();
	for (int i=0; i<nPoints; i++){
		ofPoint ithPoint;
		ithPoint.set(0,0);
		
		if (i==0){
			ithPoint = inputBone[0];
		} else if (i==(nPoints-1)){
			ithPoint = inputBone[nPoints-1];
		} else {
			for (int k=0; k<kernelW; k++){
				int j = MIN(MAX(0, i+k-kernelCenter),nPoints-1);
				ithPoint += (inputBone[j] * kernel[k]);
			}
		}
		tempBone.addVertex(ithPoint);
	}
	
	//---------------------------
	// Clean up and return.
	delete[] kernel; 
	return tempBone;
}


//------------------------------------------------------------
void SkeletonTracer::trackBones(){
	
}

//------------------------------------------------------------
void SkeletonTracer::optimallyReorderBones(){
	long then = ofGetElapsedTimeMicros();
	
	theOptimizedDrawing.clear();
	int nRawPolylines = bonesRawSmooth.size();
	if (bDoOptimizeTSP && (nRawPolylines < maxNBonesForTSP)){
		
		// Copy bonesRawSmooth (vector of ofPolylines)
		// into theRawDrawing (vector of PolylinePluses)
		theRawDrawing.clear();
		for (int i=0; i<nRawPolylines; i++){
			ofPolyline aPolyline = bonesRawSmooth[i];
			if (aPolyline.size() > 1){
				PolylinePlus aPolylinePlus;
				aPolylinePlus.polyline = aPolyline;
				aPolylinePlus.r = (liveColor & 0xFF0000) >> 16;
				aPolylinePlus.g = (liveColor & 0x00FF00) >>  8;
				aPolylinePlus.b = (liveColor & 0x0000FF)      ;
				theRawDrawing.push_back(aPolylinePlus);
			}
		}
		
		if (theRawDrawing.size() > 0){
			mySkeletonOptimizer.optimallyReorderBones(theRawDrawing, nOptimizePasses, bClosedTSP);
			theOptimizedDrawing = mySkeletonOptimizer.theOptimizedDrawing;
		}
		
	} else {
		
		// TSP is disabled, or maxNBonesForTSP is exceeded.
		// Copy smoothed into theOptimizedDrawing instead
		theRawDrawing.clear();
		for (int i=0; i<nRawPolylines; i++){
			ofPolyline aPolyline = bonesRawSmooth[i];
			if (aPolyline.size() > 1){
				PolylinePlus aPolylinePlus;
				aPolylinePlus.polyline = aPolyline;
				aPolylinePlus.r = (liveColor & 0xFF0000) >> 16;
				aPolylinePlus.g = (liveColor & 0x00FF00) >>  8;
				aPolylinePlus.b = (liveColor & 0x0000FF)      ;
				theOptimizedDrawing.push_back(aPolylinePlus);
			}
		}
	}
	
	long now = ofGetElapsedTimeMicros();
	float A = 0.96; float B = 1.0-A;
	tspElapsed = A*tspElapsed + B*(now-then);
	float optimizationAmount = mySkeletonOptimizer.optimizationAmount;
	printf("nP = %d	tspElapsed = %f		savings = %f \n", nRawPolylines, tspElapsed, optimizationAmount);
}



//------------------------------------------------------------
void SkeletonTracer::drawStateImage(){
	pixelStateImage.setFromPixels(pixelStateBuffer, buffer_w, buffer_h, OF_IMAGE_GRAYSCALE);
	pixelStateImage.draw(0,0, buffer_w, buffer_h);
}

//------------------------------------------------------------
void SkeletonTracer::drawBones(){
	
	int nPolylinePlusses = theOptimizedDrawing.size();
	for (int i=0; i<nPolylinePlusses; i++){
		PolylinePlus aPP = theOptimizedDrawing[i];
		ofPolyline aBone = aPP.polyline;
		if (aBone.size() >= 2){
			ofSetLineWidth(2.0);
			ofSetColor(aPP.r, aPP.g, aPP.b);
			aBone.draw();
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//=================================================================================
int SkeletonTracer::countUnlookedAtPixels (){
	int count = 0;
	int nPixels = buffer_w * buffer_h; // could be optimized later with bbox.
	const unsigned char testValue = (unsigned char) SKEL_COLOR_UNLOOKED_AT;
	for (int i=0; i<nPixels; i++){
		if (pixelStateBuffer[i] == testValue){
			count++;
		}
	}
	return count;
}

//------------------------------------------------------------
int SkeletonTracer::getLocOfFirstSkeletonPixel (){
	int result = SKEL_INVALID;

	int i = boundary0;
	while ((result == SKEL_INVALID) && (i<boundary1)){
		if (pixelStateBuffer[i] == SKEL_COLOR_UNLOOKED_AT){
			result = i;
		}
		i++;
	}
	return result;
}

//------------------------------------------------------------
void SkeletonTracer::markLocationWithState (int loc, SkelType state){
	unsigned char *img = pixelStateBuffer;
	switch (state){
		case SKEL_TYPE_EMPTY:			img[loc] = SKEL_COLOR_EMPTY;        break;
		case SKEL_TYPE_LOOKED_AT:		img[loc] = SKEL_COLOR_LOOKED_AT;    break;
		case SKEL_TYPE_BRANCH_POINT:	img[loc] = SKEL_COLOR_BRANCH_POINT; break;
		case SKEL_TYPE_UNLOOKED_AT:		img[loc] = SKEL_COLOR_UNLOOKED_AT;  break;
		case SKEL_TYPE_INVALID:			img[loc] = SKEL_COLOR_EMPTY;        break;
	}
}

//------------------------------------------------------------
SkelType SkeletonTracer::getTypeOfPixelAtLoc (int loc){
	SkelType result = SKEL_TYPE_INVALID;
	if ((loc >= boundary0) && (loc < boundary1)){
		unsigned char *pixels = pixelStateBuffer;
		unsigned char c = pixels[loc];
		switch(c){
			case SKEL_COLOR_LOOKED_AT:		result = SKEL_TYPE_LOOKED_AT;     break;
			case SKEL_COLOR_BRANCH_POINT:	result = SKEL_TYPE_BRANCH_POINT;  break;
			case SKEL_COLOR_UNLOOKED_AT:	result = SKEL_TYPE_UNLOOKED_AT;   break;
			case SKEL_COLOR_EMPTY:			result = SKEL_TYPE_EMPTY;         break;
		}
	}
	return result;
}

//------------------------------------------------------------
// returns loc of the first unlooked-at neighbor, counting clockwise.
int SkeletonTracer::getLocOfFirstUnlookedAtNeighbor (int loc){
	int result = SKEL_INVALID;
	if ((loc >= boundary0) && (loc < boundary1)){
		const int sw = buffer_w;
		if (getTypeOfPixelAtLoc(loc-sw+1 ) == SKEL_TYPE_UNLOOKED_AT) return (loc-sw+1 );
		if (getTypeOfPixelAtLoc(loc+1    ) == SKEL_TYPE_UNLOOKED_AT) return (loc+1    );
		if (getTypeOfPixelAtLoc(loc+sw+1 ) == SKEL_TYPE_UNLOOKED_AT) return (loc+sw+1 );
		if (getTypeOfPixelAtLoc(loc+sw   ) == SKEL_TYPE_UNLOOKED_AT) return (loc+sw   );
		if (getTypeOfPixelAtLoc(loc+sw-1 ) == SKEL_TYPE_UNLOOKED_AT) return (loc+sw-1 );
		if (getTypeOfPixelAtLoc(loc-1    ) == SKEL_TYPE_UNLOOKED_AT) return (loc-1    );
		if (getTypeOfPixelAtLoc(loc-sw-1 ) == SKEL_TYPE_UNLOOKED_AT) return (loc-sw-1 );
		if (getTypeOfPixelAtLoc(loc-sw   ) == SKEL_TYPE_UNLOOKED_AT) return (loc-sw   );
	}
	return result;
}

//------------------------------------------------------------
int SkeletonTracer::countUnlookedAtNeighbors (int loc){
	int result = 0;
	if ((loc >= boundary0) && (loc < boundary1)){
		const int sw = buffer_w;
		if (getTypeOfPixelAtLoc(loc-sw+1 ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc+1    ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc+sw+1 ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc+sw   ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc+sw-1 ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc-1    ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc-sw-1 ) == SKEL_TYPE_UNLOOKED_AT) result++;
		if (getTypeOfPixelAtLoc(loc-sw   ) == SKEL_TYPE_UNLOOKED_AT) result++;
	}
	return result;
}

//------------------------------------------------------------
bool SkeletonTracer::checkIfAnyNeighborsAreBranchPoints (int loc){
	bool result = false;
	if ((loc >= boundary0) && (loc < boundary1)){
		const int sw = buffer_w;
		if (getTypeOfPixelAtLoc(loc-sw+1 ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc+1    ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc+sw+1 ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc+sw   ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc+sw-1 ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc-1    ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc-sw-1 ) == SKEL_TYPE_BRANCH_POINT) return true;
		if (getTypeOfPixelAtLoc(loc-sw   ) == SKEL_TYPE_BRANCH_POINT) return true;
	}
	return result;
}

//------------------------------------------------------------
void SkeletonTracer::addPointToCurrentBoneAtPixelIndex(int currentLoc){
	float px = (float) (currentLoc % buffer_w);
	float py = (float) (currentLoc / buffer_w);
	currentBone.addVertex(px, py);
}

