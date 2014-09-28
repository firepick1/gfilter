#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "FireLog.h"
#include "gfilter.hpp"
#include "version.h"
#include "jansson.h"
#include <string>
#include <vector>

using namespace std;
using namespace gfilter;

//////////////////// OStreamSink ////////////////
int
OStreamSink::writeln (const char *value) {
    (*pos) << value << endl;
    return 0;
};

//////////////////// StringSink  ////////////////
int
StringSink::writeln (const char *value) {
	strings.push_back(string(value));
    return 0;
};

////////////////// main ////////////////////////
static OStreamSink osf (cout);
static IGFilter * pHead = &osf;
static vector<IGFilterPtr> filters;

static void
help () {
    cout << "gfilter GCODE filter v" << VERSION_MAJOR << "." <<
         VERSION_MINOR << "." << VERSION_PATCH << endl;
    cout << "Copyright 2014, Karl Lew" << endl;
    cout << "https://github.com/firepick1/gfilter/wiki" << endl;
    cout << endl;
	cout << "USAGE:" << endl;
	cout << "gfilter --point-offset" << endl;
}

static bool
parseArgs (int argc, char *argv[], int &jsonIndent) {
    firelog_level (FIRELOG_INFO);

    if (argc <= 1) {
        return true;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == 0) {
            // empty argument
        } else if (strcmp ("-h", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0) {
            help ();
            exit (0);
        } else if (strcmp ("--point-offset", argv[i]) == 0) {
			LOGINFO("Create MappedPointFilter");
            MappedPointFilterPtr pXYZ = new MappedPointFilter (*pHead);
            pHead = pXYZ;
            filters.push_back (pXYZ);
        } else if (strcmp ("--delta", argv[i]) == 0) {
			LOGINFO("Create DeltaFilter");
            DeltaFilterPtr pDelta = new DeltaFilter (*pHead);
            pHead = pDelta;
            filters.push_back (pDelta);
        } else if (strcmp ("--warn", argv[i]) == 0) {
            firelog_level (FIRELOG_WARN);
        } else if (strcmp ("--error", argv[i]) == 0) {
            firelog_level (FIRELOG_ERROR);
        } else if (strcmp ("--info", argv[i]) == 0) {
            firelog_level (FIRELOG_INFO);
        } else if (strcmp ("--debug", argv[i]) == 0) {
            firelog_level (FIRELOG_DEBUG);
        } else if (strcmp ("--trace", argv[i]) == 0) {
            firelog_level (FIRELOG_TRACE);
        } else {
            LOGERROR1 ("unknown gcode argument: '%s'", argv[i]);
            return false;
        }
    }
    return true;
}

int
main (int argc, char *argv[]) {
    int jsonIndent = 2;
    bool argsOk = parseArgs (argc, argv, jsonIndent);

    if (!argsOk) {
        help ();
        exit (-1);
    }

    cout << pHead->name () << endl;

    for (string line; getline (cin, line);) {
        pHead->writeln (line.c_str ());
    }

    for (int i = 0; i < filters.size (); i++) {
        delete
        filters[i];
    }

    return 0;
}
