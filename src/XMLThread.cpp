#include "XMLThread.h"



XMLThread::XMLThread(){
    
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
    waitForThread(true);
}

void XMLThread::loadXml(string filename){
    lock();
    loadBuffer.push_back(filename);
    cout<<"loading "<<filename<<endl;
    unlock();
}
void XMLThread::saveXml(ofxXmlSettings xml, string filename, bool zip){
    XMLFile newFile;
    newFile.mXml = xml;
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
    cout<<"loading "<<xmlFilename<<endl;
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
    
    bool fileSaved = false;
    
    if (xmlFile.bZip){
        
        // Create string version of the XML data.
        string stringVersionOfXML;
        xmlFile.mXml.copyXmlToString(stringVersionOfXML);
        
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
        fileSaved = xmlFile.mXml.saveFile(xmlFile.mPath);
        
        // Report and tidy up.
    }
    
    if(fileSaved){
        ofNotifyEvent(xmlSaved, xmlFile.mPath, this);
    }
    saveBuffer.pop_front();
}