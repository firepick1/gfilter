#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "FireLog.h"
#include "gfilter.hpp"
#include "version.h"
#include "jansson.h"

using namespace std;
using namespace gfilter;

PointOffsetFilter::PointOffsetFilter(IGFilter &next) : GFilterBase(next) {
    _name = "PointOffsetFilter";
	offsetRadius = 0;
}

void PointOffsetFilter::setOffsetAt(GCoord pos, GCoord offset) {
	int sizeBefore = offsetMap.size();
    if (offsetRadius == 0 && sizeBefore == 0) {
		offsetRadius = sqrt(pos.norm2);
	}
	
	PointOffset &po = offsetMap[pos];
	po.point = pos;
	po.offset = offset;
	offsets.push_back(po);
}

GCoord PointOffsetFilter::getOffsetAt(GCoord pos) {
	switch (offsets.size()) {
	    case 0: 	// No transformation
			return ORIGIN;		
		case 1: 	// A single offset defines uniform translation
			return offsets.begin()->offset;	
		case 2: {	// Two offsets define scaling and translation {									
			GCoord translate = offsets[0].offset;
			GCoord vOld = offsets[1].point - offsets[0].point;
			GCoord vNew = vOld + offsets[1].offset;
			return GCoord(
				translate.x + (vNew.x/vOld.x) * pos.x,
				translate.y + (vNew.y/vOld.y) * pos.y,
				translate.z + (vNew.z/vOld.z) * pos.z);
			}
		case 3:		// Three offsets define rotation, scaling and translation {
			LOGERROR("Not implemented");
			assert(FALSE);
			break;
		default: // Point cloud interpolation
			break;
	}

	// interpolate point cloud using simplex barycentric interpolation
	GCoord offset(0,0,0);
	double maxDist2 = offsetRadius * offsetRadius;
	int n = 0;
	for (vector<PointOffset>::iterator iOffset=offsets.begin(); iOffset!=offsets.end(); iOffset++) {
		double dist2 = pos.distance2(iOffset->point);
		if (dist2 < maxDist2) {
			double scale = 1-dist2/maxDist2;
			n++;
			cout << n << ": " << scale << "*" << iOffset->offset << endl;
			offset = offset + scale*iOffset->offset;
		}
	}

	return (1.0/n) * offset;
}

vector<PointOffset> PointOffsetFilter::offsetNeighborhood(GCoord pos, double radius) {
	double maxDist2 = radius*radius;
	vector<PointOffset> neighborhood;
	for (vector<PointOffset>::iterator iOffset=offsets.begin(); iOffset!=offsets.end(); iOffset++) {
		double dist2 = pos.distance2(iOffset->point);
		if (dist2 < maxDist2) {
		 	neighborhood.push_back(*iOffset);
		}
	}

	return neighborhood;
}

int PointOffsetFilter::writeln(const char *value) {
    int chars = g0g1.match(value);
    char buf[255];

    if (chars) {
		char *s = buf;
		*s++ = 'G';
		*s++ = g0g1.code.c_str()[1];
		if (g0g1.coord.x != HUGE_VAL) {
			s += sprintf(s, "X%f", g0g1.coord.x);
		}
		if (g0g1.coord.y != HUGE_VAL) {
			s += sprintf(s, "Y%f", g0g1.coord.y);
		}
		if (g0g1.coord.z != HUGE_VAL) {
			s += sprintf(s, "Z%f", g0g1.coord.z);
		}
		*s++ = 0;
		_next.writeln(buf);
    } else {
        _next.writeln(value);
    }

    return 0;
}

