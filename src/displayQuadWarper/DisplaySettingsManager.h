#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"


struct DisplayOutputQuad {
	float pts[4][2];
};

class DisplaySettingsManager {

	public:

 		DisplaySettingsManager();
 		void 	loadSettings();
 		void	saveSettings();

 		DisplayOutputQuad 		DOQ;
 		bool					hasLoaded();

 	private:

		bool	b_loaded_settings;
};
