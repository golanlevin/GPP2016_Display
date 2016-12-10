//
//  Skeletonizer.cpp
//  GPP2016_Display

#include "Skeletonizer.h"

//--------------------------------------------------------------
void Skeletonizer::initialize(int w, int h){
	
	skeletonBufW		= w;
	skeletonBufH		= h;
	nCurrentPositiveContours = 0;
	
	roiMinX = 0;
	roiMaxX = skeletonBufW -1;
	roiMinY = 0;
	roiMaxY = skeletonBufH -1;
	
	skeletonBuffer	= new unsigned char[skeletonBufW * skeletonBufH];
	tmpBuffer		= new unsigned char[skeletonBufW * skeletonBufH];
	inputBuff		= skeletonBuffer;
	
	skeletonImage.allocate     (skeletonBufW, skeletonBufH, OF_IMAGE_GRAYSCALE);
}

//--------------------------------------------------------------
void Skeletonizer::draw(){

	// 4. Draw the skeleton image.
	ofSetHexColor(0xffffff);
	float displayX = 0;//1*displayW + 2*displayM;
	float displayY = 0;//1*displayH + 2*displayM;
	skeletonImage.draw(displayX,displayY, skeletonBufW,skeletonBufH); // displayW,displayH);
	if (nCurrentPositiveContours > 0){
		ofSetHexColor(0x666666);
		ofNoFill();
		ofPushMatrix();
		ofTranslate(displayX,displayY);
		ofDrawRectangle(roiMinX, roiMinY, roiMaxX-roiMinX, roiMaxY-roiMinY);
		ofPopMatrix();
	}
}

//--------------------------------------------------------------
void Skeletonizer::clear(){
	int nPixels = skeletonBufW*skeletonBufH;
	memset(skeletonBuffer, (unsigned char)0, nPixels);
	skeletonImage.setFromPixels(skeletonBuffer, skeletonBufW,skeletonBufH, OF_IMAGE_GRAYSCALE);
	roiMinX = 0; roiMaxX = 1;
	roiMinY = 0; roiMaxY = 1;
}

//--------------------------------------------------------------
void Skeletonizer::computeSkeletonImageFromBlobs(cv::Mat &filledContourMat,
												 vector<vector<cv::Point>> &contours,
												 int nCurrentPosCon, int contourThickness,
												 int w, int h){
	
	nCurrentPositiveContours = nCurrentPosCon;
	skeletonBufW = w;
	skeletonBufH = h;
	
	//------------------------
	// Copy the blob pixels from filledContourMat into skeletonBuffer
	int nPixels = w * h;
	memcpy(skeletonBuffer, filledContourMat.data, nPixels);
	memset(tmpBuffer, (unsigned char)0, nPixels);
	inputBuff = skeletonBuffer;
	
	if (nCurrentPositiveContours == 0){
		// If there are no bodies present, don't bother searching.
		memset(skeletonBuffer, (unsigned char)0, nPixels);
		
	} else { // There are bodies present.
		
		//------------------------
		// Compute the bounds of the ROI, which is a rect containing all contours.
		// The ROI speeds up the skeletonization.
		roiMinX = INT_MAX; roiMaxX = INT_MIN;
		roiMinY = INT_MAX; roiMaxY = INT_MIN;
		int nContours = contours.size();
		for (int i=0; i<nContours; i++){ // Fill the positive contours
			vector<cv::Point> ithContour = contours[i];
			int nPoints = ithContour.size();
			for (int j=0; j<nPoints; j++){
				cv::Point jthPoint = ithContour[j];
				int jx = jthPoint.x;
				int jy = jthPoint.y;
				roiMinX = MIN(roiMinX, jx);
				roiMaxX = MAX(roiMaxX, jx);
				roiMinY = MIN(roiMinY, jy);
				roiMaxY = MAX(roiMaxY, jy);
			}
		}
		// Adding a small search margin to the ROI prevents an unpleasant artifact.
		int skeletonSearchMargin = contourThickness + 1;
		roiMinX -= skeletonSearchMargin;
		roiMinY -= skeletonSearchMargin;
		roiMaxX += skeletonSearchMargin;
		roiMaxY += skeletonSearchMargin;
		
		// Clamp the skeleton search ROI to the bounds of the image.
		roiMinX = MIN(w-1, MAX(roiMinX, 0));
		roiMinY = MIN(h-1, MAX(roiMinY, 0));
		roiMaxX = MIN(w-1, MAX(roiMaxX, 0));
		roiMaxY = MIN(h-1, MAX(roiMaxY, 0));
		
		//------------------------
		// Call the magic skeletonization.
		skeletonize();
	}
	
	skeletonImage.setFromPixels(skeletonBuffer, w,h, OF_IMAGE_GRAYSCALE);
}


