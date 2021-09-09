#include "hashmap.h"

// -- Key Type definitions --

void ptr_hashKey(struct hm_key *key) { key->hash = (unsigned long)key->i_key; }

int ptr_isKeyEqual(struct hm_key *a, struct hm_key *b) {
    return a->ptr == b->ptr;
}

void int_hashKey(struct hm_key *key) { key->hash = (unsigned long)key->i_key; }

int int_isKeyEqual(struct hm_key *a, struct hm_key *b) {
    return a->i_key == b->i_key;
}

// djb hash function
void range_djb(struct hm_key *key) {
    range_t range = key->range_key;
    unsigned long hash = 5381;
    int c;

    while (range.size > 0) {
        hash = ((hash << 5) + hash) + *(char *)(range.ptr++);
        range.size--;
    }
    key->hash = hash;
}

int range_isKeyEqual(struct hm_key *a, struct hm_key *b) {
    return range_cmp(a->range_key, b->range_key);
}

struct hm_key_type intKeyType =
    (struct hm_key_type){.isKeyEqual = int_isKeyEqual, .hashKey = int_hashKey};

struct hm_key_type rangeKeyType =
    (struct hm_key_type){.isKeyEqual = range_isKeyEqual, .hashKey = range_djb};

struct hm_key_type ptrKeyType =
    (struct hm_key_type){.isKeyEqual = ptr_isKeyEqual, .hashKey = ptr_hashKey};

void hm_key_init(hashmap_t *hm, struct hm_key *key) {
    hm->keyType.hashKey(key);
}

int hm_key_compare(hashmap_t *hm, struct hm_key *a, struct hm_key *b) {
    return hm->keyType.isKeyEqual(a, b);
}

void hashmap_inits(hashmap_t *hm, struct hm_key_type kType,
                   size_t bucketCount) {
    hm->buckets = dzmalloc(sizeof(struct list_head) * bucketCount);
    hm->bucketCount = bucketCount;
    hm->size = 0;
    hm->keyType = kType;
}

void hashmap_init(hashmap_t *hm, struct hm_key_type kType) {
    hashmap_inits(hm, kType, HM_INITIAL_BUCKET_SIZE);
}

struct list_head *_hashmap_locateBucket(hashmap_t *hm, struct hm_key *key) {
    size_t offset = key->hash % hm->bucketCount;
    return hm->buckets + offset;
}

struct hm_bucket_entry *_hashmap_findInBucket(hashmap_t *hm,
                                              struct list_head *bucket,
                                              struct hm_key *key) {
    if (bucket->next == NULL)
        return NULL;

    LIST_FOR_EACH(bucket) {
        struct hm_bucket_entry *entry =
            containerof(c, struct hm_bucket_entry, colisions);
        if (hm_key_compare(hm, key, &entry->key))
            return entry;
    }
    return NULL;
}

void hashmap_rehash(hashmap_t *hm) {
    int oldCount = hm->bucketCount;
    hm->bucketCount *= 2;
    struct list_head *old = hm->buckets;
    hm->buckets = dzmalloc(sizeof(struct list_head) * hm->bucketCount);

    for (int i = 0; i < oldCount; i++) {
        struct list_head *head = &old[i];
        if (head->next == NULL)
            continue;
        for (struct list_head *c = head->next, *next = c->next; c != head;
             c = next) {
            next = c->next;
            struct hm_bucket_entry *entry =
                containerof(c, struct hm_bucket_entry, colisions);
            struct list_head *newBucket =
                _hashmap_locateBucket(hm, &entry->key);
            if (newBucket->next == NULL)
                LIST_INIT(newBucket);
            list_add(newBucket, c);
        }
    }
    free(old);
}

