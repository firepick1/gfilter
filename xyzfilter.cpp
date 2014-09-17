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

XYZFilter::XYZFilter(IGFilter &next) : GFilterBase(next) {
    _name = "XYZFilter";
	offsetRadius = 0;
}

void XYZFilter::setOffsetAt(GCoord pos, GCoord offset) {
    if (offsetRadius == 0 && offsets.size() == 0) {
		offsetRadius = sqrt(pos.norm2);
	}
	offsets[pos] = offset;
}

GCoord XYZFilter::interpolate(GCoord pos) {
	GCoord offset(0,0,0);
	double maxDist2 = offsetRadius * offsetRadius;
	vector<GCoord> neighborhood = offsetNeighborhood(pos, offsetRadius);
	int n = 0;
	for (map<GCoord,GCoord>::iterator iOffset=offsets.begin(); iOffset!=offsets.end(); iOffset++) {
		double dist2 = pos.distance2(iOffset->first);
		if (dist2 < maxDist2) {
			double scale = 1-dist2/maxDist2;
			n++;
			cout << n << ": " << scale << "*" << iOffset->second << endl;
			offset = offset + scale*iOffset->second;
		}
	}
	offset = (1.0/n) * offset;
	cout << "SCALEDOFFSET:" << offset << endl;
	return pos + offset;
}

vector<GCoord> XYZFilter::offsetNeighborhood(GCoord pos, double radius) {
	double maxDist2 = radius*radius;
	vector<GCoord> neighborhood;
	for (map<GCoord,GCoord>::iterator iOffset=offsets.begin(); iOffset!=offsets.end(); iOffset++) {
		double dist2 = pos.distance2(iOffset->first);
		if (dist2 < maxDist2) {
		 	neighborhood.push_back(iOffset->first);
		}
	}

	return neighborhood;
}

int XYZFilter::writeln(const char *value) {
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

