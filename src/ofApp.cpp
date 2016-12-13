#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
void ofApp::setup(){

    
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    

    receiver.setup(6667);
    
	proxyCaptureW = 320;
	proxyCaptureH = 240;
	displayScale = 0.75;
	displayW = proxyCaptureW * displayScale;
	displayH = proxyCaptureH * displayScale;
	displayM = 8;
	
	initializeGui();
	
	proxyVideoFilenames = new string[3];
	proxyVideoFilenames[0] = "proxy/1_Fernanda-Cascada_Input.mov";
	proxyVideoFilenames[1] = "proxy/2_Lewis-Cascada_Input.mov";
	proxyVideoFilenames[2] = "proxy/3_Group-Cascada_Input.mov";

	proxyVideoPlayer.load(proxyVideoFilenames[2]);
	proxyVideoPlayer.play();
	proxyVideoPlayer.setLoopState(OF_LOOP_NORMAL);
	proxyColorImage.allocate (proxyCaptureW,proxyCaptureH);
	proxyColorImage.set(0);
	
	
	bUseProxyVideoInsteadOfOSC	= true;
	bProxyVideoPlayerPaused		= false;
	bGotAProxyFrame				= false;
	
	minBlobSize			= 20;
	maxBlobSize			= (proxyCaptureW*proxyCaptureH)/3;
	maxNBlobs			= 8;
	bGetHoles			= true;
	
	myOfxCvContourFinder.setMinArea(minBlobSize);
	myOfxCvContourFinder.setMaxArea(maxBlobSize);
	
	kinectV2DepthW		= 512;
	kinectV2DepthH		= 424;
	
	skeletonBufW		= 320; // Note: the Kinectv2 depth image is 512 x 424.
	skeletonBufH		= 240; // 320x265 would preserve the aspect ratio of Kinectv2 depth image.
	skeletonScale		= (float)skeletonBufW/(float)proxyCaptureW;
	bScaleProxyToKinect	= true;
	
	filledContourMat.create    (skeletonBufH, skeletonBufW, CV_8UC(1));
	filledContourImage.allocate(skeletonBufW, skeletonBufH, OF_IMAGE_GRAYSCALE);
	
	mySkeletonTracer = new SkeletonTracer();
	mySkeletonTracer->initialize(skeletonBufW, skeletonBufH);
	mySkeletonizer.initialize(skeletonBufW, skeletonBufH);

	
	//---------------------------------
	mySkeletonLoaderSaver = new SkeletonLoaderSaver();
	mySkeletonLoaderSaver->initialize(skeletonBufW, skeletonBufH);
	
	mySkeletonDisplayer.initialize(skeletonBufW, skeletonBufH);
	mySkeletonDisplayer.givePointers(mySkeletonTracer, mySkeletonLoaderSaver);
	
	
}

//--------------------------------------------------------------
void ofApp::exit(){
	/* mySkeletonLoaderSaver->stopThread(); */
}


//--------------------------------------------------------------
void ofApp::initializeGui(){
    
    ofxGuiSetDefaultWidth(displayW);
    inputGuiPanel.setup("Settings", "settings/GPPSettings.xml", displayM, (displayM*3)+(displayH*2));
    
    inputGuiPanel.add(proxyThreshold.setup		("proxyThreshold",		80, 0,254));
    inputGuiPanel.add(inputLineSmoothing.setup	("inputLineSmoothing",	5.0, 0.0, 16.0));
    inputGuiPanel.add(inputLineResample.setup	("inputLineResample",	3.0, 1.0, 11.0));
    inputGuiPanel.add(contourThickness.setup	("contourThickness",	2, 0,8));
    inputGuiPanel.add(bSmoothHolesToo.setup		("bSmoothHolesToo",		false));
	inputGuiPanel.add(bDrawGrayProxy.setup		("bDrawGrayProxy",		true));
    
    inputGuiPanel.add(boneResampling.setup		("boneResampling",		3.0, 1.0, 11.0));
    inputGuiPanel.add(boneSmoothSigma.setup		("boneSmoothSigma",		0.9, 0.0, 3.0));
    inputGuiPanel.add(boneSmoothKernW.setup		("boneSmoothKernW",		2, 1, 7));
    inputGuiPanel.add(bDoMergeBones.setup		("bDoMergeBones",		true));
    inputGuiPanel.add(bDoOptimizeTSP.setup		("bDoOptimizeTSP",		true));
    inputGuiPanel.add(bClosedTSP.setup			("bClosedTSP",			false));
    inputGuiPanel.add(maxNBonesForTSP.setup		("maxNBonesForTSP",		60, 20,300));
    // inputGuiPanel.add(nOptimizePasses.setup		("nOptimizePasses",		2, 1, 5));
}


