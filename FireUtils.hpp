#ifndef TINYASSERT_HPP
#define TINYASSERT_HPP

#include <assert.h>
#include <iostream>
#include "FireLog.h"
#include "string.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef bool
#define bool int
#endif

inline int fail(int rc) {
    std::cout << "***ASSERT FAILED*** expected:0 actual:" << rc << std::endl;
    return FALSE;
}
#define ASSERTZERO(exp) {int rc; assert(0==(rc=exp) || fail(rc));}
#define ASSERTNONZERO(exp, context) assertnonzero((long) exp, context)

inline void
assertnonzero(long actual, const char* context) {
    if (actual) {
        return;
    }

    char buf[255];
    snprintf(buf, sizeof(buf), "%s expected non-zero", context);
    LOGERROR(buf);
    std::cout << "***ASSERT FAILED*** " << buf << std::endl;
    assert(false);
}

inline void
ASSERTEQUAL(double expected, double actual, const char* context, double tolerance=0) {
    double diff = expected - actual;
    if (-tolerance <= diff && diff <= tolerance) {
        return;
    }

    char buf[255];
    snprintf(buf, sizeof(buf), "%s expected:%lf actual:%lf tolerance:%lf",
             context, expected, actual, tolerance);
    LOGERROR(buf);
    std::cout << "***ASSERT FAILED*** " << buf << std::endl;
    assert(false);
}

inline void
ASSERTEQUAL(const char* expected, const char* actual, const char* context) {
    if (actual && strcmp(expected, actual)==0) {
        return;
    }

    char buf[255];
    if (actual) {
        snprintf(buf, sizeof(buf), "%s expected:%s actual:%s", context, expected, actual);
    } else {
        snprintf(buf, sizeof(buf), "%s expected:%s actual:NULLs", context, expected);
    }
    LOGERROR(buf);
    std::cout << "***ASSERT FAILED*** " << buf << std::endl;
    assert(false);
}

#endif