//=============================================
void Skeletonizer::skeletonize() {
	
	// Shifting from 'Debug' to 'Release' targets doubles the speed of this.
	// It may be possible to optimize this further with Grand Central Dispatch:
	// https://forum.openframeworks.cc/t/parallel-computing/15010/6
	// See ../../resources/other/concurrency.zip file for an example.
	
	unsigned char table[]  = \
	{0,0,0,1,0,0,1,3,0,0,3,1,1,0,1,3,0,0,0,0,0,0,0,0,2,0,2,0,3,0,3,3, \
		0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,3,0,2,2, \
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
		2,0,0,0,0,0,0,0,2,0,0,0,2,0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,3,0,2,0, \
		0,0,3,1,0,0,1,3,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1, \
		3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
		2,3,1,3,0,0,1,3,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
		2,3,0,1,0,0,0,1,0,0,0,0,0,0,0,0,3,3,0,1,0,0,0,0,2,2,0,0,2,0,0,0};
	/* magic */
	
	int sw = skeletonBufW;
	int sh = skeletonBufH;
	int sn = sw*sh;
	unsigned char* pixels = inputBuff;
	
	// Black the outermost edges of the input buffer, to prevent an ugly artifact.
	for (int i=0; i<sw; i++){
		pixels[sn-sw+i] = pixels[i] = 0; }
	for (int i=0; i<sh; i++){
		pixels[i*sw + (sw-1)] = pixels[i*sw] = 0; }
	
	long then = ofGetElapsedTimeMicros();
	
	// Do the iterative, thinning-based skeletonization.
	int pass = 0;
	int pixelsRemoved = 0;
	do {
		pixelsRemoved = thin (pass++, table);
	} while (pixelsRemoved > 0);
	
	// Compute how much time that took.
	long now = ofGetElapsedTimeMicros();
	float elapsed = (now - then);
	float A = 0.95; float B = 1.0-A;
	skeletonizationDuration = A*skeletonizationDuration + B*elapsed;
}


//=============================================
inline int Skeletonizer::thin (int pass, unsigned char *table) {
	
	const int sw   = skeletonBufW;
	const int sh   = skeletonBufH;
	const int xMin = roiMinX;
	const int xMax = roiMaxX;
	const int yMin = roiMinY;
	const int yMax = roiMaxY;
	
	memcpy (tmpBuffer, inputBuff, skeletonBufW*skeletonBufH);
	
	unsigned char*	pixels  = inputBuff;
	unsigned char*	pixels2 = tmpBuffer;
	
	int offset;
	int pixelsRemoved = 0;
	const int rowOffset = sw;
	const int rowOffsetp1 = rowOffset+1;
	unsigned char index;
	unsigned char *ptr;
	unsigned char v;
	
	const bool b_oddPass = (pass & 1);
	const unsigned char testb = (b_oddPass) ? 2:1;
	
	int y,x;
	int yswpxmin = yMin*sw + xMin;
	int offsetmRowoffsetp1;
	offset = yswpxmin;
	
	for (y=yMin; y<yMax; y++) {
		offset = yswpxmin;
		offsetmRowoffsetp1 = offset - rowOffsetp1;
		yswpxmin += sw;
		
		for (x=xMin; x<xMax; x++,offsetmRowoffsetp1++) {
			if ((v = pixels2[offset])) { // yes, assignment.
				ptr = &pixels2[offsetmRowoffsetp1];
				index = 0;
				if (*ptr++){ index |= 1;  }
				if (*ptr++){ index |= 2;  }
				if (*ptr)  { index |= 4;  } ptr+=rowOffset;
				if (*ptr)  { index |= 8;  } ptr+=rowOffset;
				if (*ptr--){ index |= 16; }
				if (*ptr--){ index |= 32; }
				if (*ptr)  { index |= 64; } ptr-=rowOffset;
				if (*ptr)  { index |= 128;}
				
				if ((table[index])&testb){
					v = 0;
					pixelsRemoved++;
				}
			}
			pixels[offset++] = v;
		}
	}
	
	return pixelsRemoved;
}
