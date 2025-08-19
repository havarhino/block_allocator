#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_allocator.h"
#include "proxy_assert.h"
#include "proxy_malloc.h"

// Compile-time configuration
#ifndef ENABLE_DEBUG_HEADER
#define ENABLE_DEBUG_HEADER 0
#endif

// Test framework macros
#define TEST(name) void test_##name(void)
#define RUN_TEST(name) do { printf("Running %s...\n", #name); test_##name(); tests_passed++; } while(0)
static int tests_passed = 0;

// Helper to check if a block is allocated
static int is_block_allocated(BlockAllocator* alloc, size_t index) {
    return test_bit(alloc->bitmap, index);
}

#define BLOCK_SIZE (640)
#define TOTAL_SIZE (4 * 1024 * 1024)

// Test initialization
TEST(init_allocator) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    assert(alloc->memory != NULL);
    assert(alloc->bitmap != NULL);
    for (size_t i = 0; i < (alloc->total_blocks + 7) / 8; i++) {
        assert(alloc->bitmap[i] == 0);
    }
    free_allocator(alloc);
}

// Test small initialization
TEST(init_small_allocator) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(12, 12);
    assert(alloc != NULL);
    assert(alloc->memory != NULL);
    assert(alloc->bitmap != NULL);
    for (size_t i = 0; i < (alloc->total_blocks + 7) / 8; i++) {
        assert(alloc->bitmap[i] == 0);
    }
    void* ptr = BLOCK_ALLOC(alloc);
    assert(ptr != NULL);
    assert(is_block_allocated(alloc, 0));
    assert(is_allocated(alloc, ptr));
    print_block(alloc, ptr);

    void* ptr2 = BLOCK_ALLOC(alloc);
    assert(ptr2 == NULL);

    BLOCK_FREE(alloc, ptr);
    assert(!is_block_allocated(alloc, 0));

    free_allocator(alloc);
}

// Test small initialization
TEST(free_pointer_left_of_heap) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(12, 128);
    assert(alloc != NULL);
    assert(alloc->memory != NULL);
    assert(alloc->bitmap != NULL);
    for (size_t i = 0; i < (alloc->total_blocks + 7) / 8; i++) {
        assert(alloc->bitmap[i] == 0);
    }
    void* ptr = BLOCK_ALLOC(alloc);
    assert(ptr != NULL);
    assert(is_block_allocated(alloc, 0));
    assert(is_allocated(alloc, ptr));
    print_block(alloc, ptr);

    DISABLE_ASSERT();
    CLEAR_ASSERT_FAILURES();
    assert(ASSERT_FAILURES(0));
    BLOCK_FREE(alloc, (void *)((uint8_t*)ptr - alloc->block_size));
    assert(ASSERT_FAILURES(1));
    ENABLE_ASSERT();

    free_allocator(alloc);
}

// Test small initialization
TEST(stomp_before_and_after_data_block) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();

    BlockAllocator* alloc = init_allocator(8, 16);
    assert(alloc != NULL);

    void* ptr = BLOCK_ALLOC(alloc);
    uint8_t* stomp_ptr = (uint8_t*)ptr - 3;

    assert(ptr != NULL);
    assert(is_block_allocated(alloc, 0));
    printf("Before stomping pre-data\n");
    print_block(alloc, ptr);
    *stomp_ptr = 0;
    printf("After stomping 3rd byte before start of data\n");
    print_block(alloc, ptr);

    void* ptr2 = BLOCK_ALLOC(alloc);
    assert(ptr2 != NULL);
    printf("2nd alloc before stomping post-data\n");
    print_block(alloc, ptr2);
    stomp_ptr = (uint8_t*)ptr2 + alloc->block_data_size;
    stomp_ptr += 2;
    *stomp_ptr = 0xFF;
    printf("2nd alloc after stomping post-data\n");
    print_block(alloc, ptr2);

    DISABLE_ASSERT();
    CLEAR_ASSERT_FAILURES();
    BLOCK_FREE(alloc, ptr);
    assert(ASSERT_FAILURES(1));

    CLEAR_ASSERT_FAILURES();
    BLOCK_FREE(alloc, ptr2);
    assert(ASSERT_FAILURES(1));

    ENABLE_ASSERT();
    CLEAR_ASSERT_FAILURES();
    free_allocator(alloc);
}

