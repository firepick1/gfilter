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

////////////// GCoord /////////////

GCoord GCoord::barycentric(const GCoord &c1, const GCoord &c2, const GCoord &c3, const GCoord &c4) {
	Mat3x3 T(c1.x-c4.x, c2.x-c4.x, c3.x-c4.x,
			 c1.y-c4.y, c2.y-c4.y, c3.y-c4.y,
			 c1.z-c4.z, c2.z-c4.z, c3.z-c4.z);
	Mat3x3 Tinv;
	assert(T.inverse(Tinv));
	GCoord cc4 = (*this) - c4;

	return Tinv*cc4;
}

GCoord gfilter::operator*(Mat3x3 &mat, GCoord &c) {
	GCoord result(
		mat.at(0,0)*c.x + mat.at(0,1)*c.y + mat.at(0,2)*c.z,
		mat.at(1,0)*c.x + mat.at(1,1)*c.y + mat.at(1,2)*c.z,
		mat.at(2,0)*c.x + mat.at(2,1)*c.y + mat.at(2,2)*c.z);
	return result;
}

/////////////////// Mat3x3 /////////////////////
// Thank you, Mr. Ree

bool gfilter::operator==(const Mat3x3& lhs, const Mat3x3& rhs) {
    for (int y = 0; y < 3; y++) {
        for (int x=0; x<3; x++ ) {
            if (lhs.mat[y][x] != rhs.mat[y][x]) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

ostream& gfilter::operator<<(ostream& os, const Mat3x3& that) {
    for (int y = 0; y < 3; y++) {
        os << "[  ";
        for (int x=0; x<3; x++ ) {
            os << that.mat[y][x] << "  ";
        }
        os << "]" << endl;
    }
    os << endl;
    return os;
}

bool Mat3x3::inverse(Mat3x3 &matInv) {
    double det =det3x3();

    if ( abs(det) < 1e-2 ) {
        matInv.clear();
        return FALSE;
    }

    double detReciprocal = 1.0/det;

    for (int y = 0; y < 3;  y++) {
        for (int x = 0; x < 3;  x++) {
            matInv.mat[y][x] = det2x2(x,y) * detReciprocal;

            if ( 1 == ((x + y) % 2) ) {
                matInv.mat[y][x] = -matInv.mat[y][x];
            }
        }
    }

    return TRUE;
}