//--------------------------------------------------------------
void ofApp::propagateGui(){
    mySkeletonTracer->boneResampling	= (float)	boneResampling;
    mySkeletonTracer->boneSmoothSigma	= (float)	boneSmoothSigma;
    mySkeletonTracer->boneSmoothKernW	= (int)		boneSmoothKernW;
    mySkeletonTracer->bDoMergeBones		= (bool)	bDoMergeBones;
	
	mySkeletonDisplayer.bDoOptimizeTSP	= (bool)	bDoOptimizeTSP;
    mySkeletonDisplayer.bClosedTSP		= (bool)	bClosedTSP;
    mySkeletonDisplayer.maxNBonesForTSP	= (int)		maxNBonesForTSP;
}


//--------------------------------------------------------------
void ofApp::captureProxyVideo(){
    if (bUseProxyVideoInsteadOfOSC){
        bool bNewFrame = false;
        proxyVideoPlayer.update();
        bNewFrame = proxyVideoPlayer.isFrameNew();
        if (bNewFrame){
            proxyColorImage.setFromPixels(proxyVideoPlayer.getPixels());
            bGotAProxyFrame = true;
        } else {
			; // Don't do nothin'
        }
    }
}


//--------------------------------------------------------------
void ofApp::update(){
    
    propagateGui();
    captureProxyVideo();
    
    // Fetch the live raw contours: either from over OSC, or from a local proxy video.
    vector<vector<cv::Point>> rawContours = obtainRawContours();
    
    // Process the incoming contours (regardless of their source of origin)
    int nRawContours = rawContours.size();
    if (nRawContours > 0){
		
		// Filter the contours, smoothing them to yield better skeletons.
		// Then prepare a blob image: our input to thinning-based skeletonization.
        // (There are minor differences if we're getting data from OSC or proxy video.)
        if (bUseProxyVideoInsteadOfOSC){
            filterContoursBeforeReconstitution (rawContours);
            reconstituteBlobsFromContours (theContoursi, skeletonBufW,skeletonBufH);
        } else {
            filterContoursBeforeReconstitutionOSC (rawContours);
            reconstituteBlobsFromContoursOSC (theContoursi, skeletonBufW,skeletonBufH);
        }
		
        // Compute a skeleton image: pixels representing the live skeletons.
        mySkeletonizer.computeSkeletonImageFromBlobs(filledContourMat, theContoursi,
                                                     nCurrentPositiveContours, contourThickness,
                                                     skeletonBufW,skeletonBufH);
        
        // Trace the (polyline) skeleton bones from the (pixel) skeleton image.
        mySkeletonTracer->computeVectorSkeleton (mySkeletonizer.skeletonBuffer, nRawContours);
		
		// If mySkeletonLoaderSaver is recording, add in the most recent frame
		if (mySkeletonLoaderSaver->isRecording()){
			vector<PolylinePlus> &theRawDrawing = mySkeletonTracer->theRawDrawing;
			mySkeletonLoaderSaver->addFrameToCurrentRecording(theRawDrawing);
		}
		
		
    } else {
        // There are no incoming contours, so handle that.
        handleAbsenceOfIncomingContours();
    }
	
	
	mySkeletonDisplayer.update(); 
    
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
        // DAN MOORE CODE GOES HERE
        // rawContours = something something fetched from OSC
        // Note that it is necessary for these to match ofxCv contours,
        // For example: indicating whether a contour is a hole or not.
		
        rawContours.clear();
        isHole.clear();
		
        while(receiver.hasWaitingMessages()){
            // get the next message
            ofxOscMessage m;
            receiver.getNextMessage(m);
            if(m.getAddress() == "/blob"){
                vector<cv::Point> points;
                ofBuffer buffer = m.getArgAsBlob(0);
                int size = buffer.size()/sizeof(cv::Point);
                char *tempData = buffer.getData();
                points.resize(size);
                memcpy(&points[0], tempData, buffer.size());
                rawContours.push_back(points);
                isHole.push_back(m.getArgAsInt(1));
            }
        }
    }
    return rawContours;
}

