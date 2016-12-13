#pragma once

#include "ofMain.h"
#include "DisplaySettingsManager.h"

#define NOT_CALIBRATING_DISPLAY_WARP 	999
#define INVALID_WARP_RESULT				-1.0f
#define DQW_CALIBRATING_TIMEOUT			20000

class DisplayQuadWarper {

public:

	DisplayQuadWarper(DisplaySettingsManager *dsm);
	DisplaySettingsManager *DSM;
	void fetchSettings();

	// given a point (inx,iny) within the rect(0,0,inw,inh), return a
	// pair of floats which represents the warped version of that point.
	float* 	warpPointInRect(float inx, float iny, float inw, float inh);
	float* 	getCalibrationPoint(int which);

	void 		readCalibration();
	void 		saveCalibration();
	void 		setCalibrationMode (int m);
	void 		setCalibrationPoint(float x, float y, int which);
	char		*calibration_filename;

	int			calibration_mode;
	void 		mouseDown(float x, float y);
	void 		mouseUp  (float x, float y);
	bool 		isCalibrated();
	bool		isCalibrating();
	long 		lastCalibrationTime;

	float*		outPoint;
	
	bool		bClamp01;
	bool		b_draw_boundary;
	void 		drawBoundary();
	double		calibration_points	[4][2];


private:
	bool		b_calibrated;
	double		coeffs				[2][4];

};
