#include "../gfilter.hpp"
#include <errno.h>

using namespace gfilter;

void testJSONConfig() {
    const char *json =
        "{ \"map\":[ " \
        "{\"domain\":[0,0,0], \"range\":[0,0,0]}, " \
        "{\"domain\":[0,0,1], \"range\":[0,0,1.01]}," \
        "{\"domain\":[0,1,0], \"range\":[0,1,0.02]}," \
        "{\"domain\":[0,1,1], \"range\":[0,1,1.02]} " \
        "]}";
    json_error_t jerr;
    json_t *config = json_loads(json, 0, &jerr);
    StringSink sink;
    MappedPointFilter pof(sink, config);

    pof.writeln("G0X0Y0Z1 E3F4");
    ASSERTEQUALS("G0X0Y0Z1.01E3F4", sink[0].c_str());
    pof.writeln("G28 Z1 Y0");
    ASSERTEQUALS("G28X0Y0Z0", sink[1].c_str());
}

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
    assert(GCoord(0.37,0.11,0.51).distance2(pt2.barycentric(a,b,c,d)) < 1e-10);
    GCoord pt3(+.01,1.5,2.6);
    assert(GCoord(0.43,0.09,0.49).distance2(pt3.barycentric(a,b,c,d)) < 1e-10);

    cout << "testGCoord() PASS" << endl;
}

void testMatchNumber() {
    assert(0 == IGCodeMatcher::matchNumber("abc"));
    assert(3 == IGCodeMatcher::matchNumber("123"));
    assert(7 == IGCodeMatcher::matchNumber(" \t-12.3"));

    cout << "testMatchNumber() PASS" << endl;
}

void testGMoveMatcher() {
    GMoveMatcher matcher;

    assert(0 == matcher.match("abc"));
    assert(2 == matcher.match("g0"));
    assert(2 == matcher.match("g1"));
    assert(0 == matcher.match("g2"));
    assert(0 == matcher.match("g00"));
    assert(0 == matcher.match("g10"));
    assert(8 == matcher.match("g1z1y2x3"));
    ASSERTEQUALS("g1",matcher.code.c_str());
    assert(matcher.coord.x == 3.0);
    assert(matcher.coord.y == 2.0);
    assert(matcher.coord.z == 1.0);

    assert(16 == matcher.match("g0x-12.345y+6.78"));
    assert(matcher.code == "g0");
    assert(matcher.coord.x == -12.345);
    assert(matcher.coord.y== 6.78);
    assert(matcher.coord.z == HUGE_VAL);

    ASSERTEQUAL(5, matcher.match("g28X0"));
    assert(0 == matcher.match("g281X0"));

    cout << "testGMoveMatcher() PASS" << endl;
}


#define ASSERTGCOORD(e,a) assertGCoord(e,a,__FILE__,__LINE__)
void assertGCoord(GCoord expected, GCoord actual, const char *fname, long line) {
    double dist2 = expected.distance2(actual);
    if (dist2 > 1e-10) {
        cout << "ASSERTGCOORD expected:" << expected << " actual:" << actual
             << " " << fname << "@" << line << endl;
        assert(false);
    } else {
		cout << "ASSERTGCOORD(" << actual.toString() << ") OK" << endl;
	}
}

