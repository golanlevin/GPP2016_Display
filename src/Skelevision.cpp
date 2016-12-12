#include "Skelevision.h"

//--------------------------------------------------------------
void Skelevision::initialize(){
	
	readXML = new ofxXmlSettings();
	readXML->clear();
	
	saveXML = new ofxXmlSettings();
	saveXML->clear();
	
	currentPlaybackFrameIndex = 0;
	currentPlaybackFrames.clear();
	currentRecordingFrames.clear();
	
	bLoadedFileFromXML	= false;
	outputFileCounter	= 0;
	
	string xmlFilename			= "recordings/recording_test.xml";
	string zippedXmlFilename	= "recordings/recording_test.xml.zip";
	
	generateBogusFrames();					// Put lissajous into currentRecordingFrames
	transferCurrentRecordingToXML();		// Transfer currentRecordingFrames into saveXML
	saveXMLRecording(xmlFilename, false);	// Save from saveXML to filename.
	

	loadXMLRecording (xmlFilename, false);
	transferFromXmlToCurrentDrawing();
	
}


//--------------------------------------------------------------
void Skelevision::loadXMLRecording (string &xmlFilename, bool bFileIsZipped){
	// Loads a recording into the XML ofxXMLSettings object from a zipped (or not) file.
	ofSetLogLevel(OF_LOG_NOTICE);
	long then = ofGetElapsedTimeMicros();
	
	bLoadedFileFromXML = false;
	readXML->clear();
	
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
		bool bLoadOkay = readXML->loadFromBuffer(unzippedXMLString);
		int elapsed = (int)(ofGetElapsedTimeMicros() - then);
		ofLog(OF_LOG_NOTICE, "Read zipped XML: bLoadOkay = %d, took %d", (int)bLoadOkay, elapsed);
		bLoadedFileFromXML = bLoadOkay;
		
	} else { // if the input file is not zipped
	
		bool bLoadOkay = readXML->loadFile(xmlFilename); // Read data from XML file on disk
		int elapsed = (int)(ofGetElapsedTimeMicros() - then);
		ofLog(OF_LOG_NOTICE, "Read from XML: bLoadOkay = %d, took %d", (int)bLoadOkay, elapsed);
		bLoadedFileFromXML = bLoadOkay;
	}
}


//--------------------------------------------------------------
void Skelevision::transferFromXmlToCurrentDrawing(){
	// loadXMLRecording only loads data into the XML ofxXmlSettings object.
	// Here, we copy the data from the XML object into currentPlaybackFrames.
	
	currentPlaybackFrameIndex = 0;
	currentPlaybackFrames.clear();
	
	if (bLoadedFileFromXML){
		ofSetLogLevel(OF_LOG_WARNING);
		
		int nFramesInThisRecording = readXML->getNumTags("FRAME");
		ofLog(OF_LOG_NOTICE, "nFramesInThisRecording = %d", nFramesInThisRecording);
		if(nFramesInThisRecording > 0){
			
			for (int f=0; f<nFramesInThisRecording; f++){
				readXML->pushTag("FRAME", f);
				vector<PolylinePlus> aVectorOfPolylinePluses;
				
				int nStrokesInThisFrame = readXML->getNumTags("STROKE");
				ofLog(OF_LOG_NOTICE, "  nStrokesInThisFrame = %d", nStrokesInThisFrame);
				if (nStrokesInThisFrame > 0){
					
					for (int s=0; s<nStrokesInThisFrame; s++){
						readXML->pushTag("STROKE", s);
						ofPolyline aPolyline;
						
						int nPointsInThisStroke = readXML->getNumTags("PT");
						ofLog(OF_LOG_NOTICE, "    nPointsInThisStroke = %d", nPointsInThisStroke);
						if (nPointsInThisStroke > 0){
							
							for (int i=0; i<nPointsInThisStroke; i++){
								float x = readXML->getValue("PT:X", 0, i);
								float y = readXML->getValue("PT:Y", 0, i);
								aPolyline.addVertex(x, y);
							}
						}
						
						PolylinePlus aPolylinePlus;
						aPolylinePlus.polyline = aPolyline;
						aPolylinePlus.r = 0;
						aPolylinePlus.g = 0;
						aPolylinePlus.b = 255;
						
						aVectorOfPolylinePluses.push_back(aPolylinePlus);
						readXML->popTag(); // STROKE
					}
				}
				currentPlaybackFrames.push_back(aVectorOfPolylinePluses);
				readXML->popTag(); // FRAME
			}
		}
	}
}



//--------------------------------------------------------------
void Skelevision::drawCurrentPlaybackFrame(){

	int nFramesInThisRecording = currentPlaybackFrames.size();
	if ((nFramesInThisRecording > 0) && (bLoadedFileFromXML) &&
		(currentPlaybackFrameIndex < nFramesInThisRecording)){
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);
		
		vector<PolylinePlus> aVectorOfPolylinePluses;
		aVectorOfPolylinePluses = currentPlaybackFrames[currentPlaybackFrameIndex];
		int nStrokesInThisFrame = aVectorOfPolylinePluses.size();
		if (nStrokesInThisFrame > 0){
			
			for (int i=0; i<nStrokesInThisFrame; i++){
				PolylinePlus ithPP = aVectorOfPolylinePluses[i];
				ofPolyline ithPolyline = ithPP.polyline;
				ofSetColor(ithPP.r, ithPP.g, ithPP.b);
				ithPolyline.draw();
			}
		}
		
		// advance currentPlaybackFrameIndex
		currentPlaybackFrameIndex ++;
		currentPlaybackFrameIndex %= nFramesInThisRecording;
		
		glBlendFunc(GL_ONE, GL_ZERO); // Restore default.
	}
}


