#pragma once
#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "ofxZip.h"

struct XMLFile{
    ofxXmlSettings mXml;
    string mPath;
    bool bZip;
};

class XMLThread : public ofThread{
public:
    XMLThread();
    ~XMLThread();
    
    void threadedFunction();
    void start();
    void stop();
    
    void loadXml(string filename);
    void saveXml(ofxXmlSettings xml, string filename, bool zip = true);
    
    
    ofEvent<ofxXmlSettings> xmlLoaded;
    ofEvent<string> xmlSaved;
    
protected:
    void threadedLoad();
    void threadedSave();
    
    deque<XMLFile> saveBuffer;
    deque<string> loadBuffer;

    
    ofxXmlSettings readXML;
    ofxXmlSettings saveXML;
};