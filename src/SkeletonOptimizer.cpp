//
//  SkeletonOptimizer.cpp
//  GPP2016_Display
//
//  Created by GL on 12/6/16.
//
//

#include "SkeletonOptimizer.h"


//--------------------------------------------------------------
void SkeletonOptimizer::optimallyReorderBones(vector<PolylinePlus> &theRawDrawing,
											  int nPasses, bool bClosedTSP){
	
	theOptimizedDrawing.clear();
	if (theRawDrawing.size() > 0){
		float initialLength = computeLengthOfDrawing(theRawDrawing, bClosedTSP);
		bryce_tsp::Route *route_in = new bryce_tsp::Route();
		convert_polyline_plus_to_route (&theRawDrawing, route_in);
		
		if (route_in->size() > 0){
			bryce_tsp::LaserProgram program(route_in, bClosedTSP);
			bryce_tsp::free_route(route_in);
			
			long then = ofGetElapsedTimeMicros();
			for (int i = 0; i <= nPasses; i++){
				program.optimize(i);
				bryce_tsp::Route * route_out = program.getRoute();
				theOptimizedDrawing.clear();
				convert_route_to_polyline_plus(&theRawDrawing, route_out, &program, &theOptimizedDrawing);
				bryce_tsp::free_route(route_out);
				drawingLength = computeLengthOfDrawing(theOptimizedDrawing, bClosedTSP);
			}
			
			float frac = (drawingLength/initialLength);
			if (frac > 1.00){ // weirdly, it sometimes happens.
				// Clobber theOptimizedDrawing, and copy theRawDrawing into it instead
				frac = 1.0;
				theOptimizedDrawing.clear();
				for (int i=0; i<theRawDrawing.size(); i++){
					PolylinePlus aPolylinePlus = theRawDrawing[i];
					theOptimizedDrawing.push_back(aPolylinePlus);
				}
			}
			
			float A = 0.96; float B = 1.0-A;
			optimizationAmount = A*optimizationAmount + B*(1.0-frac);
			
		}
	}
}


//--------------------------------------------------------------
float SkeletonOptimizer::computeLengthOfDrawing(vector<PolylinePlus> aDrawing, bool bClosedTSP) {
	
	float len = 0;
	int nPolylinePlusses = aDrawing.size();
	
	for (int i = 0; i < nPolylinePlusses; i++) {
		PolylinePlus aPolylinePlus = aDrawing[i];
		ofPolyline aPolyline = aPolylinePlus.polyline;
		len += aPolyline.getPerimeter();
		
		if (bClosedTSP || (i < nPolylinePlusses - 1)){
			ofPoint lastOfThis  = aPolyline[aPolyline.size() - 1];
			ofPolyline nextPolyline = aDrawing[(i + 1) % nPolylinePlusses].polyline;
			ofPoint firstOfNext = nextPolyline[0];
			float dist = ofDist(lastOfThis.x, lastOfThis.y, firstOfNext.x, firstOfNext.y);
			len += dist;
		}
	}
	return len;
}


//------------------------------------------------------------
void SkeletonOptimizer::convert_polyline_plus_to_route(vector<PolylinePlus> * path_list,
													   bryce_tsp::Route * route){
	for (auto iter = path_list -> begin(); iter != path_list -> end(); ++iter){
		ofPolyline * polyline_in = &(iter -> polyline);
		if (polyline_in->size() > 1){
			bryce_tsp::Polyline * polyline_out = bryce_tsp::of_polyline_to_polyline(polyline_in);
			route -> push_back(polyline_out);
		}
	}
}

//------------------------------------------------------------
void SkeletonOptimizer::convert_route_to_polyline_plus(vector<PolylinePlus>		* path_in,
													   bryce_tsp::Route			* route_in,
													   bryce_tsp::LaserProgram	* permuter,
													   vector<PolylinePlus>		* path_out){
	// Transcribe each of the input Polyline Plus paths into output Polyline Plus paths.
	// using information from the optimizer.
	int len = route_in->size();
	for (int new_index = 0; new_index < len; new_index++) {
		bryce_tsp::Polyline *polyline = route_in->at(new_index);
		path_out->push_back(PolylinePlus());
		PolylinePlus &out = path_out->back();
		out.polyline.clear();
		
		// Copy over the polyline.
		for (auto pt = polyline -> cbegin(); pt != polyline -> cend(); ++pt){
			out.polyline.addVertex(*pt);
		}
		
		int original_index = permuter -> lookup_original_index(new_index);
		PolylinePlus & in  = path_in  -> at(original_index);
		copy_extra_polyline_plus_data(in, out);
	}
}

//------------------------------------------------------------
void SkeletonOptimizer::copy_extra_polyline_plus_data(PolylinePlus & src, PolylinePlus & dest){
	// Simply copy over the data fields from the source to the destination.
	dest.r = src.r;
	dest.g = src.g;
	dest.b = src.b;
}
