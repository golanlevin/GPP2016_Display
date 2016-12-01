#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
void ofApp::setup(){
	
	proxyCaptureW = 320;
	proxyCaptureH = 240;
	displayScale = 1.0;
	displayW = proxyCaptureW * displayScale;
	displayH = proxyCaptureH * displayScale;
	displayM = 10; 
	
	/*
	1_Fernanda-Cascada_Input.mov
	2_Lewis-Cascada_Input.mov
	3_Group-Cascada_Input.mov
	*/

	bUseProxyVideoInsteadOfOSC	= true;
	bProxyVideoPlayerPaused		= false;
	proxyVideoPlayer.load("proxy/3_Group-Cascada_Input.mov");
	proxyVideoPlayer.play();
	proxyVideoPlayer.setLoopState(OF_LOOP_NORMAL);
	proxyColorImage.allocate (proxyCaptureW,proxyCaptureH);
	proxyColorImage.set(0);
	
	minBlobSize			= 20;
	maxBlobSize			= (proxyCaptureW*proxyCaptureH)/3;
	maxNBlobs			= 8;
	bGetHoles			= true;
	
	skeletonBufW		= 320; // Note: the Kinectv2 depth image is 512 x 424.
	skeletonBufH		= 240; // 320x265 would preserve the aspect ratio of Kinectv2 depth image.
	skeletonScale		= (float)skeletonBufW/(float)proxyCaptureW;
	bScaleProxyToKinect	= true;
	skeletonizationDuration = 0;
	
	filledContourMat.create    (skeletonBufH, skeletonBufW, CV_8UC(1));
	filledContourImage.allocate(skeletonBufW, skeletonBufH, OF_IMAGE_GRAYSCALE);
	skeletonImage.allocate     (skeletonBufW, skeletonBufH, OF_IMAGE_GRAYSCALE);
	mySkeletonTracer.initialize(skeletonBufW, skeletonBufH);
	
	bufW = skeletonBufW;
	bufH = skeletonBufH;
	roiMinX = 0;
	roiMaxX = skeletonBufW -1;
	roiMinY = 0;
	roiMaxY = skeletonBufH -1;
	skeletonBuffer	= new unsigned char[bufW * bufH];
	tmpBuffer		= new unsigned char[bufW * bufH];
	inputBuff		= skeletonBuffer;

	
	
	myOfxCvContourFinder.setMinArea(minBlobSize);
	myOfxCvContourFinder.setMaxArea(maxBlobSize);
	
	inputGuiPanel.setup("Settings", "settings/GPPSettings.xml", displayM, (displayM*3)+(displayH*2));
	inputGuiPanel.add(proxyThreshold.setup		("proxyThreshold",		80, 0,254));
	inputGuiPanel.add(inputLineSmoothing.setup	("inputLineSmoothing",	5.0, 0.0, 16.0));
	inputGuiPanel.add(inputLineResample.setup	("inputLineResample",	3.0, 1.0, 11.0));
	inputGuiPanel.add(contourThickness.setup	("contourThickness",	2, 0,8));
	inputGuiPanel.add(bSmoothHolesToo.setup		("bSmoothHolesToo",		false));

	inputGuiPanel.add(mySkeletonTracer.boneResampling.setup		("boneResampling",		2.5, 1.0, 11.0));
	inputGuiPanel.add(mySkeletonTracer.boneSmoothSigma.setup	("boneSmoothSigma",		0.9, 0.0, 3.0));
	inputGuiPanel.add(mySkeletonTracer.boneSmoothKernW.setup	("boneSmoothKernW",		2, 1, 7));
	
}


//--------------------------------------------------------------
void ofApp::update(){
	
	
	// Fetch the raw contours, either from OSC or from a local test video.
	vector<vector<cv::Point>> rawContours = obtainRawContours();
	
	// Filter the contours, smoothing them to produce better skeletons.
	filterContoursBeforeReconstitution (rawContours);
	
	// Prepare a blob image, the input to thinning-based skeletonization.
	reconstituteBlobsFromContours (theContoursi, skeletonBufW,skeletonBufH);
	computeSkeletonImageFromBlobs (theContoursi, skeletonBufW,skeletonBufH);
	
	mySkeletonTracer.computeVectorSkeleton (skeletonBuffer);
	
	// Trace skeleton bones
	// Use ofxCv RectTracker to determine bone identity over time
	// Blur current bones over time
	// Record  bones & play back; add to overall collection of bones (new + old)
	// Perspectival quad warp coordinates
	// Compute order with which to render bones
	// Send bones to laser

}

//--------------------------------------------------------------
vector<vector<cv::Point>> ofApp::obtainRawContours(){
	
	vector<vector<cv::Point>> rawContours;
	if (bUseProxyVideoInsteadOfOSC){
		computeContoursFromProxyVideo();
		rawContours = myOfxCvContourFinder.getContours();
	} else {
		// rawContours = something fetched from OSC
		// Note that it will be necessary for these to match ofxCv contours,
		// For example, indication about whether a contour is a hole or not.
	}
	return rawContours;
}

