#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "FireLog.h"
#include "gfilter.hpp"
#include "jo_util.hpp"
#include "version.h"
#include "jansson.h"

using namespace std;
using namespace gfilter;

PointOffsetFilter::PointOffsetFilter(IGFilter &next, json_t *pConfig) : GFilterBase(next) {
    _name = "PointOffsetFilter";
    offsetRadius = 0;
	source = GCoord(0,0,0);
	if (pConfig) {
		LOGINFO("PointOffsetFilter(JSON)");
		ASSERTZERO(configure(pConfig));
	} else {
		LOGINFO("PointOffsetFilter()");
	}
}

int PointOffsetFilter::configure(json_t *pConfig) {
	LOGINFO("PointOffsetFilter::configure()");
	json_t * pOffsets = json_object_get(pConfig, "offsets");
	if (json_is_array(pOffsets)) {
		size_t index;
		json_t *pPointOffset;
		json_array_foreach(pOffsets, index, pPointOffset) {
			vector<float> vPoint = jo_vectorf(pPointOffset, "point", vector<float>(), emptyMap);
			vector<float> vOffset = jo_vectorf(pPointOffset, "offset", vector<float>(), emptyMap);
			if (vPoint.size() != 3) {
				LOGERROR1("PointOffsetFilter::configure() point vector size expectedi:3 actual:%d",
					(int) vPoint.size());
			} else if (vOffset.size() != 3) {
				LOGERROR1("PointOffsetFilter::configure() offset vector size expectedi:3 actual:%d",
					(int) vOffset.size());
			} else {
				GCoord point(vPoint[0],vPoint[1],vPoint[2]); 
				GCoord offset(vOffset[0],vOffset[1],vOffset[2]);
				LOGDEBUG2("PointOffsetFilter::configure() point:%s offset:%s",
					point.toString().c_str(), offset.toString().c_str());
				setOffsetAt(point, offset);
			}

		}
	} else if (pOffsets) {
		LOGERROR("PointOffsetFilter::configure() expected JSON array for offsets");
		return -EINVAL;
	}

	return 0;
}

void PointOffsetFilter::setOffsetAt(GCoord pos, GCoord offset) {
    if (offsetRadius == 0 || offsets.size() == 0) {
        offsetRadius = sqrt(pos.norm2);
		LOGINFO1("PointOffsetFilter() offsetRadius:%g", offsetRadius);
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
    double maxDist2 = offsetRadius * offsetRadius;
    vector<PointOffset> neighborhood;
    for (map<GCoord,PointOffset>::iterator ipo=offsets.begin(); ipo!=offsets.end(); ipo++) {
        double dist2 = pos.distance2(ipo->second.point);
        if (dist2 < maxDist2) {
            neighborhood.push_back(ipo->second);
        }
    }
	LOGDEBUG2("getOffsetAt(%s) neighborhood:%d", 
		pos.toString().c_str(), (int) neighborhood.size());

    GCoord offset;
    switch (neighborhood.size()) {
    case 0:		// assume no offset
        offset = GCoord(0,0,0);
        break;
    case 1:
    case 2:
    case 3:
        // weighted average
        break;
    default:	// just pick the first four as the tetrahedron vertices
    case 4: {
        GCoord bc = pos.barycentric(
                        neighborhood[0].point,
                        neighborhood[1].point,
                        neighborhood[2].point,
                        neighborhood[3].point);
        if (bc.isValid()) {
            double bc4 = 1 - (bc.x+bc.y+bc.z);
            offset = bc.x*neighborhood[0].offset
                     + bc.y*neighborhood[1].offset
                     + bc.z*neighborhood[2].offset
                     + bc4*neighborhood[3].offset;
		} else {
			LOGDEBUG("degenerate tetrahedron");
        }
        break;
    }
    }

	if (!offset.isValid()) {
		//cout << "weighted average" << endl;
		int n = min(4l, (long) neighborhood.size());
		double w[4];
		double wt = 0;
		offset = GCoord(0,0,0);
		for (long i=0; i<n; i++) {
			w[i] = pos.distance2(neighborhood[i].point);
			wt += w[i];
			if (w[i] == 0) {
				offset = neighborhood[i].offset;
			}
		}
		cout << "DEBUG: " << offset << endl;
		if (wt != 0) {
			for (long i=0; i<n; i++) {
			cout << "DEBUG: " << i << " "<< offset << endl;
				offset = offset + w[i]/wt * neighborhood[i].offset;
			}
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
    int chars = matcher.match(value);
    char buf[255];

    if (chars) {
		GCoord sourceNew = source;
        if (matcher.coord.x != HUGE_VAL) {
			sourceNew.x = matcher.coord.x;
        }
        if (matcher.coord.y != HUGE_VAL) {
			sourceNew.y = matcher.coord.y;
        }
        if (matcher.coord.z != HUGE_VAL) {
			sourceNew.z = matcher.coord.z;
        }
		GCoord offset = getOffsetAt(sourceNew);
		cout << "DEBUG:" << offset << endl;
		sourceNew = sourceNew + offset;
		char *s = buf;
		*s++ = 'G';
		*s++ = matcher.code.c_str()[1];
		if (matcher.code.c_str() == "G28") {
			*s++ = matcher.code.c_str()[2]; // G28
			sourceNew = GCoord(0,0,0);
			if (matcher.coord.x != HUGE_VAL) {
				*s++ = 'X'; *s++ = '0';
			}
			if (matcher.coord.y != HUGE_VAL ) {
				*s++ = 'Y'; *s++ = '0';
			}
			if (matcher.coord.z != HUGE_VAL ) {
				*s++ = 'Z'; *s++ = '0';
			}
		} else {
			if (sourceNew.x != source.x) {
				s += sprintf(s, "X%g", sourceNew.x);
			}
			if (sourceNew.y != source.y ) {
				s += sprintf(s, "Y%g", sourceNew.y);
			}
			if (sourceNew.z != source.z ) {
				s += sprintf(s, "Z%g", sourceNew.z);
			}
		}
		*s = 0;
        _next.writeln(buf);
		source = sourceNew;
    } else {
        _next.writeln(value);
    }

    return 0;
}

