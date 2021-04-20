// Kuter Dinel 2021
// Intrusive hash map based on linked lists.

#ifndef HASHMAP_H 
#define HASHMAP_H 

#include "buffer.h"
#include "list.h"

#define HM_INITIAL_BUCKET_SIZE 100

enum hm_key_type {
    EMPTY = 0,
    INT,
    RANGE
};

struct hm_key {
    union {
        int i_key;
        range_t range_key;
    };
    unsigned long hash;
    enum hm_key_type type;
};

// The intrusive entry part
struct hm_bucket_entry {
    struct hm_key key;
    struct list_head colisions; 
    void *entry;
};

// bucket entry implementation for pointer types.
// don't use this unless you have to.
// embeding a hm_bucket_entry to the struct is better.
struct m_bucket_pointer {
    struct hm_bucket_entry entry;
    void *pointer;
};

typedef struct {
    // A array of linked lists.
    struct list_head *buckets; 
    size_t bucketCount;
    size_t size;
} hashmap_t;

// djb hash function
unsigned long djb(range_t range) {
    unsigned long hash = 5381;
    int c;
    
    while (range.size > 0) {
        hash = ((hash << 5) + hash) + *(char*)(range.ptr++);
        range.size--;
    }
    return hash;
}

// if the key contains a reference to memory location it is expected to last the
// entire lifetime of the hm_key.
void hm_key_init(struct hm_key *key) {
   switch(key->type) {
    case INT:
        key->hash = (unsigned long)key->i_key;
        break;
    case RANGE:
        key->hash = djb(key->range_key);
        break;
   } 
}

// string is expected to last the entire lifetime of the hm_key
void hm_key_initString(struct hm_key *key, char *string) {
    key->range_key = range_fromString(string);
    key->type = RANGE;
    hm_key_init(key);
}

void hm_key_initInt(struct hm_key *key, int i) {
    key->i_key = i;
    key->type = INT;
    hm_key_init(key);
}

int hm_key_compare(struct hm_key *a, struct hm_key *b) {
    assert(a->type == b->type && "Key types must be equal !!");
    assert(a->hash == b->hash && "Hashes must be equal !!"); 
    switch(a->type) {
    case INT:
        return a->i_key == b->i_key;
    case RANGE:
        return range_cmp(a->range_key, b->range_key); 
   }    
}

void hashmap_inits(hashmap_t *hm, size_t bucketCount) {
    hm->buckets = dzmalloc(sizeof(struct list_head) * bucketCount);  
    hm->bucketCount = bucketCount;
    hm->size = 0;
}

void hashmap_init(hashmap_t *hm) {
    hashmap_inits(hm, HM_INITIAL_BUCKET_SIZE); 
}

struct list_head* _hashmap_locateBucket(hashmap_t *hm, struct hm_key *key) {
    size_t offset = key->hash % hm->bucketCount; 
    return hm->buckets + offset;
}

struct hm_bucket_entry* _hashmap_findInBucket(struct list_head *bucket, struct hm_key *key) {
    if (bucket->next == NULL)
        return NULL;

    LIST_FOR_EACH(bucket) {
        struct hm_bucket_entry *entry = containerof(c, struct hm_bucket_entry, colisions);
        if (hm_key_compare(key, &entry->key))
            return entry;            
    }
    return NULL;
}

void hashmap_rehash(hashmap_t *hm) {
    puts("Rehashing !!!!!!!"); 
    int oldCount = hm->bucketCount; 
    hm->bucketCount *= 2;
    struct list_head *old = hm->buckets;
    hm->buckets = dzmalloc(sizeof(struct list_head) * hm->bucketCount);  

    for (int i = 0; i < oldCount; i++) {
        struct list_head *head = &old[i]; 
        if (head->next == NULL)
            continue;
        for (struct list_head *c = head->next, *next = c->next; c != head; c = next) {
            next = c->next;
            struct hm_bucket_entry *entry = containerof(c, struct hm_bucket_entry, colisions);
            struct list_head *newBucket = _hashmap_locateBucket(hm, &entry->key); 
            if (newBucket->next == NULL)
                LIST_INIT(newBucket);
            list_add(newBucket, c); 
        } 
    }
}

void hashmap_set(hashmap_t *hm, struct hm_key *key, struct hm_bucket_entry *entry) {
    struct list_head *bucket = _hashmap_locateBucket(hm, key);  
    struct hm_bucket_entry *foundEntry = _hashmap_findInBucket(bucket, key);
    
    if (foundEntry) { // if there is a entry already, remove it.
        list_deattach(&foundEntry->colisions);
        hm->size--;        
    }
    entry->key = *key;
    if (bucket->next == NULL)
        LIST_INIT(bucket);
    list_add(bucket, &entry->colisions);
    hm->size++;
    
    // Do we need rehashing ?
    if ((hm->size * 100) / (hm->bucketCount) < 80) 
        return;
    
    hashmap_rehash(hm);
} 

struct hm_bucket_entry* hashmap_get(hashmap_t *hm, struct hm_key *key) {
    struct list_head *bucket = _hashmap_locateBucket(hm, key);  
    return _hashmap_findInBucket(bucket, key);
}

void hashmap_setRange(hashmap_t *hm, range_t range, struct hm_bucket_entry *entry) {
    struct hm_key key;
    key.range_key = range;
    key.type = RANGE;
    hashmap_set(hm, &key, entry); 
}

struct hm_bucket_entry* hashmap_getRange(hashmap_t *hm, range_t range) {
    struct hm_key key;
    key.range_key = range;
    key.type = RANGE;
    return hashmap_get(hm, &key);
}


#endif
