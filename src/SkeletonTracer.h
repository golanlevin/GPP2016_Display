//
//  SkeletonTracer.h
//  GPP2016_Display
//
//  Created by GL on 11/27/16.
//
//

#pragma once


#include "ofMain.h"
#include "LaserMain.h"
#include "LaserDefs.h"

#include "ofxCv.h" // for Tracker
#include "ofxGui.h"
#include "ofxVectorGraphics.h"


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
} BoneTerminus;

struct BoneMergeCouplet {
	int				boneIIndex;
	BoneTerminus	boneITerminus;
	int				boneJIndex;
	BoneTerminus	boneJTerminus;
};

struct PolylinePlus {
	ofPolyline polyline; // assume there will always be a field called 'polyline'
	float r;
	float g;
	float b;
};



class SkeletonTracer {
	
	public:
	
	void	initialize (int w, int h);
	void	computeVectorSkeleton (unsigned char* skeletonPixelBuffer, int nRawContours);
	void	traceVectorSkeletonFromSkeletonImage();
	void	trackBones();
	void	mergeBones(); 
	void	smoothBones();
	void	optimallyReorderBones(int nPasses);
	void	drawStateImage();
	void	drawBones();
	ofPolyline getSmoothed (ofPolyline inputBone);
	
	ofPolyline			tempBone;
	ofPolyline			currentBone;
	vector<ofPolyline>	bonesRawTraced;		// 1. The raw traced bones.
	vector<ofPolyline>	bonesRawMerged;		// 2. Ultra-short bones appended to neighbors.
	vector<ofPolyline>	bonesRawSmooth;		// 3. Visually filtered bones (resampled, smoothed).
	vector<ofPolyline>	bonesReordered;		// 4. Bones optimized for minimum cumulative length.
	
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
	
	//--------------------------------
	// Generate high resolution images.
	ofxVectorGraphics	myVectorGraphics;
	void			exportVectorGraphics(); 
	
	

	bool			bDoMergeBones;
	float			boneResampling;
	float			boneSmoothSigma;
	int				boneSmoothKernW;
	bool			bDoOptimizeTSP;
	
	//-----------------
	vector<PolylinePlus>	theRawDrawing;
	vector<PolylinePlus>	tempDrawing;
	vector<PolylinePlus>	theOptimizedDrawing;
	
	float	drawingLength;
	bool	bClosedTSP;
	float	computeLengthOfDrawing(vector<PolylinePlus> aDrawing);
	// Conversion functions between Polyline Plus vectors and the Routes used in bryce_tsp.
	void	convert_polyline_plus_to_route(vector<PolylinePlus> * path_list, bryce_tsp::Route * route);
	void	convert_route_to_polyline_plus(vector<PolylinePlus>    * path_in,
										bryce_tsp::Route        * route_in,
										bryce_tsp::LaserProgram * permuter,
										vector<PolylinePlus>    * path_out);
	
	// Copies all of the polyline plus data, except for the ofPolyline data.
	void copy_extra_polyline_plus_data(PolylinePlus & src, PolylinePlus & dest);
	
	
};
