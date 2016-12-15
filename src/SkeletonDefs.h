//
//  SkeletonDefs.h

#pragma once

struct PolylinePlus {
	ofPolyline polyline; // assume there will always be a field called 'polyline'
	float r;
	float g;
	float b;
};

#define		THE_LIVE_DRAWING	0
#define		PLAYBACK_DRAWING	1