//--------------------------------------------------------------
void ofApp::reconstituteBlobsFromContours (vector<vector<cv::Point>> contours, int w, int h){
	// Given a collection of contours,
	// Whether obtained over OSC from a Kinectv2,
	// or computed from a local proxy video, use flood fill
	// to reconstitute binary image blobs from those contours
	// (for further processing by a skeletonizer).
	int nContours = contours.size();
	
	//Fill the contours in filledContourMat using openCV drawContours
	filledContourMat.setTo(0);
	for (int i=0; i<nContours; i++){ // Fill the positive contours
		if (myOfxCvContourFinder.getHole(i) == false) {
			cv::drawContours(filledContourMat, contours, i,255, CV_FILLED);
			if (contourThickness > 0){
				cv::drawContours(filledContourMat, contours, i,255, contourThickness);
			}
		}
	}
	for (int i=0; i<nContours; i++){ // Fill the negative contours
		if (myOfxCvContourFinder.getHole(i) == true) {
			cv::drawContours(filledContourMat, contours, i,0, CV_FILLED);
			if (contourThickness > 0){
				cv::drawContours(filledContourMat, contours, i,0, contourThickness);
			}
		}
	}
	
	// Copy over those pixels into filledContourImage, for drawing etc.
	unsigned char* filledContourMatPixels;
	filledContourMatPixels = filledContourMat.data;
	filledContourImage.setFromPixels(filledContourMatPixels, w,h, OF_IMAGE_GRAYSCALE);
}

//--------------------------------------------------------------
void ofApp::computeSkeletonImageFromBlobs(vector<vector<cv::Point>> contours, int w, int h){
	
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
void ofApp::skeletonize() {
	
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
		2,3,0,1,0,0,0,1,0,0,0,0,0,0,0,0,3,3,0,1,0,0,0,0,2,2,0,0,2,0,0,0};  /* magic */
	
	int sw = bufW;
	int sh = bufH;
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
	float A = 0.925; float B = 1.0-A;
	skeletonizationDuration = A*skeletonizationDuration + B*elapsed;
	// printf("skeletonizationDuration = %f\n", skeletonizationDuration);
}

