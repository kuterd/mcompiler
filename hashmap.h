// Kuter Dinel 2021
// Intrusive hash map based on linked lists. This interface can also be used as
// a unordered set.

#ifndef HASHMAP_H
#define HASHMAP_H

#include "buffer.h"
#include "list.h"
#include "zone_alloc.h"

#define HM_INITIAL_BUCKET_SIZE 100

// Default key types.
extern struct hm_key_type intKeyType;
extern struct hm_key_type rangeKeyType;
extern struct hm_key_type ptrKeyType;

struct hm_key {
    union {
        int i_key;
        range_t range_key;

        // Depending on the key type ptr could either be extended key info
        // or the key itself(ptr key).
        void *ptr;
    };
    unsigned long hash;
};

// hashset abstract key type.
struct hm_key_type {
    int (*isKeyEqual)(struct hm_key *a, struct hm_key *b);
    void (*hashKey)(struct hm_key *key);
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
struct hm_bucket_pointer {
    struct hm_bucket_entry entry;
    void *pointer;
};

typedef struct {
    // A array of linked lists.
    struct list_head *buckets;
    size_t bucketCount;
    size_t size;

    struct hm_key_type keyType;
} hashmap_t;

typedef struct {
    hashmap_t hashmap;
    zone_allocator zone;
} hashset_t;

// Initialize hashet key.
void hm_key_init(hashmap_t *hm, struct hm_key *key);

// Check if hashset keys are equal.
int hm_key_compare(hashmap_t *hm, struct hm_key *a, struct hm_key *b);

// Init the hashmap, specifying the initial table size.
void hashmap_inits(hashmap_t *hm, struct hm_key_type kType, size_t bucketCount);

// Init the hashmap.
void hashmap_init(hashmap_t *hm, struct hm_key_type kType);

// Rehash the hashmap.
void hashmap_rehash(hashmap_t *hm);

// Free to hashmap.
void hashmap_free(hashmap_t *hm);

// Set to hashmap.
void hashmap_set(hashmap_t *hm, struct hm_key *key,
                 struct hm_bucket_entry *entry);

// Get from hashmap.
struct hm_bucket_entry *hashmap_get(hashmap_t *hm, struct hm_key *key);

// Remove from hashmap.
void hashmap_remove(hashmap_t *hm, struct hm_key *key);

// -- Convinience methods --

// Set range to hashset.
void hashmap_setRange(hashmap_t *hm, range_t range,
                      struct hm_bucket_entry *entry);

// Get range from hashset.
struct hm_bucket_entry *hashmap_getRange(hashmap_t *hm, range_t range);

// Remove range from hashset.
void hashmap_removeRange(hashmap_t *hm, range_t range);

// Set int to hashset.
void hashmap_setInt(hashmap_t *hm, int ikey, struct hm_bucket_entry *entry);

// Get int from hashmap.
struct hm_bucket_entry *hashmap_getInt(hashmap_t *hm, int ikey);

// Remove int from hashset.
void hashmap_removeInt(hashmap_t *hm, int ikey);

// Set pointer to hashmap.
void hashmap_setPtr(hashmap_t *hm, void *key, struct hm_bucket_entry *entry);

// Get pointer from the hashmap.
struct hm_bucket_entry *hashmap_getPtr(hashmap_t *hm, void *key);

// Remove ptr from hashset.
void hashmap_removeInt(hashmap_t *hm, int ikey);

// Iterator for hashmap/hashset. Not very fast for sparse hashsets.
// Consider using a linked list or dbuffer instead.

struct hashmap_it {
    // hashmap this it is associated with.
    hashmap_t *hm;
    // Index of the current bucket.
    size_t bucket;
    // Position inside the bucket.
    struct list_head *bucketPos;
};

struct hashmap_it hashmap_it_init(hashmap_t *hm);
void hashmap_it_next(struct hashmap_it *it);

// Return 1 if this is the end of the iterator.
int hashmap_it_end(struct hashmap_it *it);
struct hm_bucket_entry *hashmap_it_get(struct hashmap_it it);

// -- Hash set --

// Initialize the hashset.
void hashset_init(hashset_t *hs, struct hm_key_type kType);

// free the hashset
void hashset_free(hashset_t *hs);

// -- hashset convineience methods --

// Insert pointer to hashset.
int hashset_insertPtr(hashset_t *hs, void *ptr);

// Check if pointer exists in hashset.
int hashset_existsPtr(hashset_t *hs, void *ptr);

// Insert in to hashset
int hashset_insertInt(hashset_t *hs, int ikey);

// Check if int exists in hashset
int hashset_existsInt(hashset_t *hs, int ikey);

#endif
