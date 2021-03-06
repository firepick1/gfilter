#ifndef GFILTER_HPP
#define GFILTER_HPP

#include <ostream>
#include <vector>
#include <cstring>
#include <map>
#include <math.h>
#include "FireUtils.hpp"
#include "jansson.h"

using namespace std;

#ifndef bool
#define bool int
#define TRUE 1
#define FALSE 0
#endif

namespace gfilter {

typedef class Mat3x3 {
    private:
        double mat[3][3];

    public:
        inline Mat3x3(
            double a,double b,double c,
            double d,double e,double f,
            double g,double h,double i)
        {
            mat[0][0] = a;
            mat[0][1] = b;
            mat[0][2] = c;
            mat[1][0] = d;
            mat[1][1] = e;
            mat[1][2] = f;
            mat[2][0] = g;
            mat[2][1] = h;
            mat[2][2] = i;
        }
        inline Mat3x3() {
            clear();
        }
        inline double at(int y, int x) {
            return mat[y][x];
        };
        inline void clear() {
            memset( mat, 0, sizeof mat );
        }
        inline double det2x2( int atY, int atX) {
            int x1 = atX == 0 ? 1 : 0;
            int x2 = atX == 2 ? 1 : 2;
            int y1 = atY == 0 ? 1 : 0;
            int y2 = atY == 2 ? 1 : 2;

            return ( mat[y1][x1] * mat[y2][x2] )
                   -  ( mat[y1][x2] * mat[y2][x1] );
        }
        inline double det3x3() {
            return mat[0][0] * det2x2(0,0) - mat[0][1] * det2x2(0,1) + mat[0][2] * det2x2(0,2);
        }
        bool inverse(Mat3x3 &matInv);
        friend bool operator==(const Mat3x3& lhs, const Mat3x3& rhs);
        friend ostream& operator<<(ostream& os, const Mat3x3& that);
} Mat3x3;

typedef struct GCoord {
    double norm2; // square of distance to ORIGIN
    double x;
    double y;
    double z;
    inline GCoord() : x(HUGE_VAL), y(HUGE_VAL), z(HUGE_VAL) {}; // !isValid()
    inline GCoord(double xPos, double yPos, double zPos) : x(xPos), y(yPos), z(zPos) {
        norm2 = xPos*xPos + yPos*yPos + zPos*zPos;
    };
    inline double const distance2(const GCoord &that) {
        double dx = x - that.x;
        double dy = y - that.y;
        double dz = z - that.z;
        return dx*dx + dy*dy + dz*dz;
    }
    inline friend ostream& operator<<(ostream& os, const GCoord& value) {
        os << "(" << value.x << "," << value.y << "," << value.z << ")";
        return os;
    }
	inline string toString() const {
		char buf[255];
		snprintf(buf, sizeof(buf), "(%g,%g,%g)",x,y,z);
		return string(buf);
	}
    inline GCoord friend operator*(double scalar, const GCoord &rhs) {
        return GCoord(scalar*rhs.x,scalar*rhs.y,scalar*rhs.z);
    }
    inline GCoord friend operator+(const GCoord& lhs, const GCoord &rhs) {
        return GCoord(lhs.x+rhs.x,lhs.y+rhs.y,lhs.z+rhs.z);
    }
    inline GCoord friend operator-(const GCoord& lhs, const GCoord &rhs) {
        return GCoord(lhs.x-rhs.x,lhs.y-rhs.y,lhs.z-rhs.z);
    }
    inline bool friend operator==(const gfilter::GCoord& lhs, const gfilter::GCoord &rhs) {
        return lhs.x==rhs.x && lhs.y==rhs.y && lhs.z == rhs.z;
    }
    inline bool friend operator!=(const GCoord& lhs, const GCoord &rhs) {
        return !(lhs==rhs);
    }
	inline GCoord trunc(int places) {
		double limit = pow(10.0,-places);
		if (-limit < x && x < limit) {
			x = 0;
		}
		if (-limit < y && y < limit) {
			y = 0;
		}
		if (-limit < z && z < limit) {
			z = 0;
		}
	}
    inline bool friend operator<(const GCoord& lhs, const GCoord &rhs) {
        int cmp = lhs.norm2 - rhs.norm2;
        if (cmp == 0) {
            cmp = lhs.x - rhs.x;
        }
        if (cmp == 0) {
            cmp = lhs.y - rhs.y;
        }
        if (cmp == 0) {
            cmp = lhs.z - rhs.z;
        }
        return cmp < 0;
    };
	inline bool isValid() { return x != HUGE_VAL && y != HUGE_VAL && z != HUGE_VAL; }
    friend GCoord operator*(Mat3x3 &mat, GCoord &c);
    GCoord barycentric(const GCoord &c1, const GCoord &c2, const GCoord &c3, const GCoord &c4);
} GCoord;

const GCoord ORIGIN(0,0,0);

typedef struct MappedPoint {
    GCoord domain;
    GCoord range;
    inline friend ostream& operator<<(ostream& os, const MappedPoint& value) {
        os << "(" << value.domain.x << "," << value.domain.y << "," << value.domain.z << ")"
           << ":"
           << "(" << value.range.x << "," << value.range.y << "," << value.range.z << ")"
           ;
        return os;
    }
    inline bool friend operator==(const MappedPoint& lhs, const MappedPoint &rhs) {
        return lhs.domain==rhs.domain && lhs.range==rhs.range;
    }
    inline bool friend operator!=(const MappedPoint& lhs, const MappedPoint &rhs) {
        return !(lhs==rhs);
    }
    inline bool friend operator<(const MappedPoint& lhs, const MappedPoint &rhs) {
        return lhs.domain < rhs.domain;
    };
	inline string toString() const {
		char buf[255];
		snprintf(buf, sizeof(buf), "domain:%s range:%s", domain.toString().c_str(), range.toString().c_str());
		return string(buf);
	}
} MappedPoint, *MappedPointPtr;

typedef class IGCodeMatcher {
    public:
        /**
         * Return the length of the longest text prefix that can
         * be parsed as a number
         */
        static int matchNumber (const char *text);

        /**
         * Return the length of the longest text prefix that matches
         * whatever the implementation class is looking for
         */
        virtual int match(const char *text) = 0;
} IGCodeMatcher;

typedef class GMoveMatcher:public IGCodeMatcher {
    public:
        string code;
        GCoord coord;
        virtual int match(const char *text);
} GMoveMatcher;

typedef class IGFilter {
    protected:
        const char *_name;

    public:
        const char *name () {
            return _name;
        };
        IGFilter ():_name ("IGFilter") {
        };

        virtual int writeln (const char *value) = 0;
} IGFilter, *IGFilterPtr;

typedef class GFilterBase:public IGFilter {
    protected:
        IGFilter & _next;
    protected:
        GFilterBase (IGFilter & next):_next (next) {
            _name = "GFilterBase";
        };
    public:
        virtual int writeln (const char *value) {
            _next.writeln (value);
            return 0;
        };
} GFilterBase;

/**
 * The ultimate recipient of all the GCode
 */
typedef class GCodeSink:public IGFilter {
} GCodeSink;

typedef class StringSink:public GCodeSink {
    public:
        vector<string> strings;
        virtual int writeln (const char *value);
		string operator[](int index){ return strings[index]; }
} StringSink;

typedef class OStreamSink:public GCodeSink {
    private:
        ostream * pos;

    public:
        OStreamSink (ostream & os) {
            pos = &os;
        };
        ~OStreamSink () {
            pos->flush ();
        };

        virtual int writeln (const char *value);
} OStreamSink;

typedef class DeltaFilter:public GFilterBase {
    private:
        GMoveMatcher matcher;

    public:
        DeltaFilter (IGFilter & next);
        virtual int writeln (const char *value);
} DeltaFilter, *DeltaFilterPtr;

typedef class MappedPointFilter:public GFilterBase {
    private:
		GCoord domain;	// current input domain position 
        double domainRadius;
        GMoveMatcher matcher;
        map<GCoord, MappedPoint> mapping; // TODO: use PCL octtree or something

    public:
        MappedPointFilter (IGFilter & next, json_t* config=NULL);
		int configure(json_t *config);
        virtual int writeln (const char *value);
        GCoord interpolate(GCoord domainXYZ);
        vector<MappedPoint> domainNeighborhood(GCoord domainXYZ, double radius);
        void mapPoint(GCoord domain, GCoord range);
        double getDomainRadius() {
            return domainRadius;
        }
        void setDomainRadius(double value) {
            domainRadius = value;
        }
} MappedPointFilter, *MappedPointFilterPtr;

}				// namespace gfilter

#endif
