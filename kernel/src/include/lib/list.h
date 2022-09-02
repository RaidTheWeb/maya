#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
#include <lib/string.h>
#include <mm/pmm.h>

#define LIST_INVALID (-1)

#define LIST_INIT {0}

#define LIST_TYPE(TYPE) \
    struct { \
        TYPE *data; \
        uint64_t length; \
        uint64_t capacity; \
    }

#define LIST_ENSURE_LENGTH(LIST, LENGTH) do { \
    __auto_type LIST_ENSURE_LENGTH_list = LIST; \
    if ((LENGTH) >= LIST_ENSURE_LENGTH_list->capacity) { \
        __auto_type LIST_ENSURE_LENGTH_old_capacity = LIST_ENSURE_LENGTH_list->capacity; \
        if (LIST_ENSURE_LENGTH_list->capacity == 0) { \
            LIST_ENSURE_LENGTH_list->capacity = 8; \
        } else { \
            LIST_ENSURE_LENGTH_list->capacity *= 2; \
        } \
        __auto_type LIST_ENSURE_LENGTH_old_data = LIST_ENSURE_LENGTH_list->data; \
        LIST_ENSURE_LENGTH_list->data = malloc(LIST_ENSURE_LENGTH_list->capacity * sizeof(*LIST_ENSURE_LENGTH_list->data)); \
        if (LIST_ENSURE_LENGTH_old_data != NULL) { \
            memcpy(LIST_ENSURE_LENGTH_list->data, LIST_ENSURE_LENGTH_old_data, \
                LIST_ENSURE_LENGTH_old_capacity * sizeof(*LIST_ENSURE_LENGTH_list->data)); \
            free(LIST_ENSURE_LENGTH_old_data); \
        } \
    } \
} while (0)

#define LIST_PUSHBACK(LIST, VALUE) ({ \
    __auto_type LIST_PUSHBACK_list = LIST; \
    LIST_ENSURE_LENGTH(LIST, LIST_PUSHBACK_list->length); \
    LIST_PUSHBACK_list->data[LIST_PUSHBACK_list->length++] = VALUE; \
    LIST_PUSHBACK_list->length - 1; \
})

#define LIST_PUSH_FRONT(LIST, VALUE) LIST_INSERT(LIST, 0, VALUE)

#define LIST_INSERT(LIST, IDX, VALUE) do { \
    __auto_type LIST_INSERT_list = LIST; \
    size_t LIST_INSERT_index = IDX; \
    LIST_ENSURE_LENGTH(LIST, LIST_INSERT_list>length); \
    for (size_t LIST_INSERT_i = LIST_INSERT_list->length; LIST_INSERT_i > LIST_INSERT_index; LIST_INSERT_i--) { \
        LIST_INSERT_list->data[LIST_INSERT_i] = LIST_INSERT_list->data[LIST_INSERT_i - 1]; \
    } \
    LIST_INSERT_list->length++; \
    LIST_INSERT_list->data[LIST_INSERT_index] = VALUE; \
} while (0)

#define LIST_REMOVE(LIST, IDX) do { \
    __auto_type LIST_REMOVE_list = LIST; \
    for (size_t LIST_REMOVE_i = (IDX); LIST_REMOVE_i < LIST_REMOVE_list->length - 1; LIST_REMOVE_i++) { \
        LIST_REMOVE_list->data[LIST_REMOVE_i] = LIST_REMOVE_list->data[LIST_REMOVE_i + 1]; \
    } \
    LIST_REMOVE_list->length--; \
} while (0)

#define LIST_ITEM(LIST, IDX) ({ \
    size_t LIST_ITEM_idx = IDX; \
    __auto_type LIST_ITEM_list = LIST; \
    __auto_type LIST_ITEM_result = (typeof(*LIST_ITEM_list->data))LIST_INVALID; \
    if (LIST_ITEM_idx < LIST_ITEM_list->length) { \
        LIST_ITEM_result = LIST_ITEM_list->data[LIST_ITEM_idx]; \
    } \
    LIST_ITEM_result; \
})

#define LIST_FIND(LIST, VALUE) ({ \
    __auto_type LIST_FIND_list = LIST; \
    ssize_t LIST_FIND_result = LIST_INVALID; \
    for (size_t LIST_FIND_i = 0; LIST_FIND_i < LIST_FIND_list->length; LIST_FIND_i++) { \
        if (LIST_FIND_list->data[LIST_FIND_i] == (VALUE)) { \
            LIST_FIND_result = LIST_FIND_i; \
            break; \
        } \
    } \
    LIST_FIND_result; \
})

#define LIST_REMOVE_BY_VALUE(LIST, VALUE) do { \
    __auto_type LIST_REMOVE_BY_VALUE_list = LIST; \
    __auto_type LIST_REMOVE_BY_VALUE_v = VALUE; \
    size_t LIST_REMOVE_BY_VALUE_i = LIST_FIND(LIST_REMOVE_BY_VALUE_list, LIST_REMOVE_BY_VALUE_v); \
    LIST_REMOVE(LIST_REMOVE_BY_VALUE_list, LIST_REMOVE_BY_VALUE_i); \
} while (0)

#define LIST_FOR_EACH(LIST, BINDING, ...) do { \
    __auto_type LIST_FOR_EACH_list = LIST; \
    for (size_t LIST_FOR_EACH_i = 0; LIST_FOR_EACH_i < LIST_FOR_EACH_list->length; LIST_FOR_EACH_i++) { \
        __auto_type BINDING = &LIST_FOR_EACH_list->data[LIST_FOR_EACH_i]; \
        __VA_ARGS__ \
    } \
} while (0)

#endif
