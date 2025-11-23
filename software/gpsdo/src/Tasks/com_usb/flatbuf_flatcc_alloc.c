/*
 * flatbuf_flatcc_alloc.c
 *
 *  Created on: Nov 23, 2025
 *      Author: andia
 */
#include "flatbuf_flatcc_alloc.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * IMPORTANT:
 * Make sure FLATCC_PORTABLE_ALLOC is defined for this TU
 * so FlatCC uses these functions instead of malloc/free.
 */
#define FLATCC_PORTABLE_ALLOC
#include "flatcc_builder.h"

// ----- Simple arena type -----

typedef struct {
    uint8_t *buf;
    size_t   capacity;
    size_t   offset;
} fb_arena_t;

// ----- Two arenas: one for Status, one for KF debug -----

#define FB_STATUS_ARENA_SIZE  2048      // plenty for Status
#define FB_KF_ARENA_SIZE      16384     // as requested

static uint8_t fb_status_mem[FB_STATUS_ARENA_SIZE];
static uint8_t fb_kf_mem[FB_KF_ARENA_SIZE];

static fb_arena_t fb_status_arena = {
    .buf      = fb_status_mem,
    .capacity = FB_STATUS_ARENA_SIZE,
    .offset   = 0
};

static fb_arena_t fb_kf_arena = {
    .buf      = fb_kf_mem,
    .capacity = FB_KF_ARENA_SIZE,
    .offset   = 0
};

/*
 * Pointer to the arena currently used by FlatCC.
 * We switch this from the outside before building a message.
 */
static fb_arena_t *fb_current_arena = NULL;

void flatbuf_select_status_arena(void)
{
    fb_status_arena.offset = 0;    // reset for fresh build
    fb_current_arena = &fb_status_arena;
}

void flatbuf_select_kf_arena(void)
{
    fb_kf_arena.offset = 0;        // reset for fresh build
    fb_current_arena = &fb_kf_arena;
}

// ----- Portable allocator for FlatCC -----

// Align allocations to 8 bytes for safety
static size_t fb_align(size_t n)
{
    const size_t a = 8;
    return (n + (a - 1)) & ~(a - 1);
}

void *flatcc_portable_malloc(size_t size)
{
    if (!fb_current_arena) {
        // No arena selected -> hard fail (Option 1 behavior)
        return NULL;
    }

    size = fb_align(size);

    if (fb_current_arena->offset + size > fb_current_arena->capacity) {
        // Arena too small -> fail (caller ends up in assert/abort)
        return NULL;
    }

    void *p = fb_current_arena->buf + fb_current_arena->offset;
    fb_current_arena->offset += size;
    return p;
}

/*
 * Very simple realloc: always allocate a new block from the arena.
 * FlatCC usually does not rely on preserving old data in realloc in
 * a way that would break correctness here.
 */
void *flatcc_portable_realloc(void *ptr, size_t size)
{
    (void)ptr;
    return flatcc_portable_malloc(size);
}

/*
 * No-op free: arena memory is reused when we reset offset in
 * flatbuf_select_*_arena().
 */
void flatcc_portable_free(void *ptr)
{
    (void)ptr;
}
