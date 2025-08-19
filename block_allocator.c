#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "block_allocator.h"
#include "proxy_assert.h"
#include "proxy_malloc.h"

// Compile-time configuration
#ifndef ENABLE_DEBUG_HEADER
#define ENABLE_DEBUG_HEADER 0
#endif
#ifndef ENABLE_STOMP_DETECT
#define ENABLE_STOMP_DETECT 0
#endif

uint32_t pre_stomp_pattern_array[] = {0xDECAFBADu, 0x5A5A5A5Au};
uint32_t post_stomp_pattern_array[] = {0xDEADFADEu, 0xC5C5C5C5u};

// Initialize the allocator
// Note: If ENABLE_DEBUG_HEADER is defined, the total_size of the allocation
// will exceed the requested total_size to ensure there exists the usable
// space of block_size * total_size in bytes.
BlockAllocator* init_allocator(size_t block_size, size_t total_size) {
    BlockAllocator* alloc = malloc(sizeof(BlockAllocator));
    if (!alloc) return NULL;
    ASSERT(block_size > 0);
    ASSERT(total_size >= block_size);
    alloc->block_data_size = block_size;

    alloc->total_blocks = total_size / block_size;

    alloc->data_offset = 0;
#if ENABLE_DEBUG_HEADER
    block_size += sizeof(DebugHeader);
    alloc->data_offset += sizeof(DebugHeader);
#endif
#if ENABLE_STOMP_DETECT
    block_size += PRE_BUFFER_STOMP_GUARD_SIZE + POST_BUFFER_STOMP_GUARD_SIZE;
    alloc->data_offset += PRE_BUFFER_STOMP_GUARD_SIZE;
#endif

    alloc->block_size = block_size;
    alloc->total_size = alloc->total_blocks * alloc->block_size;

    alloc->memory = malloc(alloc->total_size);
    alloc->bitmap = malloc((alloc->total_blocks + 7) / 8); // One bit per block, rounded up
    if (!alloc->memory || !alloc->bitmap) {
        free(alloc->memory);
        free(alloc->bitmap);
        free(alloc);
        return NULL;
    }

    memset(alloc->bitmap, 0, (alloc->total_blocks + 7) / 8); // All blocks free
    return alloc;
}

#if ENABLE_STOMP_DETECT
void check_block_for_stomp(BlockAllocator* alloc, void* ptr) {
    uint32_t* pre_ptr = (uint32_t *)((uint8_t*)ptr - PRE_BUFFER_STOMP_GUARD_SIZE);
    int k;
    for(k = 0; k < (int)(PRE_BUFFER_STOMP_GUARD_SIZE / sizeof(uint32_t)); k++) {
        ASSERT(*pre_ptr++ == pre_stomp_pattern_array[k]);
    }
    uint32_t* post_ptr = (uint32_t*)((uint8_t*)ptr + alloc->block_data_size);
    for(k = 0; k < (int)(POST_BUFFER_STOMP_GUARD_SIZE / sizeof(uint32_t)); k++) {
        ASSERT(*post_ptr++ == post_stomp_pattern_array[k]);
    }
}
#endif

void check_for_stomps(BlockAllocator* alloc) {
    if (!alloc) return;

#if ENABLE_STOMP_DETECT
    size_t i;
    // Free any allocated blocks to ensure we check for stomps
    for (i = 0; i < (alloc->total_blocks + 7) / 8; i++) {
        if (alloc->bitmap[i] != 0x00) { // If byte isn't fully allocated
            for (size_t j = 0; j < 8 && (i * 8 + j) < alloc->total_blocks; j++) {
                size_t index = i * 8 + j;
                if (test_bit(alloc->bitmap, index)) {
                    uint8_t* block = alloc->memory + (index * alloc->block_size);
                    // Move the pointer forward to the start of the data so the free_block
                    // gets the pointer the client would normally hand in.
                    uint8_t* ptr = block + alloc->data_offset;
                    check_block_for_stomp(alloc, ptr);
                }
            }
        }
    }
#endif
}

// Free the allocator
void free_allocator(BlockAllocator* alloc) {
    if (alloc) {
#if ENABLE_STOMP_DETECT
        check_for_stomps(alloc);
#endif
        free(alloc->memory);
        free(alloc->bitmap);
        free(alloc);
    }
}

