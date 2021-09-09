#include "zone_alloc.h"

int main(int argc, char *args[]) {

    zone_allocator allocator;
    zone_init(&allocator);

    for (int i = 0; i < 50; i++) {
        zone_alloc(&allocator, 100);
    }

    zone_free(&allocator);

    return 0;
}
