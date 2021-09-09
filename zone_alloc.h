#ifndef ZONE_ALLOC_H
#define ZONE_ALLOC_H

#include <stddef.h> // size_t

#define ZONE_SIZE 4096

// FIXME: Currently we cannot handle allocations bigger than 4kb - sizeof(struct
// zone_header) in size.
#define MAX_ZONE_ALLOCATION (ZONE_SIZE - sizeof(struct zone_header))

#define znnew(zone, type) zone_alloc((zone), sizeof(type))

// Placed in front of a allocation.
// the actual buffer starts after this.
// TODO: make this internal.
struct zone_header {
    // No need to use the list_head here.
    struct zone_header *next;
};

// This interface is used to manage allocations.
// Rather than freeing each object individually we can free all allocations at
// once. This simplify the memory management and also makes it faster for small
// allocations.
//
// allocations are done with 4kb sized blocks that are freed all at once.
typedef struct {
    struct zone_header *topZone;
    void *ptr;
    // remaining bytes.
    size_t remaining;
} zone_allocator;

// Initialize a zone allocator.
void zone_init(zone_allocator *alloc);

// Allocate memory from the zone.
void *zone_alloc(zone_allocator *alloc, size_t size);
// Free the entire zone.
void zone_free(zone_allocator *alloc);
#endif