// Set or clear a bit in the bitmap
void set_bit(uint8_t* bitmap, size_t index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

void clear_bit(uint8_t* bitmap, size_t index) {
    bitmap[index / 8] &= ~(1 << (index % 8));
}

int test_bit(uint8_t* bitmap, size_t index) {
    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

// Allocate a block with debug info
void* alloc_block(BlockAllocator* alloc, const char* file, int line) {
#if ENABLE_DEBUG_HEADER == 0
    (void)file;
    (void)line;
#endif
    if (!alloc) return NULL;

    // Scan bitmap for first free block, checking bytes first
    size_t i;
    for (i = 0; i < (alloc->total_blocks + 7) / 8; i++) {
        if (alloc->bitmap[i] != 0xFF) { // If byte isn't fully allocated
            // Check individual bits in this byte
            for (size_t j = 0; j < 8 && (i * 8 + j) < alloc->total_blocks; j++) {
                size_t index = i * 8 + j;
                if (!test_bit(alloc->bitmap, index)) {
                    set_bit(alloc->bitmap, index);
                    uint8_t* block = alloc->memory + (index * alloc->block_size);

                    // Store debug header if enabled
#if ENABLE_DEBUG_HEADER == 1
                    DebugHeader* header = (DebugHeader*)block;
                    header->file = file;
                    header->line = line;
                    block += sizeof(DebugHeader);
#endif
#if ENABLE_STOMP_DETECT
                    uint32_t* pre_ptr = (uint32_t *)block;
                    int k;
                    for(k = 0; k < (int)(PRE_BUFFER_STOMP_GUARD_SIZE / sizeof(uint32_t)); k++) {
                        *pre_ptr++ = pre_stomp_pattern_array[k];
                    }
                    block += PRE_BUFFER_STOMP_GUARD_SIZE;

                    uint32_t* post_ptr = (uint32_t*)(block + alloc->block_data_size);
                    for(k = 0; k < (int)(POST_BUFFER_STOMP_GUARD_SIZE / sizeof(uint32_t)); k++) {
                        *post_ptr++ = post_stomp_pattern_array[k];
                    }
#endif
                    return (void*)block;
                }
            }
        }
    }
    return NULL; // No free blocks
}

int is_allocated(BlockAllocator* alloc, void* ptr) {
    if (!ptr) return 0;

    ASSERT(alloc != (void *)0);
    ASSERT(ptr >= (void *)alloc->memory);

    // Calculate block index
    size_t offset = (uint8_t*)ptr - alloc->memory;
#if ENABLE_DEBUG_HEADER
    offset -= alloc->data_offset;
#endif
    ASSERT(offset % alloc->block_size == 0); // Ensure valid pointer
    if (offset % alloc->block_size != 0) return 0;

    size_t index = offset / alloc->block_size;
    ASSERT( index < alloc->total_blocks);
    return test_bit(alloc->bitmap, index);
}

// Free a block
void free_block(BlockAllocator* alloc, void* ptr) {
    if (!alloc || !ptr) return;

    // Calculate block index
    ASSERT((uint8_t*)ptr >= alloc->memory); // pointer in range of heap allocation
    if ((uint8_t*)ptr < alloc->memory) return;

    long offset = (uint8_t*)ptr - alloc->memory;
    offset -= (long)alloc->data_offset;

    ASSERT(offset % (long)(alloc->block_size) == 0); // Ensure valid pointer
    if (offset % (long)(alloc->block_size) != 0) return;

#if ENABLE_STOMP_DETECT
    check_block_for_stomp(alloc, ptr);
#endif

    size_t index = offset / alloc->block_size;
    ASSERT(index < alloc->total_blocks);

    if (index < alloc->total_blocks) {
        clear_bit(alloc->bitmap, index);
    }
}

// Optional: Dump allocator state for debugging
#if ENABLE_DEBUG_HEADER
void dump_allocator(BlockAllocator* alloc) {
    if (!alloc) return;
    printf("Allocator state:\n");
    size_t used = 0;
    for (size_t i = 0; i < alloc->total_blocks; i++) {
        if (test_bit(alloc->bitmap, i)) {
            used++;
            DebugHeader* header = (DebugHeader*)(alloc->memory + (i * alloc->block_size));
            printf("Block %zu: Allocated at %s:%d\n", i, header->file, header->line);
        }
    }
    printf("Total blocks: %zu, Used: %zu, Free: %zu\n",
           alloc->total_blocks, used, alloc->total_blocks - used);
}
#endif

void print_block(BlockAllocator* alloc, void* ptr) {
    uint8_t * byte_ptr = (uint8_t *)ptr - alloc->data_offset;
#if ENABLE_DEBUG_HEADER
    DebugHeader* header = (DebugHeader *)byte_ptr;
    printf("Header:\n  File: %s\n  Line: %u", header->file, header->line);
    byte_ptr += sizeof(DebugHeader);
#endif
#if ENABLE_STOMP_DETECT
    printf("\nPre Stomp Region:\n  ");
    size_t s;
    uint32_t* pre_ptr = (uint32_t *)byte_ptr;
    for(s = 0; s < PRE_BUFFER_STOMP_GUARD_SIZE / sizeof(uint32_t); s++) {
        printf("0x%08X ", *pre_ptr++);
    }
    byte_ptr += PRE_BUFFER_STOMP_GUARD_SIZE;
#endif
    size_t i;
    uint8_t* data_ptr = byte_ptr;
    printf("\nData:");
    for(i = 0; i < alloc->block_data_size; i++) {
        if (i%32 == 0) {
            printf("\n  ");
        }
        printf("0x%02X ", *data_ptr++);
    }
    byte_ptr += alloc->block_data_size;

#if ENABLE_STOMP_DETECT
    printf("\nPost Stomp Region:\n  ");
    uint32_t* post_ptr = (uint32_t *)byte_ptr;
    for(s = 0; s < POST_BUFFER_STOMP_GUARD_SIZE / sizeof(uint32_t); s++) {
        printf("0x%08X ", *post_ptr++);
    }
    byte_ptr += PRE_BUFFER_STOMP_GUARD_SIZE;
#endif
    printf("\nBytes printed: %u, block_size=%zu\n", (uint32_t)((uint8_t*)byte_ptr - ((uint8_t*)ptr - alloc->data_offset)), alloc->block_size);
}

