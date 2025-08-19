#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

// Debug header structure
#if ENABLE_DEBUG_HEADER
typedef struct {
    const char* file;
    uint32_t line;
} DebugHeader;
#endif

#if ENABLE_STOMP_DETECT
extern uint32_t pre_stomp_pattern_array[];
extern uint32_t post_stomp_pattern_array[];
#define PRE_BUFFER_STOMP_GUARD_SIZE sizeof(pre_stomp_pattern_array)
#define POST_BUFFER_STOMP_GUARD_SIZE sizeof(post_stomp_pattern_array)
#endif

// Allocator block structure
typedef struct BlockAllocator {
    uint8_t* memory;        // Base memory pool
    uint8_t* bitmap;        // Bitmap for tracking free/used blocks
    size_t total_blocks;    // Total number of blocks
    size_t block_size;      // The fixed size of each block
    size_t total_size;      // The entire continguous allocated memory used by allocator
    size_t block_data_size; // The number of bytes in block allocated to client data
    size_t data_offset;     // The number of bytes from the beginning of the block to the
                            // first byte of client data.
} BlockAllocator;

BlockAllocator* init_allocator(size_t block_size, size_t total_size);
void free_allocator(BlockAllocator* alloc);
void* alloc_block(BlockAllocator* alloc, const char* file, int line);
void free_block(BlockAllocator* alloc, void* ptr);
void check_for_stomps(BlockAllocator* alloc);
void set_bit(uint8_t* bitmap, size_t index);
void clear_bit(uint8_t* bitmap, size_t index);
int test_bit(uint8_t* bitmap, size_t index);
int is_allocated(BlockAllocator* alloc, void *);
void print_block(BlockAllocator* alloc, void* ptr);
#if ENABLE_DEBUG_HEADER
void dump_allocator(BlockAllocator* alloc);
#endif

// Client-facing macros
#if ENABLE_DEBUG_HEADER
#define BLOCK_ALLOC(alloc) alloc_block((alloc), __FILE__, __LINE__)
#else
#define BLOCK_ALLOC(alloc) alloc_block((alloc), NULL, 0)
#endif
#define BLOCK_FREE(alloc, ptr) free_block((alloc), (ptr))

#endif
