#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

/*
// Todo:
// Two saver-loaders
 // behaviors for playbackers
 // on-the-fly saving-loading
// Kinectv1
*/

//--------------------------------------------------------------
void ofApp::setup(){

	bFullScreen = true;
	ofSetFullscreen(bFullScreen);
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
	
    receiver.setup(6667);
    
	proxyCaptureW = 320;
	proxyCaptureH = 240;
	displayScale = 0.75;
	mainDisplayAreaScale = 2.9375;
	displayW = proxyCaptureW * displayScale;
	displayH = proxyCaptureH * displayScale;
	displayM = 8;
	
	initializeGui();
	
	proxyVideoFilenames = new string[3];
	proxyVideoFilenames[0] = "proxy/1_Fernanda-Cascada_Input.mov";
	proxyVideoFilenames[1] = "proxy/2_Lewis-Cascada_Input.mov";
	proxyVideoFilenames[2] = "proxy/3_Group-Cascada_Input.mov";

	proxyVideoPlayer.load(proxyVideoFilenames[1]);
	proxyVideoPlayer.play();
	proxyVideoPlayer.setLoopState(OF_LOOP_NORMAL);
	proxyColorImage.allocate (proxyCaptureW,proxyCaptureH);
	proxyColorImage.set(0);
	
	
	bUseProxyVideoInsteadOfOSC	= true;
	bProxyVideoPlayerPaused		= false;
	bGotAProxyFrame				= false;
	bBlankProxyVideo			= false;
	lastProxySwitchTime			= 0;
	
	minBlobSize			= 20;
	maxBlobSize			= (proxyCaptureW*proxyCaptureH)/3;
	maxNBlobs			= 8;
	bGetHoles			= true;
	nRawContours		= 0;
	
	myOfxCvContourFinder.setMinArea(minBlobSize);
	myOfxCvContourFinder.setMaxArea(maxBlobSize);
	
	kinectV1DepthW		= 640;
	kinectV1DepthH		= 480;
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
	for (int i=0; i<2; i++){
		mySkeletonPlaybackers[i] = new SkeletonLoaderSaver();
		mySkeletonPlaybackers[i]->initialize(skeletonBufW, skeletonBufH);
	}
	
	mySkeletonRecorder = new SkeletonLoaderSaver();
	mySkeletonRecorder->initialize(skeletonBufW, skeletonBufH);
	
	mySkeletonDisplayer.initialize(skeletonBufW, skeletonBufH);
	mySkeletonDisplayer.givePointers(mySkeletonTracer,
									 mySkeletonPlaybackers[0],
									 mySkeletonPlaybackers[1]);

}

//--------------------------------------------------------------
void ofApp::exit(){
	
	mySkeletonDisplayer.setToHomePosition();
	mySkeletonDisplayer.etherdream.kill();
	
	if (bSaveOnExit){
		inputGuiPanel1.saveToFile("settings/GPPSettings.xml");
		inputGuiPanel2.saveToFile("settings/GPPSettings2.xml");
		ofLog(OF_LOG_NOTICE, "GUI saved to settings.");
		mySkeletonDisplayer.DQW->saveCalibration();
	}
	
	for (int i=0; i<2; i++){
		mySkeletonPlaybackers[i]->xmlThread.stop();
	}
}


