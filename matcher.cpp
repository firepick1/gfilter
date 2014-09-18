#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <cctype>
#include "FireLog.h"
#include "gfilter.hpp"
#include "version.h"
#include "jansson.h"

using namespace std;
using namespace gfilter;

int
IGCodeMatcher::matchNumber (const char *text) {
    const char *s;
    bool isNumber = FALSE;
    bool loop = TRUE;

    for (s = text; loop; s++) {
        switch (*s) {
        case 0:
        default:
            loop = FALSE;
            break;
        case '\t':
        case ' ':
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
        case '+':
        case '.':
            isNumber = TRUE;
            break;
        }
    }

    return isNumber ? s-text-1 : 0;
}

int
GMoveMatcher::match(const char *text) {
    char *s;
	char *endPtr;
	bool loop = TRUE;

    code.clear ();
	coord = GCoord();
    for (s = (char *) text; loop; s++) {
        switch (*s) {
        case ' ':
            continue;
        case '\t':
            continue;
        case 'x':
        case 'X': {
			s++;
            int digits = IGCodeMatcher::matchNumber (s);
            if (digits) {
				endPtr = s+digits-1;
				coord.x = strtod(s, &endPtr);
            }
			s = endPtr-1;
            break;
        }
        case 'y':
        case 'Y': {
			s++;
            int digits = IGCodeMatcher::matchNumber (s);
            if (digits) {
				endPtr = s+digits-1;
				coord.y = strtod(s, &endPtr);
            }
			s = endPtr-1;
            break;
        }
        case 'z':
        case 'Z': {
			s++;
            int digits = IGCodeMatcher::matchNumber (s);
            if (digits) {
				endPtr = s+digits-1;
				coord.z = strtod(s, &endPtr);
            }
			s = endPtr-1;
            break;
        }
        case 'g':
        case 'G':
            s++;
            if ((*s == '0' || *s == '1') && !isdigit(s[1])) {
                code.append (s-1, 2);
			} else if (*s == '2' && s[1] == '8' && !isdigit(s[2])) {
                code.append (s-1, 3);
				s++;
			} else {
				s--;
            }
            break;
        default:
            loop = false;
			break;
        }
    }

    return code.empty() ? 0 : s-text-1;
}
