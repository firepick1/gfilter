#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "FireLog.h"
#include "gdelta.hpp"
#include "version.h"
#include "jansson.h"

using namespace std;
using namespace gdelta;

static void help() {
  cout << "gdelta GCODE Cartesian Delta filter v" << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << endl;
  cout << "Copyright 2014, Karl Lew" << endl;
  cout << "https://github.com/firepick1/gdelta/wiki" << endl;
  cout << endl;
}

bool parseArgs(int argc, char *argv[], 
  int &jsonIndent) 
{
  firelog_level(FIRELOG_INFO);
 
  if (argc <= 1) {
    return false;
  }

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == 0) {
      // empty argument
    } else if (strcmp("-warn", argv[i]) == 0) {
      firelog_level(FIRELOG_WARN);
    } else if (strcmp("-error", argv[i]) == 0) {
      firelog_level(FIRELOG_ERROR);
    } else if (strcmp("-info", argv[i]) == 0) {
      firelog_level(FIRELOG_INFO);
    } else if (strcmp("-debug", argv[i]) == 0) {
      firelog_level(FIRELOG_DEBUG);
    } else if (strcmp("-trace", argv[i]) == 0) {
      firelog_level(FIRELOG_TRACE);
    } else {
      LOGERROR1("unknown gcode argument: '%s'", argv[i]);
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  int jsonIndent = 2;
  bool argsOk = parseArgs(argc, argv, jsonIndent);
  if (!argsOk) {
    help();
    exit(-1);
  }

  cout << "hello";

  return 0;
}
