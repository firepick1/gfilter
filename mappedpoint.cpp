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

MappedPointFilter::MappedPointFilter(IGFilter &next, json_t *pConfig) : GFilterBase(next) {
    _name = "MappedPointFilter";
    domainRadius = 0;
	domain = GCoord(0,0,0);
	if (pConfig) {
		LOGINFO("MappedPointFilter(JSON)");
		ASSERTZERO(configure(pConfig));
	} else {
		LOGINFO("MappedPointFilter()");
	}
}

int MappedPointFilter::configure(json_t *pConfig) {
	LOGINFO("MappedPointFilter::configure()");
	json_t * pMapping = json_object_get(pConfig, "map");
	if (json_is_array(pMapping)) {
		size_t index;
		json_t *pMappedPoint;
		json_array_foreach(pMapping, index, pMappedPoint) {
			vector<float> vDomain = jo_vectorf(pMappedPoint, "domain", vector<float>(), emptyMap);
			vector<float> vRange = jo_vectorf(pMappedPoint, "range", vector<float>(), emptyMap);
			if (vDomain.size() != 3) {
				LOGERROR1("MappedPointFilter::configure() point vector size expectedi:3 actual:%d",
					(int) vDomain.size());
			} else if (vRange.size() != 3) {
				LOGERROR1("MappedPointFilter::configure() range vector size expectedi:3 actual:%d",
					(int) vRange.size());
			} else {
				GCoord domain(vDomain[0],vDomain[1],vDomain[2]); 
				GCoord range(vRange[0],vRange[1],vRange[2]);
				LOGDEBUG2("MappedPointFilter::configure() domain:%s range:%s",
					range.toString().c_str(), domain.toString().c_str());
				mapPoint(domain, range);
			}

		}
	} else if (pMapping) {
		LOGERROR("MappedPointFilter::configure() expected JSON array for point mapping");
		return -EINVAL;
	}

	return 0;
}

void MappedPointFilter::mapPoint(GCoord domain, GCoord range) {
    if (domainRadius == 0 || mapping.size() == 0) {
        domainRadius = sqrt(domain.norm2);
		LOGINFO1("MappedPointFilter() domainRadius:%g", domainRadius);
    }

    MappedPoint &po = mapping[domain];
    po.domain = domain;
    po.range = range;
}

GCoord MappedPointFilter::interpolate(GCoord domain) {
    switch (mapping.size()) {
    case 0: 	// No transformation
		LOGTRACE("no interpolation mapping");
        return domain;
    case 1: 	// a single mapped point defines a universal translation
		return domain + mapping.begin()->second.range - mapping.begin()->second.domain;
    case 2: 
        LOGERROR("2-point mapping is undefined"); // translate and scale?
        assert(FALSE);
        break;
    case 3:	
        LOGERROR("3-point mapping is undefined");	//  translate, scale and rotate?
        assert(FALSE);
        break;
    default: // Point cloud interpolation
        break;
    }

    // interpolate point cloud using simplex barycentric interpolation
    double maxDist2 = domainRadius * domainRadius;
    vector<MappedPoint> neighborhood = domainNeighborhood(domain, domainRadius);
	LOGDEBUG2("interpolate(%s) neighborhood:%d", 
		domain.toString().c_str(), (int) neighborhood.size());

    GCoord range;
	int n = neighborhood.size();
	if (logLevel >= FIRELOG_TRACE) {
		for (int i=0; i < n; i++) {
			LOGTRACE3("neighborhood[%d]: %s %g", i, neighborhood[i].toString().c_str(), 
				domain.distance2(neighborhood[i].domain));
		}
	}
    switch (neighborhood.size()) {
    case 0:		// no mapping => no change
		range = domain;
        break;
    case 1:
    case 2:
    case 3:
        // weighted average
        break;
    default:	// the first four are the closest and are used as the tetrahedron vertices
    case 4: {
        GCoord bc = domain.barycentric(
                        neighborhood[0].domain,
                        neighborhood[1].domain,
                        neighborhood[2].domain,
                        neighborhood[3].domain);
        if (bc.isValid()) {
            double bc4 = 1 - (bc.x+bc.y+bc.z);
            range = bc.x*neighborhood[0].range
                     + bc.y*neighborhood[1].range
                     + bc.z*neighborhood[2].range
                     + bc4*neighborhood[3].range;
			range.trunc(5);
			LOGTRACE4("barycentric(%g,%g,%g,%g)", bc.x, bc.y, bc.z, bc4);
			LOGTRACE3("barycentric => (%g,%g,%g)", range.x, range.y, range.z);
		} else {
			LOGDEBUG("degenerate tetrahedron");
        }
        break;
    }
    }

	if (!range.isValid()) {
		//cout << "weighted average" << endl;
		int n = min(4l, (long) neighborhood.size());
		double w[4];
		double wt = 0;
		range = GCoord(0,0,0);
#define WEIGHTING_DISTANCE 0.001 /* All points within this distance are treated the same */
		for (long i=0; i<n; i++) {
			double d = domain.distance2(neighborhood[i].domain);
			d = max(d, WEIGHTING_DISTANCE*WEIGHTING_DISTANCE);
			w[i] = 1/sqrt(d);
			wt += w[i];
		}
		if (wt != 0) {
			for (long i=0; i<n; i++) {
				range = range + w[i]/wt * neighborhood[i].range;
			}
		}
		LOGTRACE3("weightd => (%g,%g,%g)", range.x, range.y, range.z);
	}

    return range;
}

vector<MappedPoint> MappedPointFilter::domainNeighborhood(GCoord domain, double radius) {
    double maxDist2 = radius*radius;
    vector<MappedPoint> neighborhood;
	double dist[4];	// sort top 4 for barycentric tetrahedron
	for (int i=0; i < 4; i++) {
		dist[i] = DBL_MAX;
	}
    for (map<GCoord,MappedPoint>::iterator ipo=mapping.begin(); ipo!=mapping.end(); ipo++) {
        double dist2 = domain.distance2(ipo->first);
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
						//cout << "dist[" << j << "] " <<  dist[j] << "=" << dist[j-1] <<endl;
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

int MappedPointFilter::writeln(const char *value) {
    int chars = matcher.match(value);
    char buf[255];

    if (chars) {
		GCoord domainNew  = domain;
        if (matcher.coord.x != HUGE_VAL) {
			domainNew.x = matcher.coord.x;
        }
        if (matcher.coord.y != HUGE_VAL) {
			domainNew.y = matcher.coord.y;
        }
        if (matcher.coord.z != HUGE_VAL) {
			domainNew.z = matcher.coord.z;
        }
		GCoord range = interpolate(domainNew);
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
		domain = domainNew;
    } else {
		LOGTRACE1("MappedPointFilter::writeln(%s) (no change)", value);
        _next.writeln(value);
    }

    return 0;
}