void testMappedPointFilter() {
    cout << "testMappedPointFilter() -------- BEGIN -----" << endl;
    StringSink sink;
    MappedPointFilter xyz(sink);

    xyz.writeln("arbitrary-gcode");
    assert(sink.strings[0] == "arbitrary-gcode");
    xyz.writeln("g0x1y2z3");
    ASSERTEQUALS("G0X1Y2Z3", sink.strings[1].c_str());
    xyz.writeln("g1y-12.345");
    ASSERTEQUALS("G1X1Y-12.345Z3", sink.strings[2].c_str());

    vector<MappedPoint> neighborhood;

    neighborhood = xyz.domainNeighborhood(ORIGIN, 2);
    assert(0 == neighborhood.size());

    assert(0 == xyz.getDomainRadius());
    xyz.mapPoint(GCoord(1,1,1), GCoord(1.1,1.1,1.1));
    assert(sqrt(3) == xyz.getDomainRadius());

    neighborhood = xyz.domainNeighborhood(ORIGIN, 2);
    ASSERTEQUAL(1, neighborhood.size());
    cout << neighborhood[0] << endl;
    assert(GCoord(1,1,1) == neighborhood[0].domain);
    assert(GCoord(1.1,1.1,1.1) == neighborhood[0].range);
    assert(GCoord(1.1,1.1,1.1) == xyz.interpolate(GCoord(1,1,1)));

    /// Verify that single MappedPoint specifies a universal mapping
    ASSERTGCOORD(GCoord(-99.9,100.1,0.101), xyz.interpolate(GCoord(-100,100,.001)));
    ASSERTGCOORD(GCoord(0.1,0.1,1.1), xyz.interpolate(GCoord(0,0,1)));

    // Test neighborhood calculation distance
    xyz.mapPoint(GCoord(2,2,2), GCoord(-.1,-.1,.1));
    neighborhood = xyz.domainNeighborhood(ORIGIN, 2);
    assert(1 == neighborhood.size());
    assert(GCoord(1,1,1) == neighborhood[0].domain);
    neighborhood = xyz.domainNeighborhood(GCoord(2,2,2) , 2);
    assert(2 == neighborhood.size());
    neighborhood = xyz.domainNeighborhood(GCoord(3,3,3) , 2);
    assert(1 == neighborhood.size());
    assert(GCoord(2,2,2) == neighborhood[0].domain);

    cout << "Verify that point mappings can be changed..." << endl;
    xyz.mapPoint(GCoord(1,1,1), GCoord(.1,.01,.001));
    neighborhood = xyz.domainNeighborhood(ORIGIN, 2);
    for (int i = 0; i < neighborhood.size(); i++) {
        cout << neighborhood[i] << endl;
    }
    assert(1 == neighborhood.size());
    assert(GCoord(1,1,1) == neighborhood[0].domain);
    assert(GCoord(.1,.01,.001) == neighborhood[0].range);

    cout << "Testing unit lattice from (1,1,1) to (2,2,2)" << endl;
    xyz.mapPoint(GCoord(1,1,1), GCoord(.1,.01,.001));
    xyz.mapPoint(GCoord(1,1,2), GCoord(.1,.01,.002));
    xyz.mapPoint(GCoord(1,2,1), GCoord(.1,.02,.001));
    xyz.mapPoint(GCoord(1,2,2), GCoord(.1,.02,.002));
    xyz.mapPoint(GCoord(2,1,1), GCoord(.2,.01,.001));
    xyz.mapPoint(GCoord(2,1,2), GCoord(.2,.01,.002));
    xyz.mapPoint(GCoord(2,2,1), GCoord(.2,.02,.001));
    xyz.mapPoint(GCoord(2,2,2), GCoord(.2,.02,.002));
    ASSERTEQUAL(sqrt(3), xyz.getDomainRadius());
    ASSERTGCOORD(GCoord(0.15,0.015,0.0015), xyz.interpolate(GCoord(1.5,1.5,1.5)));
    ASSERTGCOORD(GCoord(0.1,0.01,0.0015), xyz.interpolate(GCoord(1,1,1.5)));
    ASSERTGCOORD(GCoord(-20,-20,-20), xyz.interpolate(GCoord(-20,-20,-20)));
    ASSERTGCOORD(GCoord(1,1,-2), xyz.interpolate(GCoord(1,1,-2)));
    ASSERTGCOORD(GCoord(.2,0.01,.002), xyz.interpolate(GCoord(2,1,2)));
    ASSERTGCOORD(GCoord(.21,0.021,.0021), xyz.interpolate(GCoord(2.1,2.1,2.1)));
    ASSERTGCOORD(GCoord(0.2,0.02,0.002), xyz.interpolate(GCoord(2.9,2.9,2.9))); // N=1

    cout << "Testing neighborhood starvation with points exterior to lattice" << endl;
    ASSERTGCOORD(GCoord(0.19,0.019,0.0021), xyz.interpolate(GCoord(1.9,1.9,2.1))); // N=8
    // Degenerate tetrahedron (note sudden and unfortunate transition in interpolated values)
    ASSERTGCOORD(GCoord(0.165677,0.0165677,0.002), xyz.interpolate(GCoord(1.9,1.9,2.4))); // N=7
    ASSERTGCOORD(GCoord(0.163,0.0163,0.002), xyz.interpolate(GCoord(1.9,1.9,2.5))); // N=5
    ASSERTGCOORD(GCoord(0.159278,0.0159278,0.002), xyz.interpolate(GCoord(1.9,1.9,2.7))); // N=5
    ASSERTGCOORD(GCoord(0.157965,0.0157965,0.002), xyz.interpolate(GCoord(1.9,1.9,2.8))); // N=4
    ASSERTGCOORD(GCoord(0.156899,0.0156899,0.002), xyz.interpolate(GCoord(1.9,1.9,2.9))); // N=4
    ASSERTGCOORD(GCoord(0.156024,0.0156024,0.002), xyz.interpolate(GCoord(1.9,1.9,3.0))); // N=4
    ASSERTGCOORD(GCoord(0.169175,0.0169175,0.002), xyz.interpolate(GCoord(1.9,1.9,3.2))); // N=3
    ASSERTGCOORD(GCoord(0.168862,0.0168862,0.002), xyz.interpolate(GCoord(1.9,1.9,3.3))); // N=3
    ASSERTGCOORD(GCoord(0.168602,0.0168602,0.002), xyz.interpolate(GCoord(1.9,1.9,3.4))); // N=3
    ASSERTGCOORD(GCoord(0.2,0.02,0.002), xyz.interpolate(GCoord(1.9,1.9,3.5))); // N=1
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

string loadFile(const char *path) {
    LOGINFO1("loadFile(%s)", path);
    FILE *file = fopen(path, "r");
    if (file == 0) {
        LOGERROR1("loadFile() fopen(%s) failed", path);
        ASSERTZERO(-ENOENT);
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
	char * pData = (char *) malloc(length+1);
    assert(pData);
    LOGINFO2("loadFile(%s) fread(%ld)", path, (long)length);
    size_t bytesRead = fread(pData, 1, length, file);
    if (bytesRead != length) {
        LOGERROR2("loadFile(), fread failed expected:%ldB actual%ldB", length, bytesRead);
        exit(-EIO);
    }
    LOGINFO2("loadFile(%s) loaded %ldB", path, (long) bytesRead);
    fclose(file);
	pData[length] = 0;
	string result(pData);
	free(pData);
    return result;
}

void testCenter() {
	cout << "testCenter() BEGIN -------" << endl;

	string json = loadFile("test/fiducial.json");
    json_error_t jerr;
    json_t *config = json_loads(json.c_str(), 0, &jerr);
    StringSink sink;
    MappedPointFilter pof(sink, config);
	pof.setDomainRadius(24);// depends on grid sampling distance and pixels/mm	

	int line=0;
	const char *gcode;

	gcode = "G0X0Y0Z0";
	pof.writeln(gcode);
	cout << gcode << " -> " << sink[line] << endl;
	ASSERTEQUALS("G0X13.3419Y-0.541237Z0", sink[line].c_str());
	line++;

	gcode = "G0 X11 Y-112 Z0";
	pof.writeln(gcode);
	cout << gcode << " -> " << sink[line] << endl;
	ASSERTEQUALS("G0X0Y10Z0", sink[line].c_str());
	line++;

	cout << "testCenter() PASS" << endl;
}

int main() {
    firelog_init("target/test.log", FIRELOG_TRACE);

    testJSONConfig();
    testMat3x3();
    testGCoord();
    testMatchNumber();
    testGMoveMatcher();
    testMappedPointFilter();
	testCenter();

    cout << "ALL TESTS PASS" << endl;
}

