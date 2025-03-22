/**
 * @file hash.c
 * @author Zack Bostock
 * @brief General functionality related to hashing
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <globals.h>

#include <common/hash.h>
#include <common/kmalloc.h>
#include <common/kprint.h>

#define HASHCODE(hash, key)         (key % hash->size)
#define HASH_CHECK_RET(hash)        {if (!hash) return;}
#define HASH_CHECK_RET_VAL(hash)    {if (!hash) return SYS_ERR;}
#define HASH_CHECK_RET_NULL(hash)   {if (!hash) return NULL;}
#define DEFAULT_SIZE                (128)

/**
 * @brief Core initialization function for a hash set
 *
 * @param h Hash set to be initialized
 * @param size Number of hashes for it to be initialized to
 */
void hash_init_core(HASH *h, size_t size) {
    h->size = size;
    h->entries = (HASH_ENTRY *) (kmalloc(h->size * sizeof(HASH_ENTRY)));
    for (size_t i = 0; i < h->size; i++) {
        h->entries[i].key = -1;
        h->entries[i].data = NULL;
    }
}

/**
 * @brief Default initialization function
 *
 * @param h Hash structure to intialize
 */
void hash_init(HASH* h) {
    HASH_CHECK_RET(h);
    hash_init_core(h, DEFAULT_SIZE);
}

/**
 * @brief Searches for a specific key in a HASH structure
 *
 * @param h Hash set to search in
 * @param key Key to search for
 * @return void* Pointer to the value associated with the key
 */
void *hash_search(HASH *h, int64_t key) {
    HASH_CHECK_RET_NULL(h);
    uint64_t loop_count = 0;

    /* Use the macro to get the hash */
    int64_t i = HASHCODE(h, key);

    /* Move in array until empty */
    while (h->entries[i].key != EMPTY_KEY && h->entries[i].data) {
        if (h->entries[i].key == key) {
            return h->entries[i].data;
        }

        /* Move to the next cell */
        i++;

        /* Wrap around the table */
        i %= h->size;

        /* if the table has been looped twice, break */
        if (++loop_count >= (h->size * 2)) {
            break;
        }
    }

    /* Key was not found */
    return NULL;
}

/**
 * @brief Insertion function into the hash set
 *
 * @param h Hash set to be inserted into
 * @param key Key to be inserted
 * @param data Data related to the key
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS hash_insert(HASH *h, int64_t key, void *data) {
    HASH_CHECK_RET_VAL(h);
    uint64_t loop_count = 0;

    /* Get the hash */
    int64_t i = HASHCODE(h, key);

    /* Move through the entries until an empty or deleted cell */
    while (h->entries[i].key != EMPTY_KEY && h->entries[i].data) {
        /* Go to the next cell */
        i++;

        /* Wrap around the table */
        i %= h->size;

        /* At most loop the table twice */
        if (++loop_count >= (h->size * 2)) {
            /* Increase the size of the set */
            if (double_buffer((void **) &(h->entries), &(h->size), sizeof(HASH_ENTRY)) == SYS_ERR) {
                kloge("Failed to double the size of hash set!\n");
                return SYS_ERR;
            } else {
                /* Recurse on the hash set to insert now with the doubled size */
                return hash_insert(h, key, data);
            }
        }
    }

    /* Insert the key and data */
    h->entries[i].key = key;
    h->entries[i].data = data;

    return SYS_OK;
}

/**
 * @brief Helper function for removing elements out of the hash set
 *
 * @param h Hash set to remove from
 * @param key Key to remove
 * @return void* Value associated with the key that was just removed
 */
void *hash_delete(HASH *h, int64_t key) {
    HASH_CHECK_RET_NULL(h);
    uint64_t loop_count = 0;

    /* Get the hash */
    int64_t i = HASHCODE(h, key);

    /* Search */
    while (h->entries[i].key != EMPTY_KEY && h->entries[i].data) {
        if (h->entries[i].key == key) {
            void *temp = h->entries[i].data;
            /* Assign dummy element here */
            h->entries[i].key = -1;
            h->entries[i].data = NULL;
            return temp;
        }

        i++;
        i %= h->size;

        /* If we've looped through twice, assume element count not be found */
        if (++loop_count >= (h->size * 2)) {
            return NULL;
        }
    }

    return NULL;
}