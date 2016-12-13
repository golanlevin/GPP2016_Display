#include "SkeletonLoaderSaver.h"


//--------------------------------------------------------------

SkeletonLoaderSaver::SkeletonLoaderSaver(){
    
}

SkeletonLoaderSaver::~SkeletonLoaderSaver(){
    
}
//
//void SkeletonLoaderSaver::threadedFunction(){
//    while(isThreadRunning()){
//        sleep(1);
//    }
//}
//
//void SkeletonLoaderSaver::stop(){
//    waitForThread(true);
//}
//
//void SkeletonLoaderSaver::start(){
//    startThread();
//}

//--------------------------------------------------------------
void SkeletonLoaderSaver::initialize(int w, int h){

    maxNRecordingFrames = 100;
    bDeleteOldestFramesWhenRecording = true;
    bUseZippedFiles = true;
    
    readXML.clear();
    
    saveXML.clear();
    
    currentPlaybackFrameIndex = 0;
    currentPlaybackDrawing.clear();
    currentPlaybackFrames.clear();
    currentRecordingFrames.clear();
    
    bCurrentlyRecording	= false;
    bLoadedFileFromXML	= false;
    bPlaybackPaused		= false;
    outputFileCounter	= 0;
    
    bUseNormalizedDrawings = true;
    buffer_w = w;
    buffer_h = h;


    
    xmlThread.start();
    
    ofAddListener(xmlThread.xmlSaved, this,  &SkeletonLoaderSaver::recordingSaved);
    ofAddListener(xmlThread.xmlLoaded, this,  &SkeletonLoaderSaver::recordingLoaded);
}


//--------------------------------------------------------------
void SkeletonLoaderSaver::startRecording(){
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
    stopRecording();
    if (currentRecordingFrames.size() > 0){
        transferCurrentRecordingToXML();
        
        string xmlFilename = "recordings/recording_" + ofToString(outputFileCounter) + ".xml";
        if (bUseZippedFiles){ xmlFilename += ".zip"; }
        saveXMLRecording (xmlFilename, bUseZippedFiles);
        outputFileCounter++;
    }
}

//---------------------------------
bool SkeletonLoaderSaver::isRecording(){
    return bCurrentlyRecording;
}

//---------------------------------
int SkeletonLoaderSaver::getCurrentRecordingLength(){
    return (currentRecordingFrames.size());
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
void SkeletonLoaderSaver::loadAndInitiatePlaybackOfRecording (int which){
    
    if(savedFiles.size() >= which && savedFiles.size() > 0){
        string xmlFilename = savedFiles[which];
        cout<<"loading "<<xmlFilename<<endl;
        loadXMLRecording(xmlFilename, bUseZippedFiles);
    }
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::loadXMLRecording (string &xmlFilename, bool bFileIsZipped){
    // Loads a recording into the XML ofxXMLSettings object from a zipped (or not) file.
    
    xmlThread.loadXml(xmlFilename);
    
    
}


void SkeletonLoaderSaver::recordingSaved(string & filename){
    ofLog(OF_LOG_NOTICE)<<"!!!!!!!!!!!!!!!!!!!!!!  "<<filename<<" saved"<<endl;
    savedFiles.push_back(filename);
}

void SkeletonLoaderSaver::recordingLoaded(ofxXmlSettings & xml){
    readXML = xml;
    bLoadedFileFromXML = true;
    transferFromXmlToCurrentDrawing();
    
}

//--------------------------------------------------------------
void SkeletonLoaderSaver::transferFromXmlToCurrentDrawing(){
    // loadXMLRecording only loads data into the XML ofxXmlSettings object.
    // Here, we copy the data from the XML object into currentPlaybackFrames.
    
    currentPlaybackFrameIndex = 0;
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
                        aPolylinePlus.r = 0;
                        aPolylinePlus.g = 0;
                        aPolylinePlus.b = 255;
                        
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
            
            currentPlaybackFrameIndex %= nFramesInThisRecording; // Safety check.
            currentPlaybackDrawing = currentPlaybackFrames[currentPlaybackFrameIndex];
            
            // advance currentPlaybackFrameIndex
            if (!bPlaybackPaused){
                currentPlaybackFrameIndex ++;
                currentPlaybackFrameIndex %= nFramesInThisRecording;
            }
        }
    }
}



