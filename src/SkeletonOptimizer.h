#pragma once

#include <stdio.h>

#include "ofMain.h"
#include "LaserMain.h"
#include "LaserDefs.h"


struct PolylinePlus {
	ofPolyline polyline; // assume there will always be a field called 'polyline'
	float r;
	float g;
	float b;
};

//------------------------------------------------------
class SkeletonOptimizer {
	
	public:
	
		void optimallyReorderBones(vector<PolylinePlus>	&theRawDrawing, int nPasses, bool bClosedTSP);
		vector<PolylinePlus>	theOptimizedDrawing;

		float	drawingLength;
		float	computeLengthOfDrawing(vector<PolylinePlus> aDrawing, bool bClosedTSP);

		// Conversion functions between Polyline Plus vectors and the Routes used in bryce_tsp.
		void	convert_polyline_plus_to_route(vector<PolylinePlus> * path_list, bryce_tsp::Route * route);
		void	convert_route_to_polyline_plus(vector<PolylinePlus>    * path_in,
		bryce_tsp::Route        * route_in,
		bryce_tsp::LaserProgram * permuter,
		vector<PolylinePlus>    * path_out);

		// Copies all of the polyline plus data, except for the ofPolyline data.
		void copy_extra_polyline_plus_data(PolylinePlus & src, PolylinePlus & dest);
	

};