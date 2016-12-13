#include "DisplaySettingsManager.h"



//====================================================================================
DisplaySettingsManager::DisplaySettingsManager() {
	b_loaded_settings = false;
	loadSettings();
}

//====================================================================================
bool DisplaySettingsManager::hasLoaded(){
	return b_loaded_settings;
}

//====================================================================================
void DisplaySettingsManager::saveSettings(){
	
	ofxXmlSettings xml;
	
	xml.addTag("PARAMS");
	xml.pushTag("PARAMS");
	
		xml.addTag("DISPLAY_OUTPUT_QUAD");
		xml.pushTag("DISPLAY_OUTPUT_QUAD");
			xml.addValue("X0", (double)DOQ.pts[0][0]);
			xml.addValue("Y0", (double)DOQ.pts[0][1]);
			xml.addValue("X1", (double)DOQ.pts[1][0]);
			xml.addValue("Y1", (double)DOQ.pts[1][1]);
			xml.addValue("X2", (double)DOQ.pts[2][0]);
			xml.addValue("Y2", (double)DOQ.pts[2][1]);
			xml.addValue("X3", (double)DOQ.pts[3][0]);
			xml.addValue("Y3", (double)DOQ.pts[3][1]);
		xml.popTag();
		
	xml.popTag(); // PARAMS

	xml.saveFile("settings/quad_settings.xml");
	ofLogNotice() << "displaySettingsManager: Saved settings to quad_settings.xml";
}


//====================================================================================
void DisplaySettingsManager::loadSettings(){
	
	ofLogLevel(OF_LOG_NOTICE);
	ofxXmlSettings xml;

	if(!xml.loadFile ( ofToDataPath("settings/quad_settings.xml")  )) {
        ofLogError() << "displaySettingsManager: loading settings failed: "
             << "line " <<  xml.doc.ErrorRow() << ", " << (std::string) xml.doc.ErrorDesc();
		return;
    }
	
	int		defaultIntValue = 0;
	float	defaultFloatValue = 0.0f;
	bool	defaultBoolValue = false;
	char	temp[128];
	int		x0, y0;
	
	xml.pushTag("PARAMS");
	
		xml.pushTag("DISPLAY_OUTPUT_QUAD");
			for (int i=0; i<4; i++){
				DOQ.pts[i][0] = xml.getValue("X"+ofToString(i), defaultFloatValue);
				DOQ.pts[i][1] = xml.getValue("Y"+ofToString(i), defaultFloatValue);
				//ofLog(OF_LOG_NOTICE, "DISPLAY_OUTPUT_QUAD %i: %d %d\n", i, (DOQ.pts[i][0]), (DOQ.pts[i][1]));
			}
		xml.popTag();

	xml.popTag();
	
 	b_loaded_settings = true;
	ofLog(OF_LOG_NOTICE, "DisplaySettingsManager: Loaded settings");
}

