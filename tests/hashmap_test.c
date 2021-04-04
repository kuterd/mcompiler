#include <stdio.h>
#include <math.h>

#include "hashmap.h"
#include "utils.h"

struct test_struct {
    struct hm_bucket_entry bucket;  
    int id;
};

void check(hashmap_t *hm, char *asd) {
    struct test_struct *ts = malloc(sizeof(struct test_struct));

    struct hm_key a;
    hm_key_initString(&a, asd);
    hashmap_set(hm, &a, &ts->bucket);

    struct hm_key b;
    hm_key_initString(&b, asd);
    struct hm_bucket_entry *entry = hashmap_get(hm, &b); 
    
    assert(entry == &ts->bucket && "Wrong entry returned");
}

int main() {
    hashmap_t hm;
    hashmap_init(&hm);

    check(&hm, "asd");
    check(&hm, "adasd");
    check(&hm, "asd345345");
    check(&hm, "asd");
}
