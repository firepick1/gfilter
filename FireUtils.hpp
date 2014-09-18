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
    std::cerr << "***ASSERT FAILED*** " << buf << std::endl;
    assert(false);
}

#define ASSERTEQUAL(e,a) assertEqual((double)e,(double)a,0,__FILE__,__LINE__)
#define ASSERTEQUALT(e,a,t) assertEqual(e,a,t,__FILE__,__LINE__)
inline void
assertEqual(double expected, double actual, double tolerance, const char* context, long line)
{
    double diff = expected - actual;
    if (-tolerance <= diff && diff <= tolerance) {
        return;
    }

    char buf[255];
    snprintf(buf, sizeof(buf), "%s expected:%lf actual:%lf tolerance:%lf line:%ld",
             context, expected, actual, tolerance, line);
    LOGERROR(buf);
    std::cerr << "***ASSERT FAILED*** " << buf << std::endl;
    assert(false);
}

#define ASSERTEQUALS(e,a) assertEqual(e,a,__FILE__,__LINE__)
inline void
assertEqual(const char* expected, const char* actual, const char* context, int line) {
    if (actual && strcmp(expected, actual)==0) {
        return;
    }

    char buf[255];
    if (actual) {
        snprintf(buf, sizeof(buf), "%s@%d expected:%s actual:%s", context, line, expected, actual);
    } else {
        snprintf(buf, sizeof(buf), "%s@%d expected:%s actual:NULLs", context, line, expected);
    }
    LOGERROR(buf);
    std::cerr << "***ASSERT FAILED*** " << buf << std::endl;
    assert(false);
}

#endif
