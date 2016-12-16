#include "XMLThread.h"



XMLThread::XMLThread(){
    saveXML = new ofxXmlSettings();
    saveXML->clear();
}
XMLThread::~XMLThread(){

}

void XMLThread::threadedFunction(){
    while(isThreadRunning()){
        lock();
        if(saveBuffer.size() > 0){
            threadedSave();
        }
        unlock();
        
        lock();
        if(loadBuffer.size() > 0){
            
            threadedLoad();
            
        }
        unlock();
		
		yield();
        // usleep(1);
    }
}

void XMLThread::start(){
    startThread(false);
}
void XMLThread::stop(){
	ofLog(OF_LOG_WARNING, "Stopping XMLThread.");
    waitForThread(true);
}

void XMLThread::loadXml(string filename){
    lock();
    loadBuffer.push_back(filename);
    // cout<<"loading "<<filename<<endl;
    unlock();
}
void XMLThread::saveXml(vector<vector<PolylinePlus> > lines, string filename, bool zip){
    XMLFile newFile;
    newFile.mLines = lines;
    newFile.mPath = filename;
    newFile.bZip = zip;
    
    lock();
    saveBuffer.push_back(newFile);
    unlock();
}

void XMLThread::threadedLoad(){
    string xmlFilename = loadBuffer.front();
    bool bLoadedFileFromXML = false;
    readXML.clear();
    cout<<" loading "<<xmlFilename<<endl;
    bool bFileIsZipped = ofIsStringInString(xmlFilename, "zip");
    if (bFileIsZipped){
        // Load *zipped* XML into an ofBuffer
        bool bIsBinary = true;
        ofBuffer myBufferOfZippedXML = ofBufferFromFile(xmlFilename, bIsBinary);
        
        // Fetch char-array of zipped CML
        char *charArrayOfZippedXML = myBufferOfZippedXML.getData();
        int nZippedChars = myBufferOfZippedXML.size();
        
        // Convert char-array into vector of unsigned chars; uncompress.
        vector<unsigned char> zippedVec (charArrayOfZippedXML, charArrayOfZippedXML + nZippedChars);
        vector<unsigned char> unzippedVec = ofxZip::uncompress( zippedVec );
        
        // Convert vector of unsigned chars into string of (unzipped) XML.
        string unzippedXMLString (unzippedVec.begin(), unzippedVec.end());
        
        // Load string from readXML into ofxXmlSettings.
        bLoadedFileFromXML = readXML.loadFromBuffer(unzippedXMLString);
        
    } else { // if the input file is not zipped
        bLoadedFileFromXML = readXML.loadFile(xmlFilename);
    }
    
    if(bLoadedFileFromXML){
        ofNotifyEvent(xmlLoaded, readXML, this);
    }
    
    //remove from buffer
    loadBuffer.pop_front();
}
void XMLThread::threadedSave(){
    XMLFile xmlFile = saveBuffer.front();
    
    
    
    // Add currentRecordingFrames data to XML.
    // Performed when currentRecordingFrames is 'done' being recorded.
    
    ofSetLogLevel(OF_LOG_NOTICE); // OF_LOG_WARNING
    ofLog(OF_LOG_NOTICE, "Adding currentRecordingFrames data to XML.");
    
    saveXML->clear();
    int nReadFrames = xmlFile.mLines.size();
    for (int f=0; f<nReadFrames; f++){
        
        int mostRecentFrameTag = saveXML->addTag("FRAME");
        saveXML->pushTag("FRAME", mostRecentFrameTag);
        
        vector<PolylinePlus> fthVectorOfPolylinePluses =  xmlFile.mLines[f];
        int nPolylinePlusesInFthFrame = fthVectorOfPolylinePluses.size();
        for (int p=0; p<nPolylinePlusesInFthFrame; p++){
            
            int mostRecentStrokeTag = saveXML->addTag("STROKE");
            saveXML->pushTag("STROKE", mostRecentStrokeTag);
            
            ofPolyline pthPolyline = fthVectorOfPolylinePluses[p].polyline;
            int nPointsInPthPolyline = pthPolyline.size();
            for (int i=0; i<nPointsInPthPolyline; i++){
                
                ofPoint ithPointInPthPolyline = pthPolyline[i];
                double x = ithPointInPthPolyline.x;
                double y = ithPointInPthPolyline.y;
                
                int mostRecentPointTag = saveXML->addTag("PT");
                saveXML->setValue("PT:X", (double) x, mostRecentPointTag);
                saveXML->setValue("PT:Y", (double) y, mostRecentPointTag);
            }
            saveXML->popTag(); // mostRecentStrokeTag
        }
        saveXML->popTag(); // mostRecentFrameTag
    }
    
    
    bool fileSaved = false;
    
    if (xmlFile.bZip){
        
        // Create string version of the XML data.
        string stringVersionOfXML;
        saveXML->copyXmlToString(stringVersionOfXML);
        
        // Create a vector of chars version of the string data.
        // http://stackoverflow.com/questions/8247793/converting-stdstring-to-stdvectorchar
        const std::vector<unsigned char> srcVec (stringVersionOfXML.begin(), stringVersionOfXML.end());
        
        // Compress the vector of chars, producing a new vector of chars
        int compressionLevel = 1; // speediest; gives ~5:1 compression ratio
        vector<unsigned char> resultVec = ofxZip::compress( srcVec, compressionLevel );
        
        // Copy the compressed vector of chars to a char array
        int nCharsInZippedXML = resultVec.size();
        char* charsOfZippedXML = (char *) &resultVec[0];
        
        // Put that array of (compressed) chars into an ofBuffer
        ofBuffer myBufferOfZippedXML;
        myBufferOfZippedXML.allocate(nCharsInZippedXML);
        myBufferOfZippedXML.set(charsOfZippedXML, nCharsInZippedXML);
        
        // Save the ofBuffer to a zipped file
        // (such as "recordings/recording_test.xml.zip")
        bool bBinaryWrite = true;
        fileSaved = ofBufferToFile(xmlFile.mPath, myBufferOfZippedXML, bBinaryWrite);
        
        
    } else { // bSaveAsZipped is false
        
        // Save the XML data using its internal saveFile method
        // (to a file such as "recordings/recording_test.xml")
        fileSaved = saveXML->saveFile(xmlFile.mPath);
        
        // Report and tidy up.
    }
    
    if(fileSaved){
        ofNotifyEvent(xmlSaved, xmlFile.mPath, this);
    }
    saveBuffer.pop_front();
}