#include <assert.h>
#include <stdio.h>
#include "proxy_assert.h"

#ifdef TEST_ASSERT
int disable_assert = 0;
int assert_failures = 0;

void proxy_assert(int happy, const char * file, int line) {
    if (disable_assert) {
        if (!happy) {
            printf("proxy_assert (%s: %d): Failed assert)\n", file, line);
            assert_failures++;
        }
        return;
    } else {
        if (!happy) printf("\nAbout to ASSERT for File: %s: %d\n", file, line);
        assert(happy);
    }
}
#endif
