#include "Skelevision.h"

//--------------------------------------------------------------
void Skelevision::initialize(){
	XML.clear();
}

//--------------------------------------------------------------
void Skelevision::testBuffer(){
	
	//-------------------
	// 1. Create some bogus data.
	ofSetLogLevel(OF_LOG_NOTICE);
	ofLog(OF_LOG_NOTICE, "Generating Bogus skeleton data.");
	
	vector<vector<ofPolyline>> someFrames;
	someFrames.clear();
	int nFramesPerRecording = 4;
	for (int f=0; f<nFramesPerRecording; f++){
		
		int nPolylinesPerFrame = 3;
		vector<ofPolyline> somePolylines;
		for (int p=0; p<nPolylinesPerFrame; p++){
			
			int nPointsPerPolyline = 15;
			ofPolyline aPolyline;
			aPolyline.clear();
			
			float dx = f * 40;
			float dy = p * 30;
			float radiusX = 50 + (f+p)*10;
			float radiusY = 40 + (f+((p+1)*2))*10;
			
			for (int i=0; i<nPointsPerPolyline; i++){
				float t = ofMap(i,0,nPointsPerPolyline, 0,TWO_PI);
				float x = dx + radiusX * cos(t);
				float y = dy + radiusY * sin(t);
				aPolyline.addVertex(x, y);
			}
			somePolylines.push_back(aPolyline);
		}
		someFrames.push_back(somePolylines);
	}
	
	
	//-------------------
	// 2. Add data to XML
	ofSetLogLevel(OF_LOG_NOTICE);
	ofLog(OF_LOG_NOTICE, "Adding bogus data to XML.");
	
	ofxXmlSettings tempXML;
	tempXML.clear();
	int nReadFrames = someFrames.size();
	for (int f=0; f<nReadFrames; f++){
		
		int mostRecentFrameTag = tempXML.addTag("FRAME");
		tempXML.pushTag("FRAME", mostRecentFrameTag);
		
		vector<ofPolyline> fthVectorOfPolylines = someFrames[f];
		int nPolylinesInFthFrame = fthVectorOfPolylines.size();
		for (int p=0; p<nPolylinesInFthFrame; p++){
			
			int mostRecentStrokeTag = tempXML.addTag("STROKE");
			tempXML.pushTag("STROKE", mostRecentStrokeTag);
			
			ofPolyline pthPolyline = fthVectorOfPolylines[p];
			int nPointsInPthPolyline = pthPolyline.size();
			for (int i=0; i<nPointsInPthPolyline; i++){
				
				ofPoint ithPointInPthPolyline = pthPolyline[i];
				float x = ithPointInPthPolyline.x;
				float y = ithPointInPthPolyline.y;
				
				int mostRecentPointTag = tempXML.addTag("PT");
				tempXML.setValue("PT:X", x, mostRecentPointTag);
				tempXML.setValue("PT:Y", y, mostRecentPointTag);
			}
			tempXML.popTag(); // mostRecentStrokeTag
		}
		tempXML.popTag(); // mostRecentFrameTag
	}
	bool bSavedXMLFile = tempXML.saveFile("recordings/frame_test.xml");
	ofLog(OF_LOG_NOTICE, "bSavedXMLFile = %d\n", (int) bSavedXMLFile);
	
	
	bool bExportBufferVersionOfXML = false;
	if (bExportBufferVersionOfXML){ // Skip this for now.
		string stringVersionOfXML;
		tempXML.copyXmlToString(stringVersionOfXML);
		ofBuffer myBufferVersionOfXML;
		int nCharsInStringVersionOfXML = stringVersionOfXML.length();
		const char *charsVersionOfXML = stringVersionOfXML.c_str();
		myBufferVersionOfXML.allocate(nCharsInStringVersionOfXML);
		myBufferVersionOfXML.set(charsVersionOfXML, nCharsInStringVersionOfXML);
		bool bBinaryWrite = true;
		bool fileWritten = ofBufferToFile("recordings/frame_test.dat", myBufferVersionOfXML, bBinaryWrite);
		printf("Wrote file = %d\n", (int) fileWritten);
	}
	
	
	//-------------------
	// 3. Read data from XML
	ofSetLogLevel(OF_LOG_NOTICE);
	XML.clear();
	bool bLoadOkay = XML.loadFile("recordings/frame_test.xml");
	ofLog(OF_LOG_NOTICE, "Reading from XML: bLoadOkay = %d\n", (int) bLoadOkay);
	
	currentPlaybackFrameIndex = 0;
	currentPlaybackFrames.clear();
	int nFramesInThisRecording = XML.getNumTags("FRAME");
	ofLog(OF_LOG_NOTICE, "nFramesInThisRecording = %d", nFramesInThisRecording);
	if(nFramesInThisRecording > 0){
		
		for (int f=0; f<nFramesInThisRecording; f++){
			XML.pushTag("FRAME", f);
			vector<ofPolyline> aVectorOfPolylines;
			
			int nStrokesInThisFrame = XML.getNumTags("STROKE");
			ofLog(OF_LOG_NOTICE, "  nStrokesInThisFrame = %d", nStrokesInThisFrame);
			if (nStrokesInThisFrame > 0){
				
				for (int s=0; s<nStrokesInThisFrame; s++){
					XML.pushTag("STROKE", s);
					ofPolyline aPolyline;
					
					int nPointsInThisStroke = XML.getNumTags("PT");
					ofLog(OF_LOG_NOTICE, "    nPointsInThisStroke = %d", nPointsInThisStroke);
					if (nPointsInThisStroke > 0){
						
						for (int i=0; i<nPointsInThisStroke; i++){
							float x = XML.getValue("PT:X", 0, i);
							float y = XML.getValue("PT:Y", 0, i);
							aPolyline.addVertex(x, y);
						}
					}
					aVectorOfPolylines.push_back(aPolyline);
					XML.popTag(); // STROKE
				}
			}
			currentPlaybackFrames.push_back(aVectorOfPolylines);
			XML.popTag(); // FRAME
		}
	}
}


//--------------------------------------------------------------
void Skelevision::drawCurrentPlaybackFrame(){
	
	int nFramesInThisRecording = currentPlaybackFrames.size();
	if ((nFramesInThisRecording > 0) &&
		(currentPlaybackFrameIndex < nFramesInThisRecording)){
		
		vector<ofPolyline> aVectorOfPolylines;
		aVectorOfPolylines = currentPlaybackFrames[currentPlaybackFrameIndex];
		int nStrokesInThisFrame = aVectorOfPolylines.size();
		if (nStrokesInThisFrame > 0){
			
			
			
		}
	}
}