//--------------------------------------------------------------
void ofApp::handleAbsenceOfIncomingContours(){
    // What to do when there are no incoming (live) contours.
    // Blank the stuff that needs to be blanked.
    mySkeletonizer.clear();
    filledContourMat.setTo(0);
    filledContourImage.setFromPixels(filledContourMat.data, skeletonBufW,skeletonBufH, OF_IMAGE_GRAYSCALE);
    mySkeletonTracer->computeVectorSkeleton (mySkeletonizer.skeletonBuffer, 0);
}

//--------------------------------------------------------------
void ofApp::reconstituteBlobsFromContours (vector<vector<cv::Point>> &contours, int w, int h){
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
void ofApp::reconstituteBlobsFromContoursOSC (vector<vector<cv::Point>> &contours, int w, int h){
    // Given a collection of contours,
    // Whether obtained over OSC from a Kinectv2,
    // or computed from a local proxy video, use flood fill
    // to reconstitute binary image blobs from those contours
    // (for further processing by a skeletonizer).
    int nContours = contours.size();
    
    //Fill the contours in filledContourMat using openCV drawContours
    filledContourMat.setTo(0);
    for (int i=0; i<nContours; i++){ // Fill the positive contours
        if (isHole[i] == 0) {
            cv::drawContours(filledContourMat, contours, i,255, CV_FILLED);
            if (contourThickness > 0){
                cv::drawContours(filledContourMat, contours, i,255, contourThickness);
            }
        }
    }
    for (int i=0; i<nContours; i++){ // Fill the negative contours
        if (isHole[i] == 1) {
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
void ofApp::filterContoursBeforeReconstitution(vector<vector<cv::Point>> &rawContours){
    theContoursf.clear();
    theContoursi.clear();
    
    int nContours = rawContours.size();
    if (nContours > 0){
        
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
        // theContoursf.clear();
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
                    ithTempPolyline = ithTempPolyline.getSmoothed(inputLineSmoothing);
                }
            }
            theContoursf.push_back(ithTempPolyline);
        }
        
        //----------------------
        // Copy the modified (float) contours into new (int) contours.
        // theContoursi.clear();
        tempPolylines.clear();
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
}


//--------------------------------------------------------------
void ofApp::filterContoursBeforeReconstitutionOSC(vector<vector<cv::Point>> &rawContours){
    theContoursf.clear();
    theContoursi.clear();
    
    int nContours = rawContours.size();
    if (nContours > 0){
        
        // Count the positive contours: presumably, bodies.
        nCurrentPositiveContours = 0;
        for (int i=0; i<nContours; i++){ // Fill the positive contours
            if (isHole[i] == 0) {
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
				
				// if (bScaleProxyToKinect){
				jxf *= (float) skeletonBufW / (float) kinectV2DepthW; //(320.0/512.0);
				jyf *= (float) skeletonBufH / (float) kinectV2DepthH; //(320.0/512.0);
				// }
                
                ithNewPolyline.addVertex(jxf, jyf);
            }
            tempPolylines.push_back(ithNewPolyline);
        }
        
        //----------------------
        // Filter the input polylines to optimize later skeletonization: simplify, resample, smooth
        // theContoursf.clear();
        bool bSimplifyInputPolylines = true;
        for (int i=0; i<nContours; i++){
            ofPolyline ithTempPolyline = tempPolylines[i];
            ithTempPolyline.setClosed(true);
            if (bSimplifyInputPolylines) {
                ithTempPolyline.simplify(); }
            if (inputLineResample > 1.0) {
                ithTempPolyline = ithTempPolyline.getResampledBySpacing(inputLineResample); }
            if (bSmoothHolesToo || (isHole[i] == 0)){
                if (inputLineSmoothing > 0){
                    ithTempPolyline = ithTempPolyline.getSmoothed(inputLineSmoothing); }
            }
            theContoursf.push_back(ithTempPolyline);
        }
        
        //----------------------
        // Copy the modified (float) contours into new (int) contours.
        // theContoursi.clear();
        tempPolylines.clear();
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
}

//--------------------------------------------------------------
void ofApp::computeContoursFromProxyVideo(){
    
    if (bGotAProxyFrame){
        ofColor contourFinderTargetColor = ofColor(255,255,255);
        myOfxCvContourFinder.setTargetColor(contourFinderTargetColor, TRACK_COLOR_RGB);
        myOfxCvContourFinder.setThreshold(proxyThreshold);
        myOfxCvContourFinder.setSortBySize(true);
        myOfxCvContourFinder.setFindHoles(bGetHoles);
        myOfxCvContourFinder.findContours(proxyColorImage);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(0,0,0);
	ofFill();
	ofSetColor(100,100,100);
	ofDrawRectangle(0,0,1280,720);
	// ofBackground(100,100,100);

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
	
	
    // 3. Draw the blobs reconstituted from the contours, and their combined bounding box.
    ofSetHexColor(0xffffff);
    displayX = 1*displayW + 2*displayM;
    displayY = 0*displayH + 1*displayM;
    filledContourImage.draw(displayX,displayY, displayW,displayH);
	if (mySkeletonLoaderSaver->isRecording()){
		ofFill();
		ofSetColor(255,0,0);
		ofDrawRectangle(displayX+5,displayY+5, 10,10);
		int nCurrRecFrames = mySkeletonLoaderSaver->getCurrentRecordingLength();
		ofDrawBitmapString( ofToString(nCurrRecFrames), displayX+20, displayY+15);
	}
	
	
    // 4. Draw the skeleton image.
    displayX = 1*displayW + 2*displayM;
    displayY = 1*displayH + 2*displayM;
    ofPushMatrix();
    ofTranslate(displayX,displayY);
	ofScale(displayScale,displayScale);
    mySkeletonizer.draw();
    ofPopMatrix();
    int durMicros = (int)(mySkeletonizer.skeletonizationDuration);
    ofSetColor(255,255,0);
    ofDrawBitmapString( "Skl: " + ofToString(durMicros) + " us", displayX+5,displayY+16);
	
	
    // 5. Draw the skeleton-tracer state image.
    ofSetHexColor(0xffffff);
    displayX = 1*displayW + 2*displayM;
    displayY = 2*displayH + 3*displayM;
    ofPushMatrix();
    ofTranslate(displayX,displayY);
	ofScale(displayScale,displayScale);
    mySkeletonTracer->drawStateImage();
    ofPopMatrix();
    int tspMicros	= (int)(mySkeletonDisplayer.tspElapsed);
	int nTspBones	= mySkeletonDisplayer.nCombinedPolylinePluses;
	int optim		= (int)(100.0 * mySkeletonDisplayer.optimizationAmount);
    ofSetColor(255,255,0);
    ofDrawBitmapString( "TSP: " + ofToString(tspMicros) + " us", displayX+5,displayY+16);
	ofDrawBitmapString( "#PP: " + ofToString(nTspBones)        , displayX+5,displayY+30);
	ofDrawBitmapString( "%Op: " + ofToString(optim)		+ "%"  , displayX+5,displayY+44);
    
    // 6. Draw the bones.
    ofPushMatrix();
    displayX = 2*displayW + 3*displayM;
    displayY = 0*displayH + 1*displayM;
    ofTranslate(displayX,displayY);
    ofScale(3.2, 3.2);
	if (bDrawGrayProxy){
		ofSetHexColor(0x202020);
		filledContourImage.draw(0,0, displayW,displayH);
		// proxyColorImage.draw(0,0, displayW,displayH);
	} else {
		ofFill();
		ofSetColor(0,0,0);
		ofDrawRectangle(0,0, displayW,displayH);
	}
	
	ofScale(displayScale, displayScale);
	mySkeletonDisplayer.bShowPathBetweenBones = true;
	mySkeletonDisplayer.renderToScreen();
	
	// mySkeletonLoaderSaver->drawCurrentPlaybackFrame();
    ofPopMatrix();
	
	// 0. Draw the GUI.
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
			mySkeletonLoaderSaver->togglePlaybackPaused();
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
            mySkeletonTracer->exportVectorGraphics();
            break;
            
        case '1':
            proxyVideoPlayer.load(proxyVideoFilenames[0]);
            proxyVideoPlayer.play();
            break;
        case '2':
            proxyVideoPlayer.load(proxyVideoFilenames[1]);
            proxyVideoPlayer.play();
            break;
        case '3':
            proxyVideoPlayer.load(proxyVideoFilenames[2]);
            proxyVideoPlayer.play();
            break;
			
		case 'r':
		case 'R':
			mySkeletonLoaderSaver->toggleRecording();
			if (mySkeletonLoaderSaver->isRecording() == false){
				mySkeletonLoaderSaver->saveCurrentRecording();
			}
			break;
		case 'p':
			mySkeletonLoaderSaver->loadAndInitiatePlaybackOfRecording(0);
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
