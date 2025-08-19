#include "proxy_malloc.h"
#include <stdio.h>

#ifdef TEST_MALLOC
int malloc_count = 0;
int malloc_fail_count = 0;
int malloc_fail_trigger = -1; // -1 means no failure
int malloc_fail_stop_trigger = -1; // -1 means no failure

void* proxy_malloc(size_t size) {
    /* printf("proxy_malloc(%zu, trigger_start=%d, trigger_stop=%d, fail_count=%d)\n",
            size,
            malloc_fail_trigger,
            malloc_fail_stop_trigger,
            malloc_fail_count);
    */
    if (malloc_fail_trigger >= 0 && (malloc_count >= malloc_fail_trigger) &&
            ( malloc_fail_stop_trigger >= 0 && malloc_count < malloc_fail_stop_trigger)) {
        malloc_fail_count++;
        //printf("fail_count=%d)\n", malloc_fail_count);
        return NULL;
    }
    malloc_count++;
    #undef malloc // Temporarily undefine to call standard malloc
    void* ptr = malloc(size);
    #define malloc proxy_malloc // Restore the macro
    return ptr;
}
#endif
