#include "DisplayQuadWarper.h"

//=================================================================================
DisplayQuadWarper::DisplayQuadWarper (DisplaySettingsManager *dsm){

	DSM = dsm;
	lastCalibrationTime = ofGetElapsedTimeMillis();

	bClamp01 = true;
	b_calibrated = false;
	b_draw_boundary = false;
	fetchSettings();

	outPoint = new float[2];
	outPoint[0] = INVALID_WARP_RESULT;
	outPoint[1] = INVALID_WARP_RESULT;

	calibration_mode = NOT_CALIBRATING_DISPLAY_WARP;
}


//===============================================================================================
void DisplayQuadWarper::fetchSettings(){
	if (DSM != NULL){
		if (DSM->hasLoaded()){

			DisplayOutputQuad doq = DSM->DOQ;

			for (int i=0; i<4; i++) {
				calibration_points[i][0] = (double)(doq.pts[i][0]);
				calibration_points[i][1] = (double)(doq.pts[i][1]);
			}

			b_calibrated = true;
		} else {
			ofLogError() << "displayQuadWarper: fetchSettings() failed; DSM->hasLoaded() is false.";
		}
	} else {
		ofLogError() << "displayQuadWarper: fetchSettings() failed; DSM is NULL.";
	}
}



//===============================================================================================
void DisplayQuadWarper::saveCalibration(){
	// modify the settingManager's internal variables.
	if (DSM != NULL){

		for (int i=0; i<4; i++) {
			DSM->DOQ.pts[i][0] = (int) calibration_points[i][0];
			DSM->DOQ.pts[i][1] = (int) calibration_points[i][1];
		}

		DSM->saveSettings();
		ofLogNotice()  << "DQW Calibration Written";
	} else {
        ofLogWarning() << "DQW Calibration NOT Written, DSM is null";
	}
}



//===============================================================================================
bool DisplayQuadWarper::isCalibrated(){
	return b_calibrated;
}

bool DisplayQuadWarper::isCalibrating(){
	return ((calibration_mode != NOT_CALIBRATING_DISPLAY_WARP));
}

//===============================================================================================
float *DisplayQuadWarper::getCalibrationPoint(int which){
	if (b_calibrated && (which >=0) && (which < 4)){
		outPoint[0] = calibration_points[which][0];
		outPoint[1] = calibration_points[which][1];
	} else {
		ofLogError() << "displayQuadWarper: getCalibrationPoint() failed";
		outPoint[0] = INVALID_WARP_RESULT;
		outPoint[1] = INVALID_WARP_RESULT;
	}
	return outPoint;
}


//===============================================================================================
float* DisplayQuadWarper::warpPointInRect (float inx, float iny, float inw, float inh){

	// given a point (inx,iny) within the rect(0,0,inw,inh), return a
	// pair of floats which represents the warped version of that point.

	if (b_calibrated && (inw!=0) && (inh!=0)) {

		float x1 = (float) calibration_points[0][0];
		float y1 = (float) calibration_points[0][1];
		float x2 = (float) calibration_points[1][0];
		float y2 = (float) calibration_points[1][1];
		float x3 = (float) calibration_points[2][0];
		float y3 = (float) calibration_points[2][1];
		float x4 = (float) calibration_points[3][0];
		float y4 = (float) calibration_points[3][1];

		float a = inx / inw;	// in the range [0..1]
		float b = iny / inh;	// in the range [0..1]
		float ainv = 1.0f - a;	// in the range [0..1]
		float binv = 1.0f - b;	// in the range [0..1]

		float xi = (ainv*x2) + (a*x3);
		float yi = (ainv*y2) + (a*y3);

		float xj = (ainv*x1) + (a*x4);
		float yj = (ainv*y1) + (a*y4);

		float outx = (binv*xi) + (b*xj);
		float outy = (binv*yi) + (b*yj);

		outPoint[0] = outx;
		outPoint[1] = outy;

	} else {

		// ofLogError() << "displayQuadWarper: warpPointInRect() failed; no calibration exists!";
		outPoint[0] = INVALID_WARP_RESULT;
		outPoint[1] = INVALID_WARP_RESULT;
	}
	return outPoint;
}



//===============================================================================================
void DisplayQuadWarper::drawBoundary(){
	if ((calibration_mode != NOT_CALIBRATING_DISPLAY_WARP) || b_draw_boundary){
		
		float x1 = (float) calibration_points[0][0];
		float y1 = (float) calibration_points[0][1];
		float x2 = (float) calibration_points[1][0];
		float y2 = (float) calibration_points[1][1];
		float x3 = (float) calibration_points[2][0];
		float y3 = (float) calibration_points[2][1];
		float x4 = (float) calibration_points[3][0];
		float y4 = (float) calibration_points[3][1];

		ofSetLineWidth(1.0f);
		ofSetColor(255);
		ofNoFill();
		ofBeginShape();
		ofVertex  (x1, y1);
		ofVertex  (x2, y2);
		ofVertex  (x3, y3);
		ofVertex  (x4, y4);
		ofEndShape(true);
		
		float dx = 0.06 * ((x1+x3)/2.0 - x1);
		float dy = 0.06 * ((y1+y3)/2.0 - y1);

		ofSetColor(255, 255, 127);
		ofDrawBitmapString("1", x1+dx, y1+dy);
		ofDrawBitmapString("2", x2+dx, y2-dy);
		ofDrawBitmapString("3", x3-dx, y3-dy);
		ofDrawBitmapString("4", x4-dx, y4+dy);

		if ((calibration_mode >= 0) && (calibration_mode < 4)){
			float cx = calibration_points[calibration_mode][0];
			float cy = calibration_points[calibration_mode][1];
			float cr = 0.020;
			
			ofSetColor(255, 255, 127);
			ofDrawCircle(cx,cy,cr); // Draw a circle
		}

    }
}

//===============================================================================================
void DisplayQuadWarper::mouseDown(float x01, float y01){
	if ((calibration_mode >= 0) && (calibration_mode < 4)){
		setCalibrationPoint(x01, y01, calibration_mode);
		lastCalibrationTime = ofGetElapsedTimeMillis();
	}
}

//===============================================================================================
void DisplayQuadWarper::setCalibrationMode(int m){
	if ((m >= 0) && (m < 4)){
		calibration_mode = m;
		lastCalibrationTime = ofGetElapsedTimeMillis();
	} else {
		calibration_mode = NOT_CALIBRATING_DISPLAY_WARP;
	}
}

//===============================================================================================
void DisplayQuadWarper::setCalibrationPoint(float x, float y, int which){
	if ((which >= 0) && (which < 4)){
		if (bClamp01) {
			x = ofClamp(x, 0,1);
			y = ofClamp(y, 0,1);
		}
		calibration_points[which][0] = x;
		calibration_points[which][1] = y;
	}
}