//--------------------------------------------------------------
void ofApp::initializeGui(){
    
    ofxGuiSetDefaultWidth(displayW);
    inputGuiPanel1.setup("Settings", "settings/GPPSettings.xml", displayM, (displayM*3)+(displayH*2));
	
	inputGuiPanel1.add(bSaveOnExit.setup			("bSaveOnExit", false));
	inputGuiPanel1.add(bUseProxyVideoInsteadOfOSC.setup ("bUseProxyVideoInsteadOfOSC",		true));
    inputGuiPanel1.add(proxyThreshold.setup		("proxyThreshold",		80, 0,254));
    inputGuiPanel1.add(inputLineSmoothing.setup	("inputLineSmoothing",	5.0, 0.0, 16.0));
    inputGuiPanel1.add(inputLineResample.setup	("inputLineResample",	3.0, 1.0, 11.0));
    inputGuiPanel1.add(contourThickness.setup	("contourThickness",	1, 0,8));
//	inputGuiPanel1.add(bSmoothHolesToo.setup	("bSmoothHolesToo",		false));
//	inputGuiPanel1.add(bDrawGrayProxy.setup		("bDrawGrayProxy",		false));
    
    inputGuiPanel1.add(boneResampling.setup		("boneResampling",		1.5, 1.0, 11.0));
    inputGuiPanel1.add(boneSmoothSigma.setup	("boneSmoothSigma",		0.9, 0.0, 3.0));
    inputGuiPanel1.add(boneSmoothKernW.setup	("boneSmoothKernW",		2, 1, 7));
    inputGuiPanel1.add(bDoMergeBones.setup		("bDoMergeBones",		true));
//	inputGuiPanel1.add(bDoOptimizeTSP.setup		("bDoOptimizeTSP",		true));
//	inputGuiPanel1.add(bClosedTSP.setup			("bClosedTSP",			true));
//	inputGuiPanel1.add(nOptimizePasses.setup	("nOptimizePasses",		2, 1, 5));
    inputGuiPanel1.add(maxNBonesForTSP.setup	("maxNBonesForTSP",		60, 20,300));
	
	inputGuiPanel1.add(targetPointCount.setup	("targetPointCount",		500, 100,2500));
	inputGuiPanel1.add(preBlankCount.setup		("preBlankCount",			0, 0,30));
	inputGuiPanel1.add(preRepeatCount.setup		("preRepeatCount",			0, 0,30));
	inputGuiPanel1.add(postRepeatCount.setup	("postRepeatCount",			0, 0,30));
	inputGuiPanel1.add(postBlankCount.setup		("postBlankCount",			8, 0,30));
	
	//-----------------------------------
	// SETTINGS 2:
	inputGuiPanel2.setup("Settings2", "settings/GPPSettings2.xml",
						 (displayM*2)+(displayW*1),
						 (displayM*3)+(displayH*2));
	inputGuiPanel2.add(overallScaleX.setup		("overallScaleX",			0.25, 0.0,1.0));
	inputGuiPanel2.add(overallScaleY.setup		("overallScaleY",			0.25, 0.0,1.0));
	inputGuiPanel2.add(overallTransX.setup		("overallTransX",			0.00, -0.5,0.5));
	inputGuiPanel2.add(overallTransY.setup		("overallTransY",			0.00, -0.5,0.5));
	inputGuiPanel2.add(liveDrawingScale.setup	("liveDrawingScale",		1.10, 1.0, 1.5));
	inputGuiPanel2.add(noiseDiv.setup			("noiseDiv",				5.00, 2.0, 7.0));
	inputGuiPanel2.add(noiseAmt.setup			("noiseAmt",				0.75, 0.0, 1.0));
	inputGuiPanel2.add(positionAmt.setup		("positionAmt",				0.50, 0.0, 1.0));
	
	inputGuiPanel2.add(bFadeColorsAtEdges.setup	("bFadeColorsAtEdges",		true));
	inputGuiPanel2.add(bFadeColorsAtEnds.setup	("bFadeColorsAtEnds",		true));
	inputGuiPanel2.add(bAddTestPattern.setup	("bAddTestPattern",			false));
	
	inputGuiPanel2.add(refreshPeriod.setup		("refreshPeriod",			5000, 1000,30000));
	lastRefreshTime = ofGetElapsedTimeMillis();
	refreshPeriodRandomized = refreshPeriod;
	whichRefresh = 0;
	
	inputGuiPanel2.add(recordStartThresh.setup		("recordStartThresh",		0.06, 0.0, 0.25));
	inputGuiPanel2.add(recordStopThresh.setup		("recordStopThresh",		0.03, 0.0, 0.25));
	inputGuiPanel2.add(tooMuchPresenceThresh.setup	("tooMuchPresenceThresh",	0.25, 0.0, 0.50));
	
	
	
	/*
	inputGuiPanel2.add(replayR.setup			("replayR",					0.75, 0,1));
	inputGuiPanel2.add(replayG.setup			("replayG",					0.75, 0,1));
	inputGuiPanel2.add(replayB.setup			("replayB",					1.00, 0,1));
	inputGuiPanel2.add(liveR.setup				("liveR",					1.00, 0,1));
	inputGuiPanel2.add(liveG.setup				("liveG",					1.00, 0,1));
	inputGuiPanel2.add(liveB.setup				("liveB",					1.00, 0,1));
	 */
	
	// Inits for things that have been commented out.
	bSmoothHolesToo		= false;
	bDrawGrayProxy		= false;
	bDoOptimizeTSP		= true;
	bClosedTSP			= true;
	nOptimizePasses		= 2;
	
	replayR				= 0.75;
	replayG				= 0.75;
	replayB				= 0.90;
	liveR				= 1.00;
	liveG				= 0.98;
	liveB				= 0.97;
	
	inputGuiPanel1.loadFromFile("settings/GPPSettings.xml");
	inputGuiPanel2.loadFromFile("settings/GPPSettings2.xml");
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

	mySkeletonDisplayer.ildaFrame.polyProcessor.params.targetPointCount = targetPointCount;
	mySkeletonDisplayer.ildaFrame.params.output.preBlankCount	= preBlankCount;
	mySkeletonDisplayer.ildaFrame.params.output.preRepeatCount	= preRepeatCount;
	mySkeletonDisplayer.ildaFrame.params.output.postBlankCount	= postRepeatCount;
	mySkeletonDisplayer.ildaFrame.params.output.postRepeatCount	= postBlankCount;
	
	mySkeletonDisplayer.overallScaleX = overallScaleX;
	mySkeletonDisplayer.overallScaleY = overallScaleY;
	mySkeletonDisplayer.overallTransX = overallTransX;
	mySkeletonDisplayer.overallTransY = overallTransY;
	mySkeletonDisplayer.liveDrawingScale = liveDrawingScale;
	mySkeletonDisplayer.noiseDiv = noiseDiv;
	mySkeletonDisplayer.noiseAmt = noiseAmt;
	mySkeletonDisplayer.positionAmt = positionAmt;
	
	mySkeletonDisplayer.bFadeColorsAtEdges = bFadeColorsAtEdges;
	mySkeletonDisplayer.bFadeColorsAtEnds = bFadeColorsAtEnds;
	mySkeletonDisplayer.bAddTestPattern = bAddTestPattern;
	
	mySkeletonPlaybackers[0]->replayColor.r = replayR;
	mySkeletonPlaybackers[0]->replayColor.g = replayG;
	mySkeletonPlaybackers[0]->replayColor.b = replayB;
	mySkeletonPlaybackers[1]->replayColor.r = replayR;
	mySkeletonPlaybackers[1]->replayColor.g = replayG;
	mySkeletonPlaybackers[1]->replayColor.b = replayB;
	mySkeletonDisplayer.liveColor.r = liveR;
	mySkeletonDisplayer.liveColor.g = liveG;
	mySkeletonDisplayer.liveColor.b = liveB;
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
    nRawContours = rawContours.size();
    if ((nRawContours > 0) && (bBlankProxyVideo == false)){
		
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
		
		// If mySkeletonRecorder is recording, add in the most recent frame.
		if (mySkeletonRecorder->isRecording()){
			vector<PolylinePlus> &theRawDrawing = mySkeletonTracer->theRawDrawing;
			mySkeletonRecorder->addFrameToCurrentRecording(theRawDrawing);
		}
		
    } else {
        // There are no incoming contours, so handle that.
        handleAbsenceOfIncomingContours();
    }
	
	
	mySkeletonDisplayer.update(); 
	
	sequenceSkeletonPlayback();
}