// Test small initialization
TEST(stomp_without_free) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();

    BlockAllocator* alloc = init_allocator(8, 16);
    assert(alloc != NULL);

    void* ptr = BLOCK_ALLOC(alloc);
    uint8_t* stomp_ptr = (uint8_t*)ptr - 3;

    assert(ptr != NULL);
    assert(is_block_allocated(alloc, 0));
    *stomp_ptr = 0;

    DISABLE_ASSERT();
    CLEAR_ASSERT_FAILURES();
    assert(ASSERT_FAILURES(0));
    free_allocator(alloc);
    assert(ASSERT_FAILURES(1));
    ENABLE_ASSERT();
}

#ifdef TEST_MALLOC
// Test init failure (mock malloc failure)
TEST(init_allocator_malloc_failure) {
    // Test failure on alloc
    //printf("init_allocator #1\n--------------------\n");
    ENABLE_ASSERT();
    FAIL_MALLOC(0, 1);
    BlockAllocator* alloc1 = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc1 == NULL);
    ASSERT_FAIL_COUNT(1);

    // Test failure on memory allocation
    //printf("init_allocator #2\n--------------------\n");
    FAIL_MALLOC(1, 2);
    BlockAllocator* alloc2 = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc2 == NULL);
    ASSERT_FAIL_COUNT(2);

    // Test failure on bitmap allocation
    //printf("init_allocator #3\n--------------------\n");
    FAIL_MALLOC(2, 3);
    BlockAllocator* alloc3 = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc3 == NULL);
    ASSERT_FAIL_COUNT(1);
}
#endif // TEST_MALLOC

// Test single allocation
TEST(single_allocation) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void* ptr = BLOCK_ALLOC(alloc);
    assert(ptr != NULL);
    assert(is_block_allocated(alloc, 0));
    BLOCK_FREE(alloc, ptr);
    assert(!is_block_allocated(alloc, 0));
    free_allocator(alloc);
}

// Test multiple allocations
TEST(multiple_allocations) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void* ptrs[5];
    for (int i = 0; i < 5; i++) {
        ptrs[i] = BLOCK_ALLOC(alloc);
        assert(ptrs[i] != NULL);
        assert(is_block_allocated(alloc, i));
    }
    for (int i = 0; i < 5; i++) {
        BLOCK_FREE(alloc, ptrs[i]);
        assert(!is_block_allocated(alloc, i));
    }
    free_allocator(alloc);
}

// Test null allocator
TEST(null_allocator) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    void* ptr = BLOCK_ALLOC(NULL);
    assert(ptr == NULL);
    BLOCK_FREE(NULL, NULL);
    BLOCK_FREE(NULL, (void*)0x1234);
}

// Test invalid free
TEST(invalid_free) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void* ptr = BLOCK_ALLOC(alloc);
    assert(ptr != NULL);
#ifdef TEST_ASSERT
    // Try freeing invalid pointer (misaligned)
    DISABLE_ASSERT();
    CLEAR_ASSERT_FAILURES();
    BLOCK_FREE(alloc, (char*)ptr + 1); // Misaligned
    assert(ASSERT_FAILURES(1));
    assert(is_block_allocated(alloc, 0)); // Should still be allocated
    // Try freeing pointer outside allocator range
    CLEAR_ASSERT_FAILURES();
    BLOCK_FREE(alloc, (void*)0x1); // Completely invalid
    assert(ASSERT_FAILURES(1));
    assert(is_block_allocated(alloc, 0)); // Should still be allocated

    ENABLE_ASSERT();
    CLEAR_ASSERT_FAILURES();
#endif
    BLOCK_FREE(alloc, ptr);
    assert(!is_block_allocated(alloc, 0));
    free_allocator(alloc);
}

// Test full allocator
TEST(full_allocator) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void** ptrs = malloc(sizeof(void*) * alloc->total_blocks);
    assert(ptrs != NULL);
    for (size_t i = 0; i < alloc->total_blocks; i++) {
        ptrs[i] = BLOCK_ALLOC(alloc);
        assert(ptrs[i] != NULL);
        assert(is_block_allocated(alloc, i));
    }
    void* extra = BLOCK_ALLOC(alloc);
    assert(extra == NULL); // Allocator full
    for (size_t i = 0; i < alloc->total_blocks; i++) {
        BLOCK_FREE(alloc, ptrs[i]);
        assert(!is_block_allocated(alloc, i));
    }
    free(ptrs);
    free_allocator(alloc);
}

