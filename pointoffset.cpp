#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cfloat>
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
	domain = GCoord(0,0,0);
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

GCoord PointOffsetFilter::interpolate(GCoord pos) {
    switch (offsets.size()) {
    case 0: 	// No transformation
        return pos;
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
    vector<PointOffset> neighborhood = offsetNeighborhood(pos, offsetRadius);
	LOGDEBUG2("interpolate(%s) neighborhood:%d", 
		pos.toString().c_str(), (int) neighborhood.size());

    GCoord offset;
	int n = neighborhood.size();
	if (logLevel >= FIRELOG_TRACE) {
		for (int i=0; i < n; i++) {
			LOGTRACE3("neighborhood[%d]: %s %g", i, neighborhood[i].toString().c_str(), 
				pos.distance2(neighborhood[i].point));
		}
	}
    switch (neighborhood.size()) {
    case 0:		// assume no offset
        offset = GCoord(0,0,0);
        break;
    case 1:
    case 2:
    case 3:
        // weighted average
        break;
    default:	// the first four are the closest and are used as the tetrahedron vertices
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
			offset.trunc(5);
			LOGTRACE4("barycentric(%g,%g,%g,%g)", bc.x, bc.y, bc.z, bc4);
			LOGTRACE3("barycentric => (%g,%g,%g)", offset.x,offset.y,offset.z);
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
#define WEIGHTING_DISTANCE 0.001 /* All points within this distance are treated the same */
		for (long i=0; i<n; i++) {
			double d = pos.distance2(neighborhood[i].point);
			d = max(d, WEIGHTING_DISTANCE*WEIGHTING_DISTANCE);
			w[i] = 1/sqrt(d);
			wt += w[i];
		}
		if (wt != 0) {
			for (long i=0; i<n; i++) {
				offset = offset + w[i]/wt * neighborhood[i].offset;
			}
		}
		LOGTRACE3("weightd => (%g,%g,%g)", offset.x,offset.y,offset.z);
	}

    return offset;
}

vector<PointOffset> PointOffsetFilter::offsetNeighborhood(GCoord pos, double radius) {
    double maxDist2 = radius*radius;
    vector<PointOffset> neighborhood;
	double dist[4];	// sort top 4 for barycentric tetrahedron
	for (int i=0; i < 4; i++) {
		dist[i] = DBL_MAX;
	}
    for (map<GCoord,PointOffset>::iterator ipo=offsets.begin(); ipo!=offsets.end(); ipo++) {
        double dist2 = pos.distance2(ipo->first);
//		cout << "neighborhood " << dist2 << *ipo << endl;
        if (dist2 < maxDist2) {
			bool inserted = FALSE;
			int n = min(4, (int)neighborhood.size()+1);
			for (int i=0; i < n; i++) {
				if (dist2 < dist[i]) { // insert here
					if (dist[i] < DBL_MAX) {
						//cout << "insert@" << i<< ":" << ipo->second << " " << dist2 << " < " << dist[i] << " " << neighborhood.at(i) << endl;
					} else {
						//cout << "insert@" << i<< ":" << ipo->second << " " << dist2 << " < " << dist[i] << " " << endl;
					}
					neighborhood.insert(neighborhood.begin()+i, ipo->second);
					for (int j=n; --j > i; ) {
						cout << "dist[" << j << "] " <<  dist[j] << "=" << dist[j-1] <<endl;
						dist[j] = dist[j-1];
					}
					dist[i] = dist2;
					inserted = TRUE;
					break;
				} else {
					//cout << "skip@" << i<< ":" << ipo->second << dist2 << " < " << dist[i] << " " << neighborhood.at(i) << endl;
				}
			}
            if (!inserted) {
				neighborhood.push_back(ipo->second);
			}
        }
    }

    return neighborhood;
}

int PointOffsetFilter::writeln(const char *value) {
    int chars = matcher.match(value);
    char buf[255];

    if (chars) {
		GCoord newDomain  = domain;
        if (matcher.coord.x != HUGE_VAL) {
			newDomain.x = matcher.coord.x;
        }
        if (matcher.coord.y != HUGE_VAL) {
			newDomain.y = matcher.coord.y;
        }
        if (matcher.coord.z != HUGE_VAL) {
			newDomain.z = matcher.coord.z;
        }
		GCoord range = interpolate(newDomain);
		char *s = buf;
		*s++ = 'G';
		*s++ = matcher.code.c_str()[1];
		if (matcher.code.c_str()[1] == '2' && matcher.code.c_str()[2] == '8') {
			*s++ = '8';
			range = GCoord(0,0,0);
		}
		s += sprintf(s, "X%g", range.x);
		s += sprintf(s, "Y%g", range.y);
		s += sprintf(s, "Z%g", range.z);
		s += snprintf(s, sizeof(buf)-(s-buf), "%s", value+chars);
		*s = 0;
        _next.writeln(buf);
		domain = newDomain;
    } else {
        _next.writeln(value);
    }

    return 0;
}