//--------------------------------------------------------------
void ofApp::sequenceSkeletonPlayback(){
	
	// Periodically, change up the recordings played by the SkeletonLoaderSavers.
	long now = ofGetElapsedTimeMillis();
	long elapsedSincePlayerRefreshed = (now - lastRefreshTime);
	if (elapsedSincePlayerRefreshed > refreshPeriodRandomized){
		refreshSkeletonPlaybacker (whichRefresh);
		refreshPeriodRandomized = (int)(refreshPeriod * ofRandom(0.7, 1.3));
		lastRefreshTime = ofGetElapsedTimeMillis();
		whichRefresh = (whichRefresh + 1)%2;
	}
	
	// If we're displaying the proxy video, then with a low probability,
	// and if a certain amount of time has passed, load a different proxy video.
	if (bUseProxyVideoInsteadOfOSC){
		long minTimeBetweenProxySwitch = (refreshPeriod * 4);
		long elapsedSinceProxySwitch = (now - lastProxySwitchTime);
		if (elapsedSinceProxySwitch > minTimeBetweenProxySwitch){
			if (ofRandom(1.0) < 0.002){
				int randomProxy = ((int)ofRandom(3));
				proxyVideoPlayer.load(proxyVideoFilenames[randomProxy]);
				proxyVideoPlayer.play();
				lastProxySwitchTime = now;
			}
		}
	}
	
	//----------------------
	// Update whether or not we're recording, based on suitability of input.
	// Compute area of reconstituted blobs, a measure of presence.
	float contourAreaSum = 0;
	int nFinalContours = theContoursi.size();
	for (int i=0; i<nFinalContours; i++){
		vector<cv::Point> aContour = theContoursi[i];
		float aContourArea = cv::contourArea(aContour);
		if (bUseProxyVideoInsteadOfOSC){
			if (myOfxCvContourFinder.getHole(i) == true){
				aContourArea = 0-aContourArea; }
		} else {
			if (isHole[i] == 0) {
				aContourArea = 0-aContourArea; }
		}
		contourAreaSum += aContourArea;
	}
	
	float presenceFraction = abs(contourAreaSum) / (float)(skeletonBufW * skeletonBufH);
	float A = 0.925; float B = 1.0-A;
	presenceFractionAvg = (A*presenceFractionAvg) + (B*presenceFraction);
	
	if (presenceFractionAvg > tooMuchPresenceThresh){
		// If the person is too big in the frame, too close to the camera, etc.
		stopRecording();
	} else if (presenceFractionAvg > recordStartThresh){
		// If we exceed recordStartThresh (with a latch), start recording.
		startRecording();
	} else if (presenceFractionAvg < recordStopThresh){
		// If we fall below recordStopThresh (with a latch), stop recording.
		stopAndSaveRecording();
	}
	
	long started = mySkeletonRecorder->recordingStartTime;
	int elapsed = (int)(now - started);
	int recordingTooLongDur = 120000;
	if (mySkeletonRecorder->isRecording() && (elapsed > recordingTooLongDur)){
		stopAndSaveRecording();
	}
	
	
	// Move them closer together if nobody is in the frame.
	float positionAmtTarget = 0.5;
	if (nFinalContours == 0){
		positionAmtTarget = 0.25;
	}
	float pA = 0.99;
	float pB = 1.0-pA;
	positionAmt = pA*positionAmt + pB*positionAmtTarget;
}


