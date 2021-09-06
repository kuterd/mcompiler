#include <stdio.h>
#include <math.h>

#include "hashmap.h"
#include "utils.h"

zone_allocator zone;

struct test_struct {
    struct hm_bucket_entry bucket;  
    int id;
};

void key_insert(hashmap_t *hm, int i) {
    struct test_struct *ts = znnew(&zone, struct test_struct);
    ts->id = i;
    hashmap_setInt(hm, i, &ts->bucket);
}

void key_check(hashmap_t *hm, int i) {
    struct hm_bucket_entry *bentry = hashmap_getInt(hm, i);
    struct test_struct *b = containerof(bentry, struct test_struct, bucket);
    assert(b->id == i && "wrong entry");
}

void hashmap_test() {
    zone_init(&zone);
    hashmap_t hm;
    hashmap_init(&hm, intKeyType);
    
    for (int i = 0; i < 1000; i++) {
        key_insert(&hm, i);
    }

    for (int i = 0; i < 1000; i++) {
        key_check(&hm, i);
    }

    for (struct hashmap_it it = hashmap_it_init(&hm); !hashmap_it_end(&it); hashmap_it_next(&it)) {
        struct hm_bucket_entry *entry = hashmap_it_get(it);
        struct test_struct *tstruct = containerof(entry, struct test_struct, bucket);
        printf("%d\n", tstruct->id);
    } 

    zone_free(&zone);
    hashmap_free(&hm);
}

void hashset_test() {
    hashset_t hs;
    hashset_init(&hs, intKeyType);
    
    for (int i = 0; i < 1000; i++) {
        assert(hashset_insertInt(&hs, i));
    }

    for (int i = 0; i < 1000; i++) {
        assert(hashset_existsInt(&hs, i));
    }
    hashset_free(&hs);
}

int main() {
    hashmap_test();
    hashset_test();

}
