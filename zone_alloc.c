#include "zone_alloc.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>

void _new_zone(zone_allocator *alloc) {
    // TODO: keep track of wasted bytes.
    alloc->remaining = MAX_ZONE_ALLOCATION;

    void *ptr = dmalloc(ZONE_SIZE);
    struct zone_header *hdr = (struct zone_header *)ptr;
    alloc->ptr = ptr + sizeof(struct zone_header);
    hdr->next = alloc->topZone;
    alloc->topZone = hdr;
}

void zone_init(zone_allocator *alloc) {
    *alloc = (zone_allocator){};
    _new_zone(alloc);
}

void *zone_alloc(zone_allocator *alloc, size_t size) {
    assert(size <= MAX_ZONE_ALLOCATION && "Allocation size bigger than max");

    // Allocate some memory if we don't have enough.
    // this wastes some bytes.
    if (size > alloc->remaining)
        _new_zone(alloc);

    void *result = alloc->ptr;
    alloc->ptr += size;
    alloc->remaining -= size;
    return result;
}

void zone_free(zone_allocator *alloc) {
    // Free all zones, while being carefull not to access freed memory.
    for (struct zone_header *current = alloc->topZone, *next; current != NULL;
         current = next) {
        next = current->next;
        free(current);
    }
}
