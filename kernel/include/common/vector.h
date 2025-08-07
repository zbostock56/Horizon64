/**
 * @file vector.h
 * @author Zack Bostock
 * @brief Type-safe dynamically sized array implementation
 * @verbatim
 * This provides a generic, type-safe dynamic array implementation with
 * automatic memory management, bounds checking, and comprehensive error handling.
 * The vector automatically grows and shrinks as needed, with configurable
 * growth strategies for optimal performance.
 * @endverbatim
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <common/kmalloc.h>
#include <string.h>

/* Vector configuration constants */
#define VECTOR_DEFAULT_CAPACITY     8
#define VECTOR_GROWTH_FACTOR        2
#define VECTOR_SHRINK_THRESHOLD     4
#define VECTOR_MAX_CAPACITY         (SIZE_MAX / 2)

/* Vector error codes */
typedef enum {
    VECTOR_SUCCESS = 0,
    VECTOR_ERROR_NULL_POINTER = -1,
    VECTOR_ERROR_OUT_OF_BOUNDS = -2,
    VECTOR_ERROR_OUT_OF_MEMORY = -3,
    VECTOR_ERROR_OVERFLOW = -4,
    VECTOR_ERROR_INVALID_CAPACITY = -5
} VECTOR_ERROR;

/**
 * @brief Generic vector structure
 * @param type Type of elements stored in the vector
 */
#define vector_struct(type)                                                 \
    struct {                                                                \
        size_t length;          /* Current number of elements */           \
        size_t capacity;        /* Current allocated capacity */           \
        type *data;             /* Pointer to data array */                \
        bool initialized;       /* Initialization flag */                  \
    }

#define vector_extern(type, name)           extern vector_struct(type) name
#define vector_new(type, name)              vector_struct(type) name = {0, 0, NULL, false}
#define vector_new_static(type, name)       static vector_new(type, name)

/**
 * @brief Initialize a vector with default capacity
 * @param vec Pointer to vector
 * @return VECTOR_ERROR Success or error code
 */
#define vector_init(vec)                                                    \
    vector_init_with_capacity(vec, VECTOR_DEFAULT_CAPACITY)

/**
 * @brief Initialize a vector with specified capacity
 * @param vec Pointer to vector
 * @param cap Initial capacity
 * @return VECTOR_ERROR Success or error code
 */
#define vector_init_with_capacity(vec, cap)                                 \
    ({                                                                      \
        VECTOR_ERROR __result = VECTOR_SUCCESS;                          \
        if ((cap) > VECTOR_MAX_CAPACITY) {                                 \
            __result = VECTOR_ERROR_INVALID_CAPACITY;                      \
        } else {                                                            \
            size_t __cap = (cap) > 0 ? (cap) : VECTOR_DEFAULT_CAPACITY;    \
            (vec)->data = kmalloc(__cap * sizeof(*(vec)->data));           \
            if ((vec)->data) {                                              \
                (vec)->length = 0;                                          \
                (vec)->capacity = __cap;                                    \
                (vec)->initialized = true;                                  \
                memset((vec)->data, 0, __cap * sizeof(*(vec)->data));      \
            } else {                                                        \
                __result = VECTOR_ERROR_OUT_OF_MEMORY;                     \
            }                                                               \
        }                                                                   \
        __result;                                                           \
    })

/**
 * @brief Check if vector is properly initialized
 * @param vec Pointer to vector
 * @return bool True if initialized, false otherwise
 */
#define vector_is_initialized(vec)          ((vec)->initialized)

/**
 * @brief Get the current length of the vector
 * @param vec Pointer to vector
 * @return size_t Current length, 0 if invalid
 */
#define vector_len(vec)                                                     \
    ((vec)->initialized ? (vec)->length : 0)

/**
 * @brief Get the current capacity of the vector
 * @param vec Pointer to vector
 * @return size_t Current capacity, 0 if invalid
 */
#define vector_capacity(vec)                                                \
    ((vec)->initialized ? (vec)->capacity : 0)

/**
 * @brief Check if vector is empty
 * @param vec Pointer to vector
 * @return bool True if empty, false otherwise
 */
#define vector_empty(vec)                   (vector_len(vec) == 0)

/**
 * @brief Safely get element at specified index
 * @param vec Pointer to vector
 * @param index Index to access
 * @param result Pointer to store result (can be NULL for bounds check only)
 * @return VECTOR_ERROR Success or error code
 */
