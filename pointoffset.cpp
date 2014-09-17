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
    if (offsetRadius == 0 && offsets.size() == 0) {
        offsetRadius = sqrt(pos.norm2);
    }

    PointOffset &po = offsets[pos];
	po.point = pos;
    po.offset = offset;
}

GCoord PointOffsetFilter::getOffsetAt(GCoord pos) {
    switch (offsets.size()) {
    case 0: 	// No transformation
        return ORIGIN;
    case 1: 	// A single offset defines uniform translation
        return offsets.begin()->second.offset;
    case 2: 	// Two offsets define scaling and translation 
    case 3:		// Three offsets define rotation, scaling and translation 
        LOGERROR("Not implemented");
        assert(FALSE);
        break;
    default: // Point cloud interpolation
        break;
    }

    // interpolate point cloud using simplex barycentric interpolation
    GCoord offset(0,0,0);
    double maxDist2 = offsetRadius * offsetRadius;
    vector<PointOffset> neighborhood;
    for (map<GCoord,PointOffset>::iterator ipo=offsets.begin(); ipo!=offsets.end(); ipo++) {
        double dist2 = pos.distance2(ipo->second.point);
        if (dist2 < maxDist2) {
            neighborhood.push_back(ipo->second);
        }
    }
    switch (neighborhood.size()) {
    case 0:		// assume no offset
        break;
    case 1:
        offset = neighborhood[0].offset;
        break;
    case 2: {
        double w1 = pos.distance2(neighborhood[0].point);
        double w2 = pos.distance2(neighborhood[1].point);
        double wt = w1+w2;
        offset = (w1/wt)*neighborhood[0].offset
                 + (w2/wt)*neighborhood[1].offset;
        break;
    }
    case 3: {
        double w1 = pos.distance2(neighborhood[0].point);
        double w2 = pos.distance2(neighborhood[1].point);
        double w3 = pos.distance2(neighborhood[2].point);
        double wt = w1+w2+w3;
        offset = (w1/wt)*neighborhood[0].offset
                 + (w2/wt)*neighborhood[1].offset
                 + (w3/wt)*neighborhood[2].offset;
        break;
    }
    default:	// just pick the first four as the tetrahedron vertices
    case 4: {
		GCoord bc = pos.barycentric(
			neighborhood[0].point, 
			neighborhood[1].point, 
			neighborhood[2].point, 
			neighborhood[3].point);
		double bc4 = 1 - (bc.x+bc.y+bc.z);
        offset = bc.x*neighborhood[0].offset
                 + bc.y*neighborhood[1].offset
                 + bc.z*neighborhood[2].offset 
                 + bc4*neighborhood[3].offset;
        break;
		}
    }

    return offset;
}

vector<PointOffset> PointOffsetFilter::offsetNeighborhood(GCoord pos, double radius) {
    double maxDist2 = radius*radius;
    vector<PointOffset> neighborhood;
    for (map<GCoord,PointOffset>::iterator ipo=offsets.begin(); ipo!=offsets.end(); ipo++) {
        double dist2 = pos.distance2(ipo->second.point);
//		cout << "neighborhood " << dist2 << *ipo << endl;
        if (dist2 < maxDist2) {
            neighborhood.push_back(ipo->second);
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

