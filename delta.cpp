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

DeltaFilter::DeltaFilter(IGFilter &next) : GFilterBase(next) {
  _name = "DeltaFilter";
}

int DeltaFilter::writeln(const char *value) {
   if (strcmp(value,"G0") == 0) {
	   _next.writeln("G0X0Y0Z");
   } else {
	   _next.writeln(value);
   }

   return 0;
}

