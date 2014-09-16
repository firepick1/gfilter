#ifndef GFILTER_HPP
#define GFILTER_HPP

#include <ostream>
#include <vector>
#include <map>
#include <math.h>
#include "jansson.h"

using namespace std;

#ifndef bool
#define bool int
#define TRUE 1
#define FALSE 0
#endif

namespace gfilter {

typedef struct GCoord {
	double x;
	double y;
	double z;
	inline GCoord() : x(HUGE_VAL), y(HUGE_VAL), z(HUGE_VAL) {};
	inline GCoord(double xPos, double yPos, double zPos) : x(xPos), y(yPos), z(zPos) {};
	inline friend ostream& operator<<(ostream& os, const GCoord& value) {
		os << "(" << value.x << "," << value.y << "," << value.z << ")";
		return os;
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
	inline bool friend operator==(const GCoord& lhs, const GCoord &rhs) {
	  return lhs.x==rhs.x && lhs.y==rhs.y && lhs.z == rhs.z;
	}
	inline bool friend operator!=(const GCoord& lhs, const GCoord &rhs) {
	  return !(lhs==rhs);
	}
	inline bool friend operator<(const GCoord& lhs, const GCoord &rhs) {
	  int cmp = lhs.x - rhs.x;
	  if (cmp == 0) {
	      cmp = lhs.y - rhs.y;
	  }
	  if (cmp == 0) {
	      cmp = lhs.z - rhs.z;
	  }
	  return cmp < 0;
	}
} GCoord;

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

typedef class G0G1Matcher:public IGCodeMatcher {
    public:
        string code;
		GCoord coord;
        virtual int match(const char *text);
} G0G1Matcher;

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

typedef class StringSink:public IGFilter {
    public:
        vector<string> strings;
        virtual int writeln (const char *value);
} StringSink;

typedef class OStreamFilter:public IGFilter {
    private:
        ostream * pos;

    public:
        OStreamFilter (ostream & os) {
            pos = &os;
        };
        ~OStreamFilter () {
            pos->flush ();
        };

        virtual int writeln (const char *value);
} OStreamFilter;

typedef class DeltaFilter:public GFilterBase {
    private:
        G0G1Matcher g0g1;

    public:
        DeltaFilter (IGFilter & next);
        virtual int writeln (const char *value);
} DeltaFilter, *DeltaFilterPtr;

typedef class XYZFilter:public GFilterBase {
    private:
        G0G1Matcher g0g1;
		map<GCoord, GCoord> offsets;

    public:
        XYZFilter (IGFilter & next);
        virtual int writeln (const char *value);
		GCoord interpolate(GCoord pos);
		void setOffset(GCoord pos, GCoord offset);
} XYZFilter, *XYZFilterPtr;

}				// namespace gfilter

#endif
