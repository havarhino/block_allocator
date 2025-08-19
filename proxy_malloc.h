#ifndef PROXY_MALLOC_H
#define PROXY_MALLOC_H

#include <stdlib.h>

#ifdef TEST_MALLOC
extern int malloc_count;
extern int malloc_fail_count;
extern int malloc_fail_trigger;
extern int malloc_fail_stop_trigger;
void* proxy_malloc(size_t size);
#define NORMAL_MALLOC() {malloc_fail_trigger = -1; malloc_fail_count=0; malloc_count=0;}
#define FAIL_MALLOC(a, b) {malloc_count=0; malloc_fail_count=0; malloc_fail_trigger=a; malloc_fail_stop_trigger=b;}
#define ASSERT_FAIL_COUNT(n) assert(malloc_fail_count == n)
#define malloc proxy_malloc
#else
#define NORMAL_MALLOC()
#define FAIL_MALLOC(a, b)
#define ASSERT_FAIL_COUNT(n)
#endif

#endif