void hashmap_set(hashmap_t *hm, struct hm_key *key,
                 struct hm_bucket_entry *entry) {
    struct list_head *bucket = _hashmap_locateBucket(hm, key);
    struct hm_bucket_entry *foundEntry = _hashmap_findInBucket(hm, bucket, key);

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

struct hm_bucket_entry *hashmap_get(hashmap_t *hm, struct hm_key *key) {
    struct list_head *bucket = _hashmap_locateBucket(hm, key);
    return _hashmap_findInBucket(hm, bucket, key);
}

void hashmap_remove(hashmap_t *hm, struct hm_key *key) {
    struct list_head *bucket = _hashmap_locateBucket(hm, key);
    struct hm_bucket_entry *foundEntry = _hashmap_findInBucket(hm, bucket, key);

    if (foundEntry) { // if there is a entry already, remove it.
        list_deattach(&foundEntry->colisions);
        hm->size--;
    }
}

void hashmap_setRange(hashmap_t *hm, range_t range,
                      struct hm_bucket_entry *entry) {
    // assert(hm->keyType == rangeKeyType && "Wrong key type !!");

    struct hm_key key;
    key.range_key = range;
    hm_key_init(hm, &key);
    hashmap_set(hm, &key, entry);
}

struct hm_bucket_entry *hashmap_getRange(hashmap_t *hm, range_t range) {
    // assert(hm->keyType == rangeKeyType && "Wrong key type !!");

    struct hm_key key;
    key.range_key = range;
    hm_key_init(hm, &key);
    return hashmap_get(hm, &key);
}

void hashmap_removeRange(hashmap_t *hm, range_t range) {
    struct hm_key key;
    key.range_key = range;
    hm_key_init(hm, &key);
    hashmap_remove(hm, &key);
}

struct hm_bucket_entry *hashmap_getInt(hashmap_t *hm, int ikey) {
    // assert(hm->keyType == intKeyType && "Wrong key type !!");
    struct hm_key key;
    key.i_key = ikey;
    hm_key_init(hm, &key);
    return hashmap_get(hm, &key);
}

void hashmap_setInt(hashmap_t *hm, int ikey, struct hm_bucket_entry *entry) {
    // assert(hm->keyType == intKeyType && "Wrong key type !!");
    struct hm_key key;
    key.i_key = ikey;
    hm_key_init(hm, &key);
    hashmap_set(hm, &key, entry);
}

void hashmap_removeInt(hashmap_t *hm, int ikey) {
    struct hm_key key;
    key.i_key = ikey;
    hm_key_init(hm, &key);
    hashmap_remove(hm, &key);
}

struct hm_bucket_entry *hashmap_getPtr(hashmap_t *hm, void *ptr) {
    // assert(hm->keyType == intKeyType && "Wrong key type !!");
    struct hm_key key;
    key.ptr = ptr;
    hm_key_init(hm, &key);
    return hashmap_get(hm, &key);
}

void hashmap_setPtr(hashmap_t *hm, void *ptr, struct hm_bucket_entry *entry) {
    // assert(hm->keyType == ptrKeyType && "Wrong key type !!");

    struct hm_key key;
    key.ptr = ptr;
    hm_key_init(hm, &key);
    hashmap_set(hm, &key, entry);
}

void hashmap_removePtr(hashmap_t *hm, void *ptr) {
    struct hm_key key;
    key.ptr = ptr;
    hm_key_init(hm, &key);
    hashmap_remove(hm, &key);
}

void hashmap_free(hashmap_t *hm) { free(hm->buckets); }

struct hashmap_it hashmap_it_init(hashmap_t *hm) {
    struct hashmap_it result;
    result.hm = hm;
    result.bucket = 0;
    result.bucketPos = hm->buckets[0].next;
    return result;
}

int _hashmap_it_bucketEnd(struct hashmap_it *it) {
    if (it->bucket >= it->hm->bucketCount)
        return 1;
    // the circular linked list gets lazily initialized.
    // if the ->next is null that means it is not initialized.
    // therefore null.
    return !it->bucketPos->next ||
           it->bucketPos == &it->hm->buckets[it->bucket];
}

// Find a non empty bucket.
void _hashmap_it_findNonEmptyBucket(struct hashmap_it *it) {
    // Go over the buckets until we found a non empty one.
    it->bucket++;
    while (it->bucket < it->hm->bucketCount) {
        struct list_head *list = &it->hm->buckets[it->bucket];
        it->bucketPos = list->next;

        if (it->bucketPos && !_hashmap_it_bucketEnd(it))
            return;
        it->bucket++;
    }
}

void hashmap_it_next(struct hashmap_it *it) {
    assert(!hashmap_it_end(it) && "invalid iterator");

    // if this is the last element, try to find a bucket that isn't empty.
    if (_hashmap_it_bucketEnd(it))
        _hashmap_it_findNonEmptyBucket(it);
    else
        it->bucketPos = it->bucketPos->next;
}

int hashmap_it_end(struct hashmap_it *it) {
    if (_hashmap_it_bucketEnd(it))
        _hashmap_it_findNonEmptyBucket(it);

    return _hashmap_it_bucketEnd(it);
}

struct hm_bucket_entry *hashmap_it_get(struct hashmap_it it) {
    assert(!_hashmap_it_bucketEnd(&it) && "Invalid iterator state");
    return containerof(it.bucketPos, struct hm_bucket_entry, colisions);
}

// -- Hash set --
void hashset_init(hashset_t *hs, struct hm_key_type kType) {
    hashmap_init(&hs->hashmap, kType);
    zone_init(&hs->zone);
}

int hashset_insertPtr(hashset_t *hs, void *ptr) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&hs->hashmap, ptr);
    if (entry != NULL)
        return 0;

    entry = znnew(&hs->zone, struct hm_bucket_entry);
    hashmap_setPtr(&hs->hashmap, ptr, entry);
    return 1;
}

int hashset_existsPtr(hashset_t *hs, void *ptr) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&hs->hashmap, ptr);
    return entry != NULL;
}

int hashset_insertInt(hashset_t *hs, int ikey) {
    struct hm_bucket_entry *entry = hashmap_getInt(&hs->hashmap, ikey);
    if (entry != NULL)
        return 0;

    entry = znnew(&hs->zone, struct hm_bucket_entry);
    hashmap_setInt(&hs->hashmap, ikey, entry);
    return 1;
}

int hashset_existsInt(hashset_t *hs, int ikey) {
    struct hm_bucket_entry *entry = hashmap_getInt(&hs->hashmap, ikey);
    return entry != NULL;
}

void hashset_remove(hashset_t *hs, struct hm_key *key) {
    hashmap_remove(&hs->hashmap, key);
}

void hashset_free(hashset_t *hs) {
    zone_free(&hs->zone);
    hashmap_free(&hs->hashmap);
}
