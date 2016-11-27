//
//  SkeletonTracer.h
//  GPP2016_Display
//
//  Created by GL on 11/27/16.
//
//

#ifndef __GPP2016_Display__SkeletonTracer__
#define __GPP2016_Display__SkeletonTracer__

#include "ofMain.h"
#include "ofxCv.h" // for Tracker
#include "ofxGui.h"

//---------------------------------------------
#define SKEL_INVALID						-1
#define SKEL_N_WORTHWHILE_PIXELS_THRESH		32
#define SKEL_MAX_N_BONES					512
#define SKEL_MAX_N_BRANCH_POINTS			256

typedef enum SkelType {
	SKEL_TYPE_EMPTY 			= 0,
	SKEL_TYPE_UNLOOKED_AT		= 1,
	SKEL_TYPE_LOOKED_AT 		= 2,
	SKEL_TYPE_BRANCH_POINT		= 3,
	SKEL_TYPE_INVALID			= -1
} SkelType;

typedef enum SkelColor {
	SKEL_COLOR_EMPTY        = 0,
	SKEL_COLOR_LOOKED_AT    = 64,
	SKEL_COLOR_BRANCH_POINT = 128,
	SKEL_COLOR_UNLOOKED_AT  = 255
} SkelColor;

typedef enum BoneTerminus {
	BONE_START = 0,
	BONE_END   = 1
};

struct BoneMergeCouplet {
	int				boneIIndex;
	BoneTerminus	boneITerminus;
	int				boneJIndex;
	BoneTerminus	boneJTerminus;
};


class SkeletonTracer {
	
	public:
	
	void	initialize (int w, int h);
	void	computeVectorSkeleton (unsigned char* skeletonPixelBuffer);
	void	traceVectorSkeletonFromSkeletonImage();
	void	trackBones();
	void	mergeBones(); 
	void	smoothBones();
	void	optimallyReorderBones();
	void	drawStateImage();
	void	drawBones();
	
	ofPolyline			currentBone;
	vector<ofPolyline>	bonesRawTraced;		// 1. The raw traced bones
	vector<ofPolyline>	bonesRawMerged;		// 2. Ultra-short bones appended to neighbors.
	vector<ofPolyline>	bonesRawSmooth;		// 3. Visually filtered bones (resampled, smoothed).
	vector<ofPolyline>	bonesPrev;			// Yesterframe's raw traced bones
	
	ofxFloatSlider	boneResampling;
	ofxFloatSlider	boneSmoothing;
	
	int				buffer_w;
	int				buffer_h;
	unsigned char	*tmpBuffer;
	unsigned char	*pixelStateBuffer;
	ofImage			pixelStateImage;
	int				boundary0;
	int				boundary1;
	
	int				*branchPointIndices;
	int				nBranchPointIndices;
	
	void			addPointToCurrentBoneAtPixelIndex (int loc);
	int				countUnlookedAtPixels();
	int				getLocOfFirstSkeletonPixel();
	void			markLocationWithState (int loc, SkelType state);
	SkelType		getTypeOfPixelAtLoc (int loc);
	int				getLocOfFirstUnlookedAtNeighbor (int loc);
	int				countUnlookedAtNeighbors (int loc);
	bool			checkIfAnyNeighborsAreBranchPoints (int loc);
	
};


#endif /* defined(__GPP2016_Display__SkeletonTracer__) */
