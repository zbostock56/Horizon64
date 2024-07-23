/**
 * @file vector.h
 * @author Zack Bostock
 * @brief Dynamically sized array (DSA)
 * @verbatim
 * This data is related to a DSA which can grow and shrink
 * with different inputs. This handles resizing the the buffer
 * which holds up the DSA in memory.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#include <common/string.h>
#include <common/kmalloc.h>
#include <common/memory.h>

#define vector_struct(type)                                                 \
    struct {                                                                \
        size_t length;                                                      \
        size_t capacity;                                                    \
        type *data;                                                         \
    }

#define vector_extern(type, name)           extern vector_struct(type) name
#define vector_new(type, name)              vector_struct(type) name = {0}
#define vector_new_static(type, name)       static vector_new(type, name)

/**
 * @brief Amount to multiply the size of the DSA by in case of reaching
 *        its capacity
 */
#define RESIZE (4)

/**
 * @brief Gets the length of the DSA
 *
 * @param vec Vector to operate on
 */
#define vector_len(vec)                     (vec)->length

/**
 * @brief Gets the element stored at index
 *
 * @param vec Vector to operate on
 * @param index Index of data to get
 */
#define vector_at(vec, index)               (vec)->data[index]

/**
 * @brief Appends an element at the back of the DSA, resizes if needed
 *
 * @param vec Vector to operate on
 * @param elem Element to append on end of vector
 */
#define vector_append(vec, elem) {                                          \
    (vec)->length++;                                                        \
    if ((vec)->capacity < ((vec)->length * sizeof(elem))) {                 \
        (vec)->capacity = (vec)->length * sizeof(elem) * RESIZE;            \
        (vec)->data = krealloc((vec)->data, (vec)->capacity);               \
    }                                                                       \
    (vec)->data[(vec)->length - 1] = elem;                                  \
}

/**
 * @brief Erases the element specified
 *
 * @param vec Vector to operate on
 * @param index Index to erase
 */
#define vector_erase(vec, index) {                                          \
    memcpy(&((vec)->data[index]), &((vec)->data[index + 1]),                \
            sizeof((vec)->data[0]) * (vec)->length - index - 1);            \
    (vec)->length--;                                                        \
}

/**
 * @brief Frees memory associated with DSA
 *
 * @param vec Vector to operate on
 */
#define vector_free(vec) {                                                  \
    (vec)->len = 0;                                                         \
    (vec)->capacity = 0;                                                    \
    if ((vec)->data != NULL) {                                              \
        kfree((vec)->data);                                                 \
    }                                                                       \
    (vec)->data = NULL;                                                     \
}

/**
 * @brief Erases the first instance of val in vec
 *
 * @param vec Vector to operate on
 * @param val Value to remove on first sighting
 */
#define vector_erase_value(vec, val) {                                      \
    for (size_t __i = 0; __i < vector_len(vec); __i++) {                    \
        if (vector_at(vec, __i) == (val)) {                                 \
            vector_erase(vec, __i);                                         \
            break;                                                          \
        }                                                                   \
    }                                                                       \
}