//--------------------------------------------------------------
void ofApp::refreshSkeletonPlaybacker(int which){
	lastRefreshTime = ofGetElapsedTimeMillis();
	which = (which%2);
	
	// Initiate playback of a random recording -- but,
	// With some random chance, clear the player altogether.
	float probabilityOfBeingClear = 0.05;
	if (ofRandom(0.0,1.0) < probabilityOfBeingClear){
		mySkeletonPlaybackers[which]->clear();
	} else {
		mySkeletonPlaybackers[which]->loadAndInitiatePlaybackOfRandomRecording();
	}
}




//--------------------------------------------------------------
void ofApp::toggleRecording(){
	if (mySkeletonRecorder->isRecording() == false){
		startRecording();
	} else {
		stopAndSaveRecording();
	}
}

//--------------------------------------------------------------
void ofApp::startRecording(){
	if (mySkeletonRecorder->isRecording() == false){
		mySkeletonRecorder->startRecording();
	}
}

//--------------------------------------------------------------
void ofApp::stopRecording(){
	if (mySkeletonRecorder->isRecording()){
		mySkeletonRecorder->stopRecording();
	}
}

//--------------------------------------------------------------
void ofApp::stopAndSaveRecording(){
	if (mySkeletonRecorder->isRecording()){
		mySkeletonRecorder->saveCurrentRecording();
	}
}

