#ifndef PROXY_ASSERT_H
#define PROXY_ASSERT_H

#include <assert.h>
#include <stdlib.h>

#ifdef TEST_ASSERT

extern int assert_failures;
extern int disable_assert;
void proxy_assert(int happy, const char * file, int line);
#define ENABLE_ASSERT() disable_assert = 0;
#define DISABLE_ASSERT() disable_assert = 1;
#define CLEAR_ASSERT_FAILURES() assert_failures = 0;
#define ASSERT_FAILURES(af) assert_failures == af
#define ASSERT(x) proxy_assert(x, __FILE__, __LINE__)

#else

#define CLEAR_ASSERT_FAILURES()
#define ASSERT_FAILURES(af) 1
#define ENABLE_ASSERT()
#define DISABLE_ASSERT()
#define ASSERT assert

#endif
#endif