// Test debug header (when enabled)
#if ENABLE_DEBUG_HEADER
TEST(debug_header) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void* ptr = BLOCK_ALLOC(alloc);
    assert(ptr != NULL);
    DebugHeader* header = (DebugHeader*)((char*)ptr - alloc->data_offset);
    assert(header->file != NULL);
    assert(header->line > 0);
    BLOCK_FREE(alloc, ptr);
    free_allocator(alloc);
}
#endif

// Test dump_allocator null handling (when enabled)
#if ENABLE_DEBUG_HEADER
TEST(dump_allocator_null) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    dump_allocator(NULL);
}
#endif

// Test dump_allocator with allocated blocks (when enabled)
#if ENABLE_DEBUG_HEADER
TEST(dump_allocator) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void* ptr1 = BLOCK_ALLOC(alloc);
    void* ptr2 = BLOCK_ALLOC(alloc);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    dump_allocator(alloc); // Should print allocator state with 2 blocks allocated
    BLOCK_FREE(alloc, ptr1);
    BLOCK_FREE(alloc, ptr2);
    dump_allocator(alloc); // Should print allocator state with 2 blocks allocated
    free_allocator(alloc);
}
#endif

// Test bitmap operations
TEST(bitmap_operations) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    set_bit(alloc->bitmap, 0);
    assert(is_block_allocated(alloc, 0));
    clear_bit(alloc->bitmap, 0);
    assert(!is_block_allocated(alloc, 0));
    set_bit(alloc->bitmap, alloc->total_blocks - 1);
    assert(is_block_allocated(alloc, alloc->total_blocks - 1));
    clear_bit(alloc->bitmap, alloc->total_blocks - 1);
    assert(!is_block_allocated(alloc, alloc->total_blocks - 1));
    free_allocator(alloc);
}

// Test mixed allocation patterns
TEST(mixed_allocation) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void* ptr1 = BLOCK_ALLOC(alloc);
    void* ptr2 = BLOCK_ALLOC(alloc);
    BLOCK_FREE(alloc, ptr1);
    void* ptr3 = BLOCK_ALLOC(alloc);
#if ENABLE_DEBUG_HEADER
    assert(ptr3 == (char*)alloc->memory + alloc->data_offset);
#else
    assert(ptr3 == (char*)alloc->memory);
#endif
    BLOCK_FREE(alloc, ptr2);
    BLOCK_FREE(alloc, ptr3);
    free_allocator(alloc);
}

// Test bitmap scanning with nearly full allocator
TEST(nearly_full_allocator) {
    ENABLE_ASSERT();
    NORMAL_MALLOC();
    BlockAllocator* alloc = init_allocator(BLOCK_SIZE, TOTAL_SIZE);
    assert(alloc != NULL);
    void** ptrs = malloc(sizeof(void*) * (alloc->total_blocks - 1));
    assert(ptrs != NULL);
    // Allocate all but the last block
    for (size_t i = 0; i < alloc->total_blocks - 1; i++) {
        ptrs[i] = BLOCK_ALLOC(alloc);
        assert(ptrs[i] != NULL);
        assert(is_block_allocated(alloc, i));
    }
    // Allocate the last block
    void* last = BLOCK_ALLOC(alloc);
    assert(last != NULL);
    assert(is_block_allocated(alloc, alloc->total_blocks - 1));
    // Free all blocks
    for (size_t i = 0; i < alloc->total_blocks - 1; i++) {
        BLOCK_FREE(alloc, ptrs[i]);
    }
    BLOCK_FREE(alloc, last);
    free(ptrs);
    free_allocator(alloc);
}

int main() {
    printf("Starting unit tests...\n");
    RUN_TEST(init_allocator);
    RUN_TEST(init_small_allocator);
    RUN_TEST(free_pointer_left_of_heap);
#ifdef TEST_MALLOC
    RUN_TEST(init_allocator_malloc_failure);
#endif
    RUN_TEST(stomp_without_free);
    RUN_TEST(stomp_before_and_after_data_block);
    RUN_TEST(single_allocation);
    RUN_TEST(multiple_allocations);
    RUN_TEST(null_allocator);
    RUN_TEST(invalid_free);
    RUN_TEST(full_allocator);
#if ENABLE_DEBUG_HEADER
    RUN_TEST(debug_header);
    RUN_TEST(dump_allocator_null);
    RUN_TEST(dump_allocator);
#endif
    RUN_TEST(bitmap_operations);
    RUN_TEST(mixed_allocation);
    RUN_TEST(nearly_full_allocator);
    printf("All %d tests passed!\n", tests_passed);
    return 0;
}