//--------------------------------------------------------------
vector<vector<cv::Point>> ofApp::obtainRawContours(){
    
    vector<vector<cv::Point>> rawContours;
    if (bUseProxyVideoInsteadOfOSC){
		
        computeContoursFromProxyVideo();
        rawContours = myOfxCvContourFinder.getContours();
		
    } else {
		
		bBlankProxyVideo = false;
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
			//if(m.getAddress() == "/blob"){  "/metadata/kinect");
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
	theContoursi.clear();
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
				jxf *= (float) skeletonBufW / (float) kinectV1DepthW; //(320.0/512.0);
				jyf *= (float) skeletonBufH / (float) kinectV1DepthH; //(320.0/512.0);
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
	
	float ty = displayY;
	float dy = 14;
	if (bUseProxyVideoInsteadOfOSC){
		int frameIndex = proxyVideoPlayer.getCurrentFrame();
		ofSetColor(160,160,160);
		ofDrawBitmapString( "Frame: " + ofToString(frameIndex), displayX+5,ty+dy); ty+=dy;
	}
	
	
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
	
	// Draw the recording indicator
	ty = displayY;
	ofFill();
	ofSetColor(255,128,0);
	ofDrawBitmapString("%: " + ofToString(presenceFractionAvg), displayX+5, ty+15); ty+=dy;
	// presenceFractionAvg, recordStartThresh, recordStopThresh
	
	if (mySkeletonRecorder->isRecording()){
		ofDrawRectangle(displayX+5,ty+5, 10,10);
		int nRecFrames = mySkeletonRecorder->getCurrentRecordingLength();
		int fileIndex = SkeletonLoaderSaver::outputFileCounter;
		ofDrawBitmapString("Recording #" + ofToString(fileIndex) + ": " +
						   ofToString(nRecFrames), displayX+20, ty+15);
		ofDrawBitmapString("Recording #" + ofToString(fileIndex) + ": " +
						   ofToString(nRecFrames), displayX+21, ty+15);
	}

    // 4. Draw the skeleton image, and a data report
    displayX = 1*displayW + 2*displayM;
    displayY = 1*displayH + 2*displayM;
    ofPushMatrix();
    ofTranslate(displayX,displayY);
	ofScale(displayScale,displayScale);
    mySkeletonizer.draw();
    ofPopMatrix();
	
	int durMicros	= (int)(mySkeletonizer.skeletonizationDuration);
	int tspMicros	= (int)(mySkeletonDisplayer.tspElapsed);
	int totalNPts	= mySkeletonDisplayer.totalNPoints;
	int nTspBones	= mySkeletonDisplayer.nCombinedPolylinePluses;
	int optim		= (int)(100.0 * mySkeletonDisplayer.optimizationAmount);
	int nIldaPts	= mySkeletonDisplayer.ildaFrame.getNPoints();
	
	ty = 16;
	dy = 14;
	ofSetColor(255,255,0);
	ofDrawBitmapString( "Skl: " + ofToString(durMicros) + " us", displayX+5,displayY+ty); ty+=dy;
	ofDrawBitmapString( "TSP: " + ofToString(tspMicros) + " us", displayX+5,displayY+ty); ty+=dy;
	ofDrawBitmapString( "#Pt: " + ofToString(totalNPts)        , displayX+5,displayY+ty); ty+=dy;
	ofDrawBitmapString( "#PP: " + ofToString(nTspBones)        , displayX+5,displayY+ty); ty+=dy;
	ofDrawBitmapString( "%Op: " + ofToString(optim)		+ "%"  , displayX+5,displayY+ty); ty+=dy;
	ofDrawBitmapString( "#IP: " + ofToString(nIldaPts)	       , displayX+5,displayY+ty); ty+=dy;
	
	//-------------------------
    // 5. Draw the bones.
    ofPushMatrix();
    displayX = 2*displayW + 3*displayM;
    displayY = 0*displayH + 1*displayM;
    ofTranslate(displayX,displayY);
    ofScale(mainDisplayAreaScale, 0-mainDisplayAreaScale); // 705x705
	ofTranslate(0, 0-displayW); // coordinate system is flipped.
	
	ofFill();
	ofSetColor(32,32,32);
	ofDrawRectangle(0,0, displayW,displayW); // yes, square.
	if (bDrawGrayProxy){
		ofSetHexColor(0x202020);
		filledContourImage.draw(0,0, displayW,displayH);
	}
	
	ofScale(displayScale, displayScale);
	mySkeletonDisplayer.bShowPathBetweenBones = true;
	mySkeletonDisplayer.renderToScreen();
	mySkeletonDisplayer.renderDisplayQuadWarper(); 
    ofPopMatrix();
	
	ty = displayY;
	dy = 14;
	int laserstate = mySkeletonDisplayer.etherdream.getState();
	if (laserstate == 0){
		ofSetColor(255,0,0);
		ofDrawBitmapString( "ETHERDREAM_NOTFOUND", displayX+5,ty+dy); ty+=dy;
	} else {
		ofSetColor(0,255,0);
		ofDrawBitmapString( "ETHERDREAM_FOUND: id-" + ofToString(laserstate), displayX+5,ty+dy); ty+=dy;
	}
	
	// Draw the playback indicators.
	ty+=dy;
	ofSetColor(255,255,0);
	ofDrawBitmapString("Refresh:", displayX+5,ty);
	ofPushMatrix();
	ofTranslate(displayX+120, ty);
	float elapsed = (ofGetElapsedTimeMillis() - lastRefreshTime);
	float elapsedFrac = elapsed / (float)refreshPeriodRandomized;
	float barlen = sqrt(refreshPeriodRandomized);
	ofSetLineWidth(1.0);
	ofSetColor(180,180,180);
	ofDrawLine(0,0,			0,-10);
	ofDrawLine(barlen,0,	barlen,-10);
	ofDrawLine(0,-5,		barlen,-5);
	ofSetColor(255,255,0);
	ofDrawLine(0,-5,		elapsedFrac*barlen,-5);
	ofDrawCircle(elapsedFrac*barlen,-5, 3);
	ofPopMatrix();
	
	for (int i=0; i<2; i++){
		ofSetColor(180,180,180);
		int video	= mySkeletonPlaybackers[i]->getCurrentPlaybackRecordingIndex();
		int frame	= mySkeletonPlaybackers[i]->getCurrentPlaybackFrameIndex();
		int length	= mySkeletonPlaybackers[i]->getCurrentPlaybackLength();
		ofDrawBitmapString(ofToString(i) + ": " + ofToString(video) + ", " +
						   ofToString(frame) + "/" + ofToString(length),
						   displayX+5,ty+dy);
		ty+=dy;
		
		ofPushMatrix();
		ofTranslate(displayX+120, ty);
		
		ofSetColor(180,180,180);
		ofSetLineWidth(1.0);
		ofDrawLine(0,0,			0,-10);
		ofDrawLine(length,0,	length,-10);
		ofDrawLine(0,-5,		length,-5);
		
		ofSetColor(255,255,255);
		ofDrawLine(0,-5,		frame,-5);
		ofDrawCircle(frame,-5, 3);
		
		ofPopMatrix();
	}
	
	// 0. Draw the GUI.
	inputGuiPanel1.draw();
	inputGuiPanel2.draw();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	mySkeletonDisplayer.DQW->setCalibrationMode(-1);
	
    switch (key){
        case ' ':
            if (bUseProxyVideoInsteadOfOSC){
                bProxyVideoPlayerPaused = !bProxyVideoPlayerPaused;
                proxyVideoPlayer.setPaused(bProxyVideoPlayerPaused);
            }
			mySkeletonPlaybackers[0]->togglePlaybackPaused();
			mySkeletonPlaybackers[1]->togglePlaybackPaused();
            break;
            
        case ',':
            proxyVideoPlayer.previousFrame();
            break;
        case '.':
            proxyVideoPlayer.nextFrame();
            break;
		case 'b':
			bBlankProxyVideo = !bBlankProxyVideo;
			break;
            
        case 'S':
            inputGuiPanel1.saveToFile("settings/GPPSettings.xml");
			inputGuiPanel2.saveToFile("settings/GPPSettings2.xml");
			ofLog(OF_LOG_NOTICE, "GUI saved to settings.");
			mySkeletonDisplayer.DQW->saveCalibration();
            break;
        case 'L':
            inputGuiPanel1.loadFromFile("settings/GPPSettings.xml");
			inputGuiPanel2.loadFromFile("settings/GPPSettings2.xml");
            break;
		case 'F':
		case 'f':
			bFullScreen = !bFullScreen;
			ofSetFullscreen(bFullScreen);
			if (!bFullScreen){
				ofSetWindowShape(1280,720);
			}
			break;
			
		case '1':
			mySkeletonDisplayer.DQW->setCalibrationMode(0); break;
		case '2':
			mySkeletonDisplayer.DQW->setCalibrationMode(1); break;
		case '3':
			mySkeletonDisplayer.DQW->setCalibrationMode(2); break;
		case '4':
			mySkeletonDisplayer.DQW->setCalibrationMode(3); break;
            
        case '8':
			lastProxySwitchTime = ofGetElapsedTimeMillis();
            proxyVideoPlayer.load(proxyVideoFilenames[0]);
            proxyVideoPlayer.play();
            break;
        case '9':
			lastProxySwitchTime = ofGetElapsedTimeMillis();
            proxyVideoPlayer.load(proxyVideoFilenames[1]);
            proxyVideoPlayer.play();
            break;
        case '0':
			lastProxySwitchTime = ofGetElapsedTimeMillis();
            proxyVideoPlayer.load(proxyVideoFilenames[2]);
            proxyVideoPlayer.play();
            break;
			
		case 'c':
			mySkeletonPlaybackers[0]->clear();
			mySkeletonPlaybackers[1]->clear();
			break;
			
		case 'r':
			toggleRecording();
			break;
			
		case 't':
			bAddTestPattern = !bAddTestPattern;
			break;
			
		case 'p':
			refreshSkeletonPlaybacker(whichRefresh);
			whichRefresh = (whichRefresh + 1)%2;
			break;

    }
}



//--------------------------------------------------------------
void ofApp::keyReleased(int key){}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	
	// Handle adjustment for display quad warper control points.
	float mainDisplayAreaX = 2*displayW + 3*displayM;
	float mainDisplayAreaY = 0*displayH + 1*displayM;
	float mainDisplayAreaW = displayW * mainDisplayAreaScale;
	
	float x01 =       (mouseX - mainDisplayAreaX)/mainDisplayAreaW;
	float y01 = 1.0 - (mouseY - mainDisplayAreaY)/mainDisplayAreaW; // yes, W*W
	
	mySkeletonDisplayer.DQW->mouseDown(x01, y01);
}

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