#define vector_get(vec, index, result)                                      \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((index) >= (vec)->length) {                             \
            __error = VECTOR_ERROR_OUT_OF_BOUNDS;                          \
        } else if (result) {                                                \
            *(result) = (vec)->data[index];                                 \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Safely set element at specified index
 * @param vec Pointer to vector
 * @param index Index to set
 * @param value Value to set
 * @return VECTOR_ERROR Success or error code
 */
#define vector_set(vec, index, value)                                       \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((index) >= (vec)->length) {                             \
            __error = VECTOR_ERROR_OUT_OF_BOUNDS;                          \
        } else {                                                            \
            (vec)->data[index] = (value);                                   \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Unsafe direct access to element (use only when bounds are guaranteed)
 * @param vec Pointer to vector
 * @param index Index to access
 * @return Element at index (undefined behavior if out of bounds)
 */
#define vector_at(vec, index)               ((vec)->data[index])

/**
 * @brief Get pointer to element at specified index
 * @param vec Pointer to vector
 * @param index Index to access
 * @return Pointer to element at index, NULL if invalid or out of bounds
 */
#define vector_ptr_at(vec, index)                                           \
    ({                                                                      \
        void *__ptr = NULL;                                                 \
        if (vector_is_initialized(vec) && (index) < (vec)->length) {       \
            __ptr = &(vec)->data[index];                                    \
        }                                                                   \
        __ptr;                                                              \
    })

/**
 * @brief Get pointer to first element
 * @param vec Pointer to vector
 * @return Pointer to first element, NULL if empty or invalid
 */
#define vector_front(vec)                                                   \
    (vector_empty(vec) ? NULL : &(vec)->data[0])

/**
 * @brief Get pointer to last element
 * @param vec Pointer to vector
 * @return Pointer to last element, NULL if empty or invalid
 */
#define vector_back(vec)                                                    \
    (vector_empty(vec) ? NULL : &(vec)->data[(vec)->length - 1])

/**
 * @brief Internal helper to resize vector capacity
 * @param vec Pointer to vector
 * @param new_capacity New capacity
 * @return VECTOR_ERROR Success or error code
 */
#define __vector_resize(vec, new_capacity)                                  \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((new_capacity) > VECTOR_MAX_CAPACITY) {                 \
            __error = VECTOR_ERROR_OVERFLOW;                               \
        } else if ((new_capacity) > 0) {                                   \
            void *__new_data = krealloc((vec)->data,                       \
                                       (new_capacity) * sizeof(*(vec)->data)); \
            if (__new_data) {                                               \
                (vec)->data = __new_data;                                   \
                if ((new_capacity) > (vec)->capacity) {                    \
                    memset((vec)->data + (vec)->capacity, 0,               \
                          ((new_capacity) - (vec)->capacity) * sizeof(*(vec)->data)); \
                }                                                           \
                (vec)->capacity = (new_capacity);                          \
            } else {                                                        \
                __error = VECTOR_ERROR_OUT_OF_MEMORY;                      \
            }                                                               \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Reserve capacity for at least n elements
 * @param vec Pointer to vector
 * @param n Minimum capacity to reserve
 * @return VECTOR_ERROR Success or error code
 */
#define vector_reserve(vec, n)                                              \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((n) > (vec)->capacity) {                                \
            __error = __vector_resize(vec, n);                             \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Shrink capacity to fit current length
 * @param vec Pointer to vector
 * @return VECTOR_ERROR Success or error code
 */
#define vector_shrink_to_fit(vec)                                           \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((vec)->length < (vec)->capacity) {                      \
            size_t __new_cap = (vec)->length > 0 ? (vec)->length :         \
                              VECTOR_DEFAULT_CAPACITY;                     \
            __error = __vector_resize(vec, __new_cap);                     \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Append element to end of vector
 * @param vec Pointer to vector
 * @param elem Element to append
 * @return VECTOR_ERROR Success or error code
 */
#define vector_append(vec, elem)                                            \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!(vec)->initialized) {                                          \
            __error = vector_init(vec);                                     \
        }                                                                   \
        if (__error == VECTOR_SUCCESS) {                                   \
            if ((vec)->length >= (vec)->capacity) {                        \
                size_t __new_cap = (vec)->capacity * VECTOR_GROWTH_FACTOR; \
                if (__new_cap < (vec)->capacity) {                         \
                    __new_cap = VECTOR_MAX_CAPACITY;                       \
                }                                                           \
                __error = __vector_resize(vec, __new_cap);                 \
            }                                                               \
            if (__error == VECTOR_SUCCESS) {                               \
                (vec)->data[(vec)->length] = (elem);                       \
                (vec)->length++;                                            \
            }                                                               \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Insert element at specified index
 * @param vec Pointer to vector
 * @param index Index to insert at
 * @param elem Element to insert
 * @return VECTOR_ERROR Success or error code
 */
#define vector_insert(vec, index, elem)                                     \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((index) > (vec)->length) {                              \
            __error = VECTOR_ERROR_OUT_OF_BOUNDS;                          \
        } else {                                                            \
            if ((vec)->length >= (vec)->capacity) {                        \
                size_t __new_cap = (vec)->capacity * VECTOR_GROWTH_FACTOR; \
                __error = __vector_resize(vec, __new_cap);                 \
            }                                                               \
            if (__error == VECTOR_SUCCESS) {                               \
                if ((index) < (vec)->length) {                             \
                    memmove(&(vec)->data[(index) + 1],                     \
                           &(vec)->data[index],                            \
                           ((vec)->length - (index)) * sizeof(*(vec)->data)); \
                }                                                           \
                (vec)->data[index] = (elem);                               \
                (vec)->length++;                                            \
            }                                                               \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Remove element at specified index
 * @param vec Pointer to vector
 * @param index Index to remove
 * @return VECTOR_ERROR Success or error code
 */
#define vector_remove(vec, index)                                           \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((index) >= (vec)->length) {                             \
            __error = VECTOR_ERROR_OUT_OF_BOUNDS;                          \
        } else {                                                            \
            if ((index) < (vec)->length - 1) {                             \
                memmove(&(vec)->data[index],                               \
                       &(vec)->data[(index) + 1],                         \
                       ((vec)->length - (index) - 1) * sizeof(*(vec)->data)); \
            }                                                               \
            (vec)->length--;                                                \
            /* Shrink if using less than 1/4 of capacity */               \
            if ((vec)->length > 0 &&                                       \
                (vec)->capacity > VECTOR_DEFAULT_CAPACITY &&               \
                (vec)->length * VECTOR_SHRINK_THRESHOLD < (vec)->capacity) { \
                size_t __new_cap = (vec)->capacity / VECTOR_GROWTH_FACTOR; \
                if (__new_cap >= (vec)->length) {                          \
                    __vector_resize(vec, __new_cap);                       \
                }                                                           \
            }                                                               \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Remove last element from vector
 * @param vec Pointer to vector
 * @return VECTOR_ERROR Success or error code
 */
#define vector_pop(vec)                                                     \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if ((vec)->length == 0) {                                   \
            __error = VECTOR_ERROR_OUT_OF_BOUNDS;                          \
        } else {                                                            \
            __error = vector_remove(vec, (vec)->length - 1);               \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Remove first occurrence of value
 * @param vec Pointer to vector
 * @param value Value to remove
 * @return VECTOR_ERROR Success or VECTOR_ERROR_NOT_FOUND
 */
#define vector_remove_value(vec, value)                                     \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_ERROR_NOT_FOUND;                   \
        if (vector_is_initialized(vec)) {                                   \
            for (size_t __i = 0; __i < (vec)->length; __i++) {             \
                if ((vec)->data[__i] == (value)) {                         \
                    __error = vector_remove(vec, __i);                     \
                    break;                                                  \
                }                                                           \
            }                                                               \
        } else {                                                            \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Find index of first occurrence of value
 * @param vec Pointer to vector
 * @param value Value to find
 * @param index Pointer to store found index
 * @return VECTOR_ERROR Success or error code
 */
#define vector_find(vec, value, index)                                      \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_ERROR_NOT_FOUND;                   \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else if (!(index)) {                                             \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else {                                                            \
            for (size_t __i = 0; __i < (vec)->length; __i++) {             \
                if ((vec)->data[__i] == (value)) {                         \
                    *(index) = __i;                                         \
                    __error = VECTOR_SUCCESS;                              \
                    break;                                                  \
                }                                                           \
            }                                                               \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Clear all elements from vector (doesn't free memory)
 * @param vec Pointer to vector
 * @return VECTOR_ERROR Success or error code
 */
#define vector_clear(vec)                                                   \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(vec)) {                                  \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else {                                                            \
            (vec)->length = 0;                                              \
            memset((vec)->data, 0, (vec)->capacity * sizeof(*(vec)->data)); \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Free all memory associated with vector
 * @param vec Pointer to vector
 * @return VECTOR_ERROR Success or error code
 */
#define vector_free(vec)                                                    \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!(vec)) {                                                       \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else {                                                            \
            if ((vec)->data) {                                              \
                kfree((vec)->data);                                         \
            }                                                               \
            (vec)->data = NULL;                                             \
            (vec)->length = 0;                                              \
            (vec)->capacity = 0;                                            \
            (vec)->initialized = false;                                     \
        }                                                                   \
        __error;                                                            \
    })

/**
 * @brief Copy vector contents to another vector
 * @param src Source vector
 * @param dst Destination vector (will be reinitialized)
 * @return VECTOR_ERROR Success or error code
 */
#define vector_copy(src, dst)                                               \
    ({                                                                      \
        VECTOR_ERROR __error = VECTOR_SUCCESS;                           \
        if (!vector_is_initialized(src) || !(dst)) {                       \
            __error = VECTOR_ERROR_NULL_POINTER;                           \
        } else {                                                            \
            vector_free(dst);                                               \
            __error = vector_init_with_capacity(dst, (src)->capacity);     \
            if (__error == VECTOR_SUCCESS) {                               \
                memcpy((dst)->data, (src)->data,                           \
                      (src)->length * sizeof(*(src)->data));              \
                (dst)->length = (src)->length;                             \
            }                                                               \
        }                                                                   \
        __error;                                                            \
    })

// Legacy compatibility macros (deprecated, use new error-checked versions)
#define vector_erase(vec, index)            vector_remove(vec, index)
#define vector_erase_value(vec, val)        vector_remove_value(vec, val)

// Additional error code for find operations
#undef VECTOR_ERROR_NOT_FOUND
#define VECTOR_ERROR_NOT_FOUND              -6