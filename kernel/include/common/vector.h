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
 * @brief Resize multiplier for the dynamic array.
 */
#define RESIZE (4)

/**
 * @brief Gets the length of the vector.
 *
 * @param vec Vector to operate on
 */
#define vector_len(vec)                     ((vec)->length)

/**
 * @brief Gets the element stored at the given index.
 *
 * @param vec Vector to operate on
 * @param index Index of the data to retrieve.
 */
#define vector_at(vec, index)               ((vec)->data[index])

/**
 * @brief Appends an element to the end of the vector, resizing if needed.
 *
 * @param vec Vector to operate on.
 * @param elem Element to append.
 */
#define vector_append(vec, elem)                                            \
    do {                                                                    \
        size_t __new_len = (vec)->length + 1;                               \
        if (__new_len * sizeof(*(vec)->data) > (vec)->capacity) {           \
            (vec)->capacity = __new_len * sizeof(*(vec)->data) * RESIZE;    \
            (vec)->data = krealloc((vec)->data, (vec)->capacity);           \
        }                                                                   \
        (vec)->data[(vec)->length] = (elem);                                \
        (vec)->length = __new_len;                                          \
    } while (0)

/**
 * @brief Erases the element at the specified index.
 *
 * @param vec Vector to operate on.
 * @param index Index of the element to remove.
 */
#define vector_erase(vec, index)                                            \
    do {                                                                    \
        size_t __num = (vec)->length - (index) - 1;                         \
        if (__num > 0) {                                                    \
            memcpy(&((vec)->data[index]), &((vec)->data[(index) + 1]),      \
                   __num * sizeof((vec)->data[0]));                         \
        }                                                                   \
        (vec)->length--;                                                    \
    } while (0)

/**
 * @brief Frees memory associated with the vector.
 *
 * @param vec Vector to operate on.
 */
#define vector_free(vec)                                                    \
    do {                                                                    \
        (vec)->length = 0;                                                  \
        (vec)->capacity = 0;                                                \
        if ((vec)->data != NULL) {                                          \
            kfree((vec)->data);                                             \
        }                                                                   \
        (vec)->data = NULL;                                                 \
    } while (0)

/**
 * @brief Erases the first instance of a value in the vector.
 *
 * @param vec Vector to operate on.
 * @param val Value to remove on first occurrence.
 */
#define vector_erase_value(vec, val)                                        \
    do {                                                                    \
        for (size_t __i = 0; __i < vector_len(vec); __i++) {                \
            if (vector_at(vec, __i) == (val)) {                             \
                vector_erase(vec, __i);                                     \
                break;                                                      \
            }                                                               \
        }                                                                   \
    } while (0)
