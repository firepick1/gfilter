#include <iostream>
#include "../gfilter.hpp"
#include <assert.h>

using namespace gfilter;

void testGCoord() {
	assert(GCoord() == GCoord(HUGE_VAL,HUGE_VAL,HUGE_VAL));
	assert(GCoord(1,2,3) == GCoord(1.0,2.0,3.0));
	assert(GCoord(2,2,2) != GCoord(1.0,2.0,3.0));
	assert(GCoord(3,3,3) == GCoord(-1,0,1)+GCoord(4,3,2));
	assert(GCoord(3,3,3) == GCoord(4,3,2)-GCoord(1,0,-1));
	assert(GCoord(3,30,300) == 3.0*GCoord(1,10,100));

	GCoord a(0,1,2);
	GCoord b(0,1,3);
	GCoord c(0,2,3);
	GCoord d(-1,0,0);

	assert(GCoord(1,0,0) == a.barycentric(a,b,c,d));
	assert(GCoord(0,1,0) == b.barycentric(a,b,c,d));
	assert(GCoord(0,0,1) == c.barycentric(a,b,c,d));
	assert(GCoord(0,0,0) == d.barycentric(a,b,c,d));
	GCoord pt1(0,1.5,2.6);
	double pt1_d2 = GCoord(.4,.1,.5).distance2(pt1.barycentric(a,b,c,d));
	assert(pt1_d2 < 1e-30);
	GCoord pt2(-.01,1.5,2.6);
	cout << pt2.barycentric(a,b,c,d) << endl;

	cout << "testGCoord() PASS" << endl;
}

void testMatchNumber() {
    assert(0 == IGCodeMatcher::matchNumber("abc"));
    assert(3 == IGCodeMatcher::matchNumber("123"));
    assert(7 == IGCodeMatcher::matchNumber(" \t-12.3"));

    cout << "testMatchNumber() PASS" << endl;
}

void testG0G1Matcher() {
    G0G1Matcher g0g1;

    assert(0 == g0g1.match("abc"));
    assert(2 == g0g1.match("g0"));
    assert(2 == g0g1.match("g1"));
    assert(0 == g0g1.match("g2"));
    assert(0 == g0g1.match("g00"));
    assert(0 == g0g1.match("g10"));
    assert(8 == g0g1.match("g1z1y2x3"));
    assert(g0g1.code == "g1");
    assert(g0g1.coord.x == 3.0);
    assert(g0g1.coord.y == 2.0);
    assert(g0g1.coord.z == 1.0);

    assert(16 == g0g1.match("g0x-12.345y+6.78"));
    assert(g0g1.code == "g0");
    assert(g0g1.coord.x == -12.345);
    assert(g0g1.coord.y== 6.78);
    assert(g0g1.coord.z == HUGE_VAL);

	cout << "testG0G1Matcher() PASS" << endl;
}

void testXYZFilter() {
    StringSink sink;
    XYZFilter xyz(sink);

    xyz.writeln("arbitrary-gcode");
    assert(sink.strings[0] == "arbitrary-gcode");
    xyz.writeln("g0x1y2z3");
    assert(sink.strings[1] == "G0X1.000000Y2.000000Z3.000000");
    xyz.writeln("g1y-12.345");
    assert(sink.strings[2] == "G1Y-12.345000");

    vector<GCoord> neighborhood;

	neighborhood = xyz.offsetNeighborhood(ORIGIN, 2);
	assert(0 == neighborhood.size());

	assert(0 == xyz.getOffsetRadius());
	xyz.setOffsetAt(GCoord(1,1,1), GCoord(.1,.1,.1));
	assert(sqrt(3) == xyz.getOffsetRadius());

	neighborhood = xyz.offsetNeighborhood(ORIGIN, 2);
	assert(1 == neighborhood.size());
	assert(GCoord(1,1,1) == neighborhood[0]);

	cout << "DEBUG ==> " << xyz.interpolate(GCoord(1,1,1)) << endl;
	assert(GCoord(1.1,1.1,1.1) == xyz.interpolate(GCoord(1,1,1)));
	cout << xyz.interpolate(GCoord(.5,1,1));
	//assert(GCoord(1.05,1,1) == xyz.interpolate(GCoord(.5,1,1)));
}

void testMat3x3() {
	Mat3x3 mat( 1, 2, 3, 
				0, 1, 4, 
				5, 6, 0);
	Mat3x3 matCopy( 1, 2, 3, 
					0, 1, 4, 
					5, 6, 0);
	cout << mat;
	assert(mat == matCopy);
	Mat3x3 matInv;
	mat.inverse(matInv);
	cout << matInv;
	assert(Mat3x3(-24,18,5,20,-15,-4,-5,4,1) == matInv);

	cout << "testMat3x3() PASS" << endl;
}

int main() {
	testMat3x3();
    testGCoord();
    testMatchNumber();
    testG0G1Matcher();
    testXYZFilter();
	cout << "ALL TESTS PASS" << endl;
}

