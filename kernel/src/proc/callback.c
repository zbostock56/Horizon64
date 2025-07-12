/**
 * @file callback.c
 * @author Zack Bostock
 * @brief Functionality pertaining to subscribing and publishing event callbacks
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <proc/callback.h>

#include <common/vector.h>
#include <common/lock.h>

#include <sys/acpi/hpet.h>

#include <proc/ctxsw.h>

/* List of subscribers to an event and list of published events */
vector_new_static(CALLBACK, publishers);
vector_new_static(CALLBACK, subscribers);

static LOCK cb_lock;

/**
 * @brief Publishes a callback
 *
 * @param id ID of the process who published
 * @param type Type of callback
 * @param param Parameters of callback
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cb_publish(PROC_ID id, CB_TYPE type, CB_PARAM param) {
    if (type == CB_UNDEF) {
        return SYS_ERR;
    }

    CALLBACK cb = {
        .publisher_id = id,
        .subscriber_id = UINT64_MAX,
        .type = type,
        .param = param
    };

    cb.timestamp = hpet_get_millis();

    LOCK_LOCK(&cb_lock);
    vector_append(&publishers, cb);
    UNLOCK_LOCK(&cb_lock);

    return SYS_OK;
}

/**
 * @brief Subscribes to a specific callback type
 *
 * @param id ID of the process who is subscribing
 * @param type Type of callback to subscribe to
 * @param param Paramters of the callback
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cb_subscribe(PROC_ID id, CB_TYPE type, CB_PARAM *param) {
    if (type == CB_UNDEF) {
        return SYS_ERR;
    }

    CALLBACK cb = {
        .publisher_id = UINT64_MAX,
        .subscriber_id = id,
        .type = type,
        .param = 0,
    };

    cb.timestamp = hpet_get_millis();

    LOCK_LOCK(&cb_lock);
    vector_append(&subscribers, cb);
    UNLOCK_LOCK(&cb_lock);

    cb = sched_wait_callback(cb);
    *param = cb.param;

    return SYS_OK;
}

/**
 * @brief Dispatcher for callbacks
 *
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cb_dispatch() {
    LOCK_LOCK(&cb_lock);

    /* Kick off all callbacks */
    while (TRUE) {
        if (vector_len(&publishers) > 0) {
            CALLBACK cb = vector_at(&publishers, 0);
            /* If the callback is an undefined one, just remove it */
            if (cb.type == CB_UNDEF) {
                vector_erase(&publishers, 0);
                continue;
            }
            /* Send off the callback to be exercised */
            if (sched_resume_callback(cb)) {
                vector_erase(&publishers, 0);
            }
            break;
        } else {
            break;
        }
    }

    UNLOCK_LOCK(&cb_lock);

    return SYS_OK;
}