//--------------------------------------------------------------
// This function is soon to be retired.
//
void SkeletonLoaderSaver::drawCurrentPlaybackFrame(){
    
    
    int nFramesInThisRecording = currentPlaybackFrames.size();
    if ((nFramesInThisRecording > 0) && (bLoadedFileFromXML) &&
        (currentPlaybackFrameIndex < nFramesInThisRecording)){
        
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        
        vector<PolylinePlus> aVectorOfPolylinePluses;
        aVectorOfPolylinePluses = currentPlaybackFrames[currentPlaybackFrameIndex];
        int nStrokesInThisFrame = aVectorOfPolylinePluses.size();
        if (nStrokesInThisFrame > 0){
            
            ofPushMatrix();
            if (bUseNormalizedDrawings){ ofScale(buffer_w, buffer_w); }
            
            // Draw the currentPlayback Frame to the screen
            for (int i=0; i<nStrokesInThisFrame; i++){
                PolylinePlus ithPP = aVectorOfPolylinePluses[i];
                ofPolyline ithPolyline = ithPP.polyline;
                ofSetLineWidth(2.0);
                ofSetColor(ithPP.r, ithPP.g, ithPP.b);
                ithPolyline.draw();
            }
            
            ofPopMatrix();
        }
        
        // advance currentPlaybackFrameIndex
        if (!bPlaybackPaused){
            currentPlaybackFrameIndex ++;
            currentPlaybackFrameIndex %= nFramesInThisRecording;
        }
        
        glPopAttrib(); // Restore blend mode default.
    }
}


//--------------------------------------------------------------
void SkeletonLoaderSaver::transferCurrentRecordingToXML(){
    
    // Add currentRecordingFrames data to XML.
    // Performed when currentRecordingFrames is 'done' being recorded.
    
    ofSetLogLevel(OF_LOG_NOTICE); // OF_LOG_WARNING
    ofLog(OF_LOG_NOTICE, "Adding currentRecordingFrames data to XML.");
    
    saveXML.clear();
    int nReadFrames = currentRecordingFrames.size();
    for (int f=0; f<nReadFrames; f++){
        
        int mostRecentFrameTag = saveXML.addTag("FRAME");
        saveXML.pushTag("FRAME", mostRecentFrameTag);
        
        vector<PolylinePlus> fthVectorOfPolylinePluses = currentRecordingFrames[f];
        int nPolylinePlusesInFthFrame = fthVectorOfPolylinePluses.size();
        for (int p=0; p<nPolylinePlusesInFthFrame; p++){
            
            int mostRecentStrokeTag = saveXML.addTag("STROKE");
            saveXML.pushTag("STROKE", mostRecentStrokeTag);
            
            ofPolyline pthPolyline = fthVectorOfPolylinePluses[p].polyline;
            int nPointsInPthPolyline = pthPolyline.size();
            for (int i=0; i<nPointsInPthPolyline; i++){
                
                ofPoint ithPointInPthPolyline = pthPolyline[i];
                double x = ithPointInPthPolyline.x;
                double y = ithPointInPthPolyline.y;
                
                int mostRecentPointTag = saveXML.addTag("PT");
                saveXML.setValue("PT:X", (double) x, mostRecentPointTag);
                saveXML.setValue("PT:Y", (double) y, mostRecentPointTag);
            }
            saveXML.popTag(); // mostRecentStrokeTag
        }
        saveXML.popTag(); // mostRecentFrameTag
    }
}


//--------------------------------------------------------------
void SkeletonLoaderSaver::saveXMLRecording (string &xmlFilename, bool bSaveAsZipped)
{
    xmlThread.saveXml(saveXML, xmlFilename, bSaveAsZipped);
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



