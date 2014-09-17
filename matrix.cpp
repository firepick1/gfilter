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
