#include <stdio.h>
#include "block_allocator.h"

int main() {
    BlockAllocator* alloc = init_allocator(640, 5*1024*1024);
    if (!alloc) {
        printf("Failed to initialize allocator\n");
        return 1;
    }

    // Allocate 5 blocks
    void* ptrs[5];
    for (int i = 0; i < 5; i++) {
        ptrs[i] = BLOCK_ALLOC(alloc);
        if (ptrs[i]) {
            printf("Allocated block %d at %p\n", i, ptrs[i]);
        } else {
            printf("Failed to allocate block %d\n", i);
        }
    }

    // Free the blocks
    for (int i = 0; i < 5; i++) {
        if (ptrs[i]) {
            BLOCK_FREE(alloc, ptrs[i]);
            printf("Freed block %d\n", i);
        }
    }

    free_allocator(alloc);
    return 0;
}
