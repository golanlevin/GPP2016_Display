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
	displayScale = 1.0;
	displayW = proxyCaptureW * displayScale;
	displayH = proxyCaptureH * displayScale;
	displayM = 10;
	
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
	
	skeletonBufW		= 320; // Note: the Kinectv2 depth image is 512 x 424.
	skeletonBufH		= 240; // 320x265 would preserve the aspect ratio of Kinectv2 depth image.
	skeletonScale		= (float)skeletonBufW/(float)proxyCaptureW;
	bScaleProxyToKinect	= true;
	
	filledContourMat.create    (skeletonBufH, skeletonBufW, CV_8UC(1));
	filledContourImage.allocate(skeletonBufW, skeletonBufH, OF_IMAGE_GRAYSCALE);
	
	mySkeletonTracer = new SkeletonTracer();
	mySkeletonTracer->initialize(skeletonBufW, skeletonBufH);
	mySkeletonizer.initialize(skeletonBufW, skeletonBufH);

	mySkelevision.initialize();
	mySkelevision.testBuffer();
	
}







//--------------------------------------------------------------
void ofApp::initializeGui(){
    
    ofxGuiSetDefaultWidth(320);
    inputGuiPanel.setup("Settings", "settings/GPPSettings.xml", displayM, (displayM*3)+(displayH*2));
    
    inputGuiPanel.add(proxyThreshold.setup		("proxyThreshold",		80, 0,254));
    inputGuiPanel.add(inputLineSmoothing.setup	("inputLineSmoothing",	5.0, 0.0, 16.0));
    inputGuiPanel.add(inputLineResample.setup	("inputLineResample",	3.0, 1.0, 11.0));
    inputGuiPanel.add(contourThickness.setup	("contourThickness",	2, 0,8));
    inputGuiPanel.add(bSmoothHolesToo.setup		("bSmoothHolesToo",		false));
    
    inputGuiPanel.add(boneResampling.setup		("boneResampling",		3.0, 1.0, 11.0));
    inputGuiPanel.add(boneSmoothSigma.setup		("boneSmoothSigma",		0.9, 0.0, 3.0));
    inputGuiPanel.add(boneSmoothKernW.setup		("boneSmoothKernW",		2, 1, 7));
    inputGuiPanel.add(bDoMergeBones.setup		("bDoMergeBones",		true));
    inputGuiPanel.add(bDoOptimizeTSP.setup		("bDoOptimizeTSP",		true));
    inputGuiPanel.add(bClosedTSP.setup			("bClosedTSP",			false));
    inputGuiPanel.add(maxNBonesForTSP.setup		("maxNBonesForTSP",		50, 20,300));
    // inputGuiPanel.add(nOptimizePasses.setup		("nOptimizePasses",		2, 1, 5));
}


//--------------------------------------------------------------
void ofApp::propagateGui(){
    mySkeletonTracer->boneResampling	= (float)	boneResampling;
    mySkeletonTracer->boneSmoothSigma	= (float)	boneSmoothSigma;
    mySkeletonTracer->boneSmoothKernW	= (int)		boneSmoothKernW;
    mySkeletonTracer->bDoMergeBones		= (bool)	bDoMergeBones;
    mySkeletonTracer->bDoOptimizeTSP	= (bool)	bDoOptimizeTSP;
    mySkeletonTracer->bClosedTSP		= (bool)	bClosedTSP;
    mySkeletonTracer->maxNBonesForTSP	= (int)		maxNBonesForTSP;
    // mySkeletonTracer->nOptimizePasses	= (int)		nOptimizePasses;
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
            //;
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
        
        if(bUseProxyVideoInsteadOfOSC){
            // Filter the contours, smoothing them to yield better skeletons.
            filterContoursBeforeReconstitution (rawContours);
            
            // Prepare a blob image: our input to thinning-based skeletonization.
            reconstituteBlobsFromContours (theContoursi, skeletonBufW,skeletonBufH);
        }else{
            // Filter the contours, smoothing them to yield better skeletons.
            filterContoursBeforeReconstitutionOSC (rawContours);
            
            // Prepare a blob image: our input to thinning-based skeletonization.
            reconstituteBlobsFromContoursOSC (theContoursi, skeletonBufW,skeletonBufH);
            
        }
        // Compute a skeleton image: pixels representing the live skeletons.
        mySkeletonizer.computeSkeletonImageFromBlobs(filledContourMat, theContoursi,
                                                     nCurrentPositiveContours, contourThickness,
                                                     skeletonBufW,skeletonBufH);
        
        // Trace the (polyline) skeleton bones from the (pixel) skeleton image.
        mySkeletonTracer->computeVectorSkeleton (mySkeletonizer.skeletonBuffer, nRawContours);
        
    } else {
        // There are no incoming contours, so handle that.
        handleAbsenceOfIncomingContours();
    }
    
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
        // Note that it will be necessary for these to match ofxCv contours,
        // For example: indicating whether a contour is a hole or not.
        
        
        //				int size = buffer.size()/sizeof(ofPoint);
        //				char *tempData = buffer.getData();
        //				points.resize(size);
        //				memcpy(&points[0], tempData, buffer.size());
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
//                if (bScaleProxyToKinect){
                    jxf *= (320.0/512.0);
                    jyf *= (320.0/512.0);
//                }
                
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
    
    // 3. Draw the blobs reconstituted from the contours, and their combined bounding box.
    ofSetHexColor(0xffffff);
    displayX = 1*displayW + 2*displayM;
    displayY = 0*displayH + 1*displayM;
    filledContourImage.draw(displayX,displayY, displayW,displayH);
    
    // 4. Draw the skeleton image.
    displayX = 1*displayW + 2*displayM;
    displayY = 1*displayH + 2*displayM;
    ofPushMatrix();
    ofTranslate(displayX,displayY);
    mySkeletonizer.draw();
    ofPopMatrix();
    int durMicros = (int)(mySkeletonizer.skeletonizationDuration);
    ofSetColor(255,255,0);
    ofDrawBitmapString( ofToString(durMicros) + " us", displayX+5,displayY+16);
    
    // 5. Draw the skeleton-tracer state image.
    ofSetHexColor(0xffffff);
    displayX = 1*displayW + 2*displayM;
    displayY = 2*displayH + 3*displayM;
    ofPushMatrix();
    ofTranslate(displayX,displayY);
    mySkeletonTracer->drawStateImage();
    ofPopMatrix();
    int tspMicros = (int)(mySkeletonTracer->tspElapsed);
    ofSetColor(255,255,0);
    ofDrawBitmapString( ofToString(tspMicros) + " us", displayX+5,displayY+16);
    
    
    
    // 6. Draw the bones.
    ofPushMatrix();
    displayX = 2*displayW + 3*displayM;
    displayY = 0*displayH + 1*displayM;
    ofTranslate(displayX,displayY);
    ofScale(3.0,3.0);
    ofSetHexColor(0x202020);
    proxyColorImage.draw(0,0, displayW,displayH);
    mySkeletonTracer->drawBones();
    ofPopMatrix();
    
    
    inputGuiPanel.draw();
	
	ofPushMatrix();
	ofTranslate(mouseX,mouseY);
	mySkelevision.drawCurrentPlaybackFrame();
	ofPopMatrix();
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