//--------------------------------------------------------------
void Skelevision::transferCurrentRecordingToXML(){

	// Add currentRecordingFrames data to XML.
	// Performed when currentRecordingFrames is 'done' being recorded.
	
	ofSetLogLevel(OF_LOG_NOTICE); // OF_LOG_WARNING
	ofLog(OF_LOG_NOTICE, "Adding currentRecordingFrames data to XML.");
	
	saveXML->clear();
	int nReadFrames = currentRecordingFrames.size();
	for (int f=0; f<nReadFrames; f++){
		
		int mostRecentFrameTag = saveXML->addTag("FRAME");
		saveXML->pushTag("FRAME", mostRecentFrameTag);
		
		vector<PolylinePlus> fthVectorOfPolylinePluses = currentRecordingFrames[f];
		int nPolylinePlusesInFthFrame = fthVectorOfPolylinePluses.size();
		for (int p=0; p<nPolylinePlusesInFthFrame; p++){
			
			int mostRecentStrokeTag = saveXML->addTag("STROKE");
			saveXML->pushTag("STROKE", mostRecentStrokeTag);
			
			ofPolyline pthPolyline = fthVectorOfPolylinePluses[p].polyline;
			int nPointsInPthPolyline = pthPolyline.size();
			for (int i=0; i<nPointsInPthPolyline; i++){
				
				ofPoint ithPointInPthPolyline = pthPolyline[i];
				float x = ithPointInPthPolyline.x;
				float y = ithPointInPthPolyline.y;
				
				int mostRecentPointTag = saveXML->addTag("PT");
				saveXML->setValue("PT:X", x, mostRecentPointTag);
				saveXML->setValue("PT:Y", y, mostRecentPointTag);
			}
			saveXML->popTag(); // mostRecentStrokeTag
		}
		saveXML->popTag(); // mostRecentFrameTag
	}
}



//--------------------------------------------------------------
void Skelevision::saveXMLRecording (string &xmlFilename, bool bSaveAsZipped){
	ofSetLogLevel(OF_LOG_NOTICE); // OF_LOG_WARNING
	long then = ofGetElapsedTimeMicros();
	
	if (bSaveAsZipped){
		
		// Create string version of the XML data.
		string stringVersionOfXML;
		saveXML->copyXmlToString(stringVersionOfXML);
		
		// Create a vector of chars version of the string data.
		// http://stackoverflow.com/questions/8247793/converting-stdstring-to-stdvectorchar
		const std::vector<unsigned char> srcVec (stringVersionOfXML.begin(), stringVersionOfXML.end());
		
		// Compress the vector of chars, producing a new vector of chars
		int compressionLevel = 1; // speediest; gives ~5:1
		vector<unsigned char> resultVec = ofxZip::compress( srcVec, compressionLevel );
		
		// Copy the compressed vector of chars to a char array
		int nCharsInZippedXML = resultVec.size();
		char* charsOfZippedXML = (char *) &resultVec[0];
		
		// Put the array of compressed chars into a buffer
		ofBuffer myBufferOfZippedXML;
		myBufferOfZippedXML.allocate(nCharsInZippedXML);
		myBufferOfZippedXML.set(charsOfZippedXML, nCharsInZippedXML);
		
		// Save the buffer to a zipped file
		// such as "recordings/recording_test.xml.zip"
		bool bBinaryWrite = true;
		bool bSavedZippedFile = ofBufferToFile(xmlFilename, myBufferOfZippedXML, bBinaryWrite);
	
		// Report and tidy up.
		int elapsed = (int)(ofGetElapsedTimeMicros() - then);
		ofLog(OF_LOG_NOTICE, "Saved zipped XML file = %d, took %d", (int)bSavedZippedFile, elapsed);
		saveXML->clear();
		
	} else { // bSaveAsZipped is false
		
		// Save the XML data using its internal saveFile method.
		// To a file such as "recordings/recording_test.xml");
		bool bSavedXMLFile = saveXML->saveFile(xmlFilename);
		
		// Report and tidy up.
		int elapsed = (int)(ofGetElapsedTimeMicros() - then);
		ofLog(OF_LOG_NOTICE, "Saved XML file = %d, took %d", (int) bSavedXMLFile, elapsed);
		saveXML->clear();
	}
}

//--------------------------------------------------------------
void Skelevision::generateBogusFrames(){
	//-------------------
	// Create some bogus drawing data.

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
				aPolyline.addVertex(x, y);
			}
			
			PolylinePlus aPolylinePlus;
			aPolylinePlus.polyline = aPolyline;
			aPolylinePlus.r = 0;
			aPolylinePlus.g = 0;
			aPolylinePlus.b = 255;
			
			somePolylinePluses.push_back(aPolylinePlus);
		}
		currentRecordingFrames.push_back(somePolylinePluses);
	}
}