//=============================================
inline int ofApp::thin (int pass, unsigned char *table) {
	
	const int sw   = bufW;
	const int sh   = bufH;
	const int xMin = roiMinX;
	const int xMax = roiMaxX;
	const int yMin = roiMinY;
	const int yMax = roiMaxY;
	
	memcpy (tmpBuffer, inputBuff, bufW*bufH);
	
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


//--------------------------------------------------------------
void ofApp::filterContoursBeforeReconstitution(vector<vector<cv::Point>> rawContours){
	int nContours = rawContours.size();
	
	// Count the positive contours: presumably, bodies.
	nCurrentPositiveContours = 0;
	for (int i=0; i<nContours; i++){ // Fill the positive contours
		if (myOfxCvContourFinder.getHole(i) == false) {
			nCurrentPositiveContours++;
		}
	}
	
	//----------------------
	// Copy the raw (int) contours into temp (float / ofPolyline) versions.
	vector<ofPolyline> tempPolylines;
	tempPolylines.clear();
	for (int i=0; i<nContours; i++){
		vector<cv::Point> ithRawContour = rawContours[i];
		ofPolyline ithNewPolyline;
		int nPoints = ithRawContour.size();
		for (int j=0; j<nPoints; j++){
			cv::Point jthRawPoint = ithRawContour[j];
			float jxf = (float)jthRawPoint.x;
			float jyf = (float)jthRawPoint.y;
			if (bScaleProxyToKinect){
				jxf *= skeletonScale;
				jyf *= skeletonScale;
			}

			ithNewPolyline.addVertex(jxf, jyf);
		}
		tempPolylines.push_back(ithNewPolyline);
	}
	
	//----------------------
	// Filter the input polylines to optimize later skeletonization: simplify, resample, smooth
	theContoursf.clear();
	bool bSimplifyInputPolylines = true;
	for (int i=0; i<nContours; i++){
		ofPolyline ithTempPolyline = tempPolylines[i];
		ithTempPolyline.setClosed(true);
		if (bSimplifyInputPolylines) {
			ithTempPolyline.simplify(); }
		if (inputLineResample > 1.0) {
			ithTempPolyline = ithTempPolyline.getResampledBySpacing(inputLineResample); }
		if (bSmoothHolesToo || (myOfxCvContourFinder.getHole(i) == false)){
			if (inputLineSmoothing > 0){
				ithTempPolyline = ithTempPolyline.getSmoothed(inputLineSmoothing); }
		}
		theContoursf.push_back(ithTempPolyline);
	}
	
	//----------------------
	// Copy the modified (float) contours into new (int) contours.
	tempPolylines.clear();
	theContoursi.clear();
	for (int i=0; i<nContours; i++){
		ofPolyline ithNewPolyline = theContoursf[i];
		vector<cv::Point> ithNewContour;
		int nPoints = ithNewPolyline.size();
		for (int j=0; j<nPoints; j++){
			ofVec2f jthNewPoint = ithNewPolyline[j];
			int jxi = (int)round(jthNewPoint.x);
			int jyi = (int)round(jthNewPoint.y);
			ithNewContour.push_back(cv::Point(jxi,jyi));
		}
		theContoursi.push_back(ithNewContour);
	}
}

//--------------------------------------------------------------
void ofApp::computeContoursFromProxyVideo(){
	bool bNewFrame = false;
	proxyVideoPlayer.update();
	bNewFrame = proxyVideoPlayer.isFrameNew();
	if (bNewFrame){
		proxyColorImage.setFromPixels(proxyVideoPlayer.getPixels());
	}
	
	ofColor contourFinderTargetColor = ofColor(255,255,255);
	myOfxCvContourFinder.setTargetColor(contourFinderTargetColor, TRACK_COLOR_RGB);
	myOfxCvContourFinder.setThreshold(proxyThreshold);
	myOfxCvContourFinder.setSortBySize(true);
	myOfxCvContourFinder.setFindHoles(bGetHoles);
	myOfxCvContourFinder.findContours(proxyColorImage);
	
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(100,100,100);
	
	float displayX, displayY;

	// 1. Draw the incoming image
	ofSetHexColor(0xffffff);
	displayX = 0*displayW + 1*displayM;
	displayY = 0*displayH + 1*displayM;
	proxyColorImage.draw(displayX,displayY, displayW,displayH);

	// 2. Draw the contours:
	ofFill();
	ofSetHexColor(0x000000);
	displayX = 0*displayW + 1*displayM;
	displayY = 1*displayH + 2*displayM;
	ofDrawRectangle(displayX,displayY, displayW,displayH);
	ofSetHexColor(0xffffff);
	ofPushMatrix();
	ofTranslate(displayX,displayY);
	ofScale(displayScale,displayScale);
	myOfxCvContourFinder.draw();
	ofPopMatrix();
	
	// 3. Draw the blobs reconstituted from the contours, and their combined bbox.
	ofSetHexColor(0xffffff);
	displayX = 1*displayW + 2*displayM;
	displayY = 0*displayH + 1*displayM;
	filledContourImage.draw(displayX,displayY, displayW,displayH);
	
	// 4. Draw the skeleton image.
	ofSetHexColor(0xffffff);
	displayX = 1*displayW + 2*displayM;
	displayY = 1*displayH + 2*displayM;
	skeletonImage.draw(displayX,displayY, skeletonBufW,skeletonBufH); // displayW,displayH);
	if (nCurrentPositiveContours > 0){
		ofSetHexColor(0x666666);
		ofNoFill();
		ofPushMatrix();
		ofTranslate(displayX,displayY);
		ofDrawRectangle(roiMinX, roiMinY, roiMaxX-roiMinX, roiMaxY-roiMinY);
		ofPopMatrix();
	}
	
	// 5. Draw the skeleton-tracer state image.
	ofSetHexColor(0xffffff);
	displayX = 1*displayW + 2*displayM;
	displayY = 2*displayH + 3*displayM;
	ofPushMatrix();
	ofTranslate(displayX,displayY);
	mySkeletonTracer.drawStateImage();
	ofPopMatrix();
	
	// 6. Draw the bones.
	ofPushMatrix();
	displayX = 2*displayW + 3*displayM;
	displayY = 0*displayH + 1*displayM;
	ofTranslate(displayX,displayY);
	ofScale(3.0,3.0);
	ofSetHexColor(0x202020);
	proxyColorImage.draw(0,0, displayW,displayH);
	mySkeletonTracer.drawBones();
	ofPopMatrix();
	
	
	/*
	// finally, a report:
	ofSetHexColor(0xffffff);
	stringstream reportStr;
	reportStr << "fps: " << ofGetFrameRate();
	ofDrawBitmapString(reportStr.str(), 20, 600);
	*/
	

	inputGuiPanel.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key){
		case ' ':
			if (bUseProxyVideoInsteadOfOSC){
				bProxyVideoPlayerPaused = !bProxyVideoPlayerPaused;
				proxyVideoPlayer.setPaused(bProxyVideoPlayerPaused);
			}
			break;
			
		case ',':
			proxyVideoPlayer.previousFrame();
			break;
		case '.':
			proxyVideoPlayer.nextFrame();
			break;
			
		case 'S':
			inputGuiPanel.saveToFile("settings/GPPSettings.xml");
			break;
		case 'L':
			inputGuiPanel.loadFromFile("settings/GPPSettings.xml");
			break;
		case 'E':
			mySkeletonTracer.exportVectorGraphics();
			break;
	}
}



//--------------------------------------------------------------
void ofApp::keyReleased(int key){}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){}
