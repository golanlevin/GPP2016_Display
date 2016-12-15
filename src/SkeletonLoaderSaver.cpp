#include "SkeletonLoaderSaver.h"


//--------------------------------------------------------------

SkeletonLoaderSaver::SkeletonLoaderSaver(){
    
}

SkeletonLoaderSaver::~SkeletonLoaderSaver(){
    
}

int SkeletonLoaderSaver::outputFileCounter = 0;

//--------------------------------------------------------------
void SkeletonLoaderSaver::initialize(int w, int h){

    maxNRecordingFrames = 1000;
    bDeleteOldestFramesWhenRecording = true;
    bUseZippedFiles = true;
    
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

		int outn = SkeletonLoaderSaver::outputFileCounter;
        string xmlFilename = "recordings/recording_" + ofToString(outn) + ".xml";
        if (bUseZippedFiles){ xmlFilename += ".zip"; }
        saveXMLRecording (xmlFilename, bUseZippedFiles);
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
    SkeletonLoaderSaver::outputFileCounter++;
}

void SkeletonLoaderSaver::recordingLoaded(ofxXmlSettings & xml){
     ofLog(OF_LOG_NOTICE)<<"!!!!!!!!!!!!!!!!!!!!!!   loaded"<<endl;
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



