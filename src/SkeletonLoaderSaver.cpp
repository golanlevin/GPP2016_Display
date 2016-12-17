#include "SkeletonLoaderSaver.h"


//--------------------------------------------------------------

SkeletonLoaderSaver::SkeletonLoaderSaver(){
    
}

SkeletonLoaderSaver::~SkeletonLoaderSaver(){
    
}

int SkeletonLoaderSaver::outputFileCounter = 0;

//--------------------------------------------------------------
void SkeletonLoaderSaver::initialize(int w, int h){
	bUseZippedFiles = true;
	
	string path = "recordings";
	recordingsDirectory.open(path);
	if (bUseZippedFiles){
		recordingsDirectory.allowExt("zip");
	} else {
		recordingsDirectory.allowExt("xml");
	}
	recordingsDirectory.listDir();
	/*
	for (int i=0; i< recordingsDirectory.size(); i++){
		ofLogNotice(recordingsDirectory.getPath(i));
	}
	*/ 
	
	bPlayBackAndForth	= true;
	playbackDirection	= 1;
	minNRecordingFrames	= 50;
    maxNRecordingFrames = 200;
	maxNRecordings		= 100;
	bDeleteOldestFramesWhenRecording = true;
	playbackRecordingIndex = 0;
    
    readXML.clear();

	replayColor.set(0.0, 40.0/255.0, 1.0);
    
    currentPlaybackFrameIndex = 0;
    currentPlaybackDrawing.clear();
    currentPlaybackFrames.clear();
    currentRecordingFrames.clear();
    
    bCurrentlyRecording	= false;
    bLoadedFileFromXML	= false;
    bPlaybackPaused		= false;
    
    bUseNormalizedDrawings = true;
    buffer_w = w;
    buffer_h = h;


    
    xmlThread.start();
    
    ofAddListener(xmlThread.xmlSaved, this,  &SkeletonLoaderSaver::recordingSaved);
    ofAddListener(xmlThread.xmlLoaded, this,  &SkeletonLoaderSaver::recordingLoaded);
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::clear(){
	readXML.clear();
	
	currentPlaybackDrawing.clear();
	currentPlaybackFrames.clear();
	currentRecordingFrames.clear();
	
	currentPlaybackFrameIndex = 0;
	currentPlaybackFramePercent = 0;
	bCurrentlyRecording = false;
}


//--------------------------------------------------------------
void SkeletonLoaderSaver::startRecording(){
	recordingStartTime = ofGetElapsedTimeMillis();
    currentRecordingFrames.clear();
    bCurrentlyRecording = true;
}

//---------------------------------
void SkeletonLoaderSaver::stopRecording(){
    bCurrentlyRecording = false;
}

//---------------------------------
void SkeletonLoaderSaver::toggleRecording(){
    bCurrentlyRecording = !bCurrentlyRecording;
    if (bCurrentlyRecording){
        startRecording();
    } else {
        stopRecording();
    }
}

//---------------------------------
void SkeletonLoaderSaver::saveCurrentRecording(){
	// Don't call stopRecording() beforehand, or this won't do anything!
	if (bCurrentlyRecording){
		stopRecording();

		if (currentRecordingFrames.size() > minNRecordingFrames){
			int outn = SkeletonLoaderSaver::outputFileCounter;
			string xmlFilename = "recordings/recording_" + ofToString(outn) + ".xml";
			if (bUseZippedFiles){ xmlFilename += ".zip"; }
			saveXMLRecording (xmlFilename, bUseZippedFiles);
		}
	}
}

//---------------------------------
bool SkeletonLoaderSaver::isRecording(){
    return bCurrentlyRecording;
}



//--------------------------------------------------------------
void SkeletonLoaderSaver::addFrameToCurrentRecording (vector<PolylinePlus> &theRawDrawing){
    if (bCurrentlyRecording){
        
        currentRecordingFrames.push_back(theRawDrawing);
        
        int nRecordedFrames = currentRecordingFrames.size();
        if (nRecordedFrames > maxNRecordingFrames){
            if (bDeleteOldestFramesWhenRecording){
                while (currentRecordingFrames.size() > maxNRecordingFrames){
                    currentRecordingFrames.erase(currentRecordingFrames.begin());
                }
            } else {
                stopRecording();
            }
        }
    }
}


//---------------------------------
void SkeletonLoaderSaver::loadAndInitiatePlaybackOfRandomRecording(){
	stopRecording();
	recordingsDirectory.listDir();
	int randomIndex = (int) ofRandom(0.999999 * recordingsDirectory.size());
	
	// Ensure difference from previous run.
	if (recordingsDirectory.size() > 2){
		while (randomIndex == playbackRecordingIndex){
			randomIndex = (int) ofRandom(0.999999 * recordingsDirectory.size());
		}
	}

	// Flip a weighted coin to decide if we'll go back-and-forth on this run.
	bPlayBackAndForth = (ofRandom(1.0) < 0.66);
	
	// Reset state
	currentPlaybackFrameIndex = 0;
	currentPlaybackFramePercent = 0;
	
	// Load the requested file.
	playbackRecordingIndex = randomIndex;
	string xmlFilename = recordingsDirectory.getPath(playbackRecordingIndex);
	loadXMLRecording(xmlFilename, bUseZippedFiles);
}

//---------------------------------
void SkeletonLoaderSaver::loadAndInitiatePlaybackOfRecording (int which){
    if(savedFiles.size() >= which && savedFiles.size() > 0){
        string xmlFilename = savedFiles[which];
        loadXMLRecording(xmlFilename, bUseZippedFiles);
		currentPlaybackFrameIndex = 0;
		playbackRecordingIndex = which;
    }
}

//---------------------------------
void SkeletonLoaderSaver::loadAndInitiatePlaybackOfNextRecording(){
	recordingsDirectory.listDir();
	int nRecordings = recordingsDirectory.size();
	if (nRecordings > 0){
		playbackRecordingIndex = (playbackRecordingIndex + 1)%nRecordings;
		string xmlFilename = recordingsDirectory.getPath(playbackRecordingIndex);
		loadXMLRecording(xmlFilename, bUseZippedFiles);
		currentPlaybackFrameIndex = 0;
	}
}

//---------------------------------
int SkeletonLoaderSaver::getCurrentPlaybackRecordingIndex(){
	return playbackRecordingIndex;
}

//---------------------------------
int SkeletonLoaderSaver::getCurrentPlaybackFrameIndex(){
	return currentPlaybackFrameIndex;
}

//---------------------------------
int SkeletonLoaderSaver::getCurrentPlaybackLength(){
	return (currentPlaybackFrames.size());
}

//---------------------------------
int SkeletonLoaderSaver::getCurrentRecordingLength(){
	return (currentRecordingFrames.size());
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::loadXMLRecording (string &xmlFilename, bool bFileIsZipped){
    // Loads a recording into the XML ofxXMLSettings object from a zipped (or not) file.
    
    xmlThread.loadXml(xmlFilename);
    
    
}


void SkeletonLoaderSaver::recordingSaved(string & filename){
	// ofLog(OF_LOG_NOTICE)<<"!!!!!!!!!!!!!!!!!!!!!!  "<<filename<<" saved"<<endl;
    savedFiles.push_back(filename);
	
	int ofc;
	ofc = SkeletonLoaderSaver::outputFileCounter;
	ofc = (ofc + 1) % maxNRecordings;
	SkeletonLoaderSaver::outputFileCounter = ofc;
}

void SkeletonLoaderSaver::recordingLoaded(ofxXmlSettings & xml){
	// ofLog(OF_LOG_NOTICE)<<"!!!!!!!!!!!!!!!!!!!!!!   loaded"<<endl;
    readXML = xml;
    bLoadedFileFromXML = true;
    transferFromXmlToCurrentDrawing();
    
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::transferFromXmlToCurrentDrawing(){
    // loadXMLRecording only loads data into the XML ofxXmlSettings object.
    // Here, we copy the data from the XML object into currentPlaybackFrames.
    
    currentPlaybackFrameIndex = 0;
	currentPlaybackFramePercent = 0;
    currentPlaybackFrames.clear();
    
    if (bLoadedFileFromXML){
        ofSetLogLevel(OF_LOG_WARNING);
        
        int nFramesInThisRecording = readXML.getNumTags("FRAME");
        ofLog(OF_LOG_NOTICE, "nFramesInThisRecording = %d", nFramesInThisRecording);
        if(nFramesInThisRecording > 0){
            
            for (int f=0; f<nFramesInThisRecording; f++){
                readXML.pushTag("FRAME", f);
                vector<PolylinePlus> aVectorOfPolylinePluses;
                
                int nStrokesInThisFrame = readXML.getNumTags("STROKE");
                ofLog(OF_LOG_NOTICE, "  nStrokesInThisFrame = %d", nStrokesInThisFrame);
                if (nStrokesInThisFrame > 0){
                    
                    for (int s=0; s<nStrokesInThisFrame; s++){
                        readXML.pushTag("STROKE", s);
                        ofPolyline aPolyline;
                        
                        int nPointsInThisStroke = readXML.getNumTags("PT");
                        ofLog(OF_LOG_NOTICE, "    nPointsInThisStroke = %d", nPointsInThisStroke);
                        if (nPointsInThisStroke > 0){
                            
                            for (int i=0; i<nPointsInThisStroke; i++){
                                float x = readXML.getValue("PT:X", (double) 0.0, i);
                                float y = readXML.getValue("PT:Y", (double) 0.0, i);
                                aPolyline.addVertex(x, y);
                            }
                        }
                        
                        PolylinePlus aPolylinePlus;
                        aPolylinePlus.polyline = aPolyline;
                        aPolylinePlus.r = replayColor.r * 255.0;
                        aPolylinePlus.g = replayColor.g * 255.0;
                        aPolylinePlus.b = replayColor.b * 255.0;
                        
                        aVectorOfPolylinePluses.push_back(aPolylinePlus);
                        readXML.popTag(); // STROKE
                    }
                }
                currentPlaybackFrames.push_back(aVectorOfPolylinePluses);
                readXML.popTag(); // FRAME
            }
        }
        ofLog(OF_LOG_NOTICE, "Loaded %d frames.", nFramesInThisRecording);
    }
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::togglePlaybackPaused(){
    bPlaybackPaused = !bPlaybackPaused;
    if (bPlaybackPaused){
        stopRecording();
    }
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::retrieveAndAdvanceCurrentPlaybackDrawing(){
    
    currentPlaybackDrawing.clear();
    if (bLoadedFileFromXML){
        int nFramesInThisRecording = currentPlaybackFrames.size();
        if (nFramesInThisRecording > 0) {
			
			
			if (bPlayBackAndForth){
				// Sanity check.
				currentPlaybackFrameIndex = MAX(currentPlaybackFrameIndex, 0);
				currentPlaybackFrameIndex = MIN(currentPlaybackFrameIndex, nFramesInThisRecording-1);
				currentPlaybackDrawing = currentPlaybackFrames[currentPlaybackFrameIndex];
				
				// advance currentPlaybackFrameIndex
				if (!bPlaybackPaused){
					currentPlaybackFrameIndex += playbackDirection;
					if (currentPlaybackFrameIndex >= (nFramesInThisRecording-1)){
						playbackDirection = -1;
					}
					if (currentPlaybackFrameIndex <= 0){
						playbackDirection = 1;
					}
				}
				
			} else {
				// Straightforward looping, with discontinuity
				currentPlaybackFrameIndex %= nFramesInThisRecording; // Safety check.
				currentPlaybackDrawing = currentPlaybackFrames[currentPlaybackFrameIndex];
				
				// advance currentPlaybackFrameIndex
				if (!bPlaybackPaused){
					currentPlaybackFrameIndex ++;
					currentPlaybackFrameIndex %= nFramesInThisRecording;
				}
			}
			
			currentPlaybackFramePercent = (float)currentPlaybackFrameIndex/(float)nFramesInThisRecording;
        }
    }
}


//--------------------------------------------------------------
void SkeletonLoaderSaver::saveXMLRecording (string &xmlFilename, bool bSaveAsZipped)
{
    xmlThread.saveXml(currentRecordingFrames, xmlFilename, bSaveAsZipped);
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::generateBogusFrames(){
    //-------------------
    // Create some bogus drawing data.
    startRecording();
    currentRecordingFrames.clear();
    
    int nFramesPerRecording = 200;
    for (int f=0; f<nFramesPerRecording; f++){
        
        int nPolylinesPerFrame = 15;
        vector<PolylinePlus> somePolylinePluses;
        for (int p=0; p<nPolylinesPerFrame; p++){
            
            int nPointsPerPolyline = 20;
            ofPolyline aPolyline;
            aPolyline.clear();
            
            float dx = (320/2);
            float dy = (240/2);
            float radiusX =  30 + p*3.0;
            float radiusY =  30 + p*3.0;
            
            for (int i=0; i<nPointsPerPolyline; i++){
                float t = ofMap(i,0,nPointsPerPolyline, 0,TWO_PI * 0.4);
                float x = dx + radiusX * cos((f * 0.1) + t);
                float y = dy + radiusY * sin((f * 0.1) + t + p*0.02);
                x /= buffer_w;
                y /= buffer_w; // NOTE: SQUARE SPACE (WxW)
                aPolyline.addVertex(x, y);
            }
            
            PolylinePlus aPolylinePlus;
            aPolylinePlus.polyline = aPolyline;
            aPolylinePlus.r = 0;
            aPolylinePlus.g = 0;
            aPolylinePlus.b = 255;
            
            somePolylinePluses.push_back(aPolylinePlus);
        }
        
        addFrameToCurrentRecording (somePolylinePluses);
    }
    
    stopRecording();
}



