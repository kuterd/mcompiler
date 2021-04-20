#include <stdio.h>
#include <math.h>

#include "hashmap.h"
#include "utils.h"

struct test_struct {
    struct hm_bucket_entry bucket;  
    int id;
};

void key_insert(hashmap_t *hm, int i) {
    struct test_struct *ts = malloc(sizeof(struct test_struct));
    ts->id = i;
    struct hm_key a;
    hm_key_initInt(&a, i);
    hashmap_set(hm, &a, &ts->bucket);
}

void key_check(hashmap_t *hm, int i) {
    struct hm_key a;
    hm_key_initInt(&a, i);
    struct hm_bucket_entry *bentry = hashmap_get(hm, &a);
    struct test_struct *b = containerof(bentry, struct test_struct, bucket);
    assert(b->id == i && "wrong entry");
}

int main() {
    hashmap_t hm;
    hashmap_init(&hm);
    
    for (int i = 0; i < 1000; i++) {
        key_insert(&hm, i);
    }

    for (int i = 0; i < 1000; i++) {
        key_check(&hm, i);
    }

}
