/**
 * @file callback.c
 * @author Zack Bostock
 * @brief Event callback system
 * @date 2025
 * 
 * Provides functionality for subscribing to and publishing event callbacks
 * with proper error handling, statistics tracking, and optimized dispatch.
 * 
 * @copyright Copyright (c) 2025
 */

#include <proc/callback.h>
#include <common/vector.h>
#include <common/lock.h>
#include <sys/acpi/hpet.h>
#include <proc/ctxsw.h>
#include <common/kprint.h>

/* Configuration constants */
#define CB_MAX_PUBLISHERS    1024
#define CB_MAX_SUBSCRIBERS   512
#define CB_DISPATCH_TIMEOUT  1000  /* milliseconds */
#define CB_CLEANUP_THRESHOLD 100   /* Clean up after this many processed events */

/* Statistics structure */
typedef struct {
    uint64_t published_count;
    uint64_t dispatched_count;
    uint64_t failed_dispatches;
    uint64_t cleanup_count;
    uint64_t max_publisher_queue;
    uint64_t max_subscriber_queue;
} CALLBACK_STATS;

/* Event queue structures */
vector_new_static(CALLBACK, publishers);
vector_new_static(CALLBACK, subscribers);

/* Synchronization and state */
static LOCK cb_lock = {0};
static CALLBACK_STATS cb_stats = {0};
static uint8_t cb_system_initialized = FALSE;
static uint64_t processed_events = 0;

/* String buffer for callback types */
static const char *CB_TYPE_STRINGS[] = {
#define X(name, str) str,
    CB_TYPE_LIST
#undef X
};

/**
 * @brief Validates callback parameters
 * 
 * @param id Process ID
 * @param type Callback type
 * @return STATUS SYS_OK if valid, SYS_ERR if invalid
 */
static STATUS validate_callback_params(PROC_ID id, CB_TYPE type) {
    if (type == CB_UNDEF) {
        klogw("CALLBACK: Invalid callback type CB_UNDEF\n");
        return SYS_ERR;
    }
    
    if (type >= CB_NUM_TYPES) {
        klogw("CALLBACK: Invalid callback type %d (max: %d)\n", type, CB_NUM_TYPES - 1);
        return SYS_ERR;
    }
    
    if (id == UINT64_MAX) {
        klogw("CALLBACK: Invalid process ID\n");
        return SYS_ERR;
    }
    
    return SYS_OK;
}

/**
 * @brief Updates queue statistics
 */
static void update_queue_stats(void) {
    uint64_t pub_len = vector_len(&publishers);
    uint64_t sub_len = vector_len(&subscribers);
    
    if (pub_len > cb_stats.max_publisher_queue) {
        cb_stats.max_publisher_queue = pub_len;
    }
    
    if (sub_len > cb_stats.max_subscriber_queue) {
        cb_stats.max_subscriber_queue = sub_len;
    }
}

/**
 * @brief Cleans up processed events and defragments queues
 * 
 * @return STATUS SYS_OK if successful
 */
static STATUS cleanup_processed_events(void) {
    size_t removed = 0;
    
    /* Remove undefined callbacks from publishers queue */
    for (size_t i = 0; i < vector_len(&publishers); ) {
        CALLBACK *cb = vector_ptr_at(&publishers, i);
        if (cb && cb->type == CB_UNDEF) {
            vector_erase(&publishers, i);
            removed++;
        } else {
            i++;
        }
    }
    
    if (removed > 0) {
        cb_stats.cleanup_count++;
        klogd("CALLBACK: Cleaned up %d invalid callbacks\n", removed);
    }
    
    processed_events = 0;
    return SYS_OK;
}

/**
 * @brief Checks if there's a subscriber waiting for a specific callback type
 * 
 * @param type Callback type to check
 * @param subscriber_id Output parameter for subscriber ID
 * @return TRUE if subscriber found, FALSE otherwise
 */
static uint8_t is_subscriber_waiting(CB_TYPE type, PROC_ID *subscriber_id) {
    for (size_t i = 0; i < vector_len(&subscribers); i++) {
        const CALLBACK *sub = vector_ptr_at(&subscribers, i);
        if (sub && sub->type == type) {
            if (subscriber_id) {
                *subscriber_id = sub->subscriber_id;
            }
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Attempts to match and dispatch a published callback
 * 
 * @param published_cb The callback to dispatch
 * @return STATUS SYS_OK if dispatched, SYS_ERR if no matching subscriber
 */
static STATUS match_and_dispatch_callback(const CALLBACK *published_cb) {
    if (!published_cb) return SYS_ERR;
    
    PROCESS *p;

    /* Look for matching subscriber */
    for (size_t i = 0; i < vector_len(&subscribers); i++) {
        CALLBACK *subscriber = vector_ptr_at(&subscribers, i);
        if (subscriber && subscriber->type == published_cb->type) {
            /* Create dispatch callback with subscriber info */
            CALLBACK dispatch_cb = *published_cb;
            dispatch_cb.subscriber_id = subscriber->subscriber_id;
            
            /* Attempt to resume the waiting process */
            if (sched_resume_callback(dispatch_cb)) {
                /* Remove the subscriber as it's been serviced */
                vector_erase(&subscribers, i);
                cb_stats.dispatched_count++;
                p = sched_get_proc_by_id(subscriber->subscriber_id);
                if (p) {
                    klogd("CALLBACK: Dispatched type %s to process (%s: pid %d)\n", 
                        CB_TYPE_STRINGS[published_cb->type], p->name, p->id);
                } else {
                    klogd("CALLBACK: Dispatched type %s to process %d\n", 
                        CB_TYPE_STRINGS[published_cb->type], subscriber->subscriber_id);
                }
                return SYS_OK;
            } else {
                p = sched_get_proc_by_id(subscriber->subscriber_id);
                if (p) {
                    klogw("CALLBACK: Failed to resume process (%s: pid %d) for callback type %s\n",
                        p->name, p->id, CB_TYPE_STRINGS[published_cb->type]);
                } else {
                    klogw("CALLBACK: Failed to resume process %d for callback type %s\n",
                        subscriber->subscriber_id, CB_TYPE_STRINGS[published_cb->type]);
                }
                cb_stats.failed_dispatches++;
                return SYS_ERR;
            }
        }
    }
    
    /* No matching subscriber found */
    klogd("CALLBACK: No subscriber found for callback type %s\n", CB_TYPE_STRINGS[published_cb->type]);
    return SYS_OK;
}

/**
 * @brief Initializes the callback system
 * 
 * @return STATUS SYS_OK if successful
 */
STATUS cb_init() {
    if (cb_system_initialized) {
        klogw("CALLBACK: System already initialized\n");
        return SYS_OK;
    }
    
    /* Initialize vectors */
    vector_init(&publishers);
    vector_init(&subscribers);
    
    /* Reserve space for better performance */
    vector_reserve(&publishers, CB_MAX_PUBLISHERS);
    vector_reserve(&subscribers, CB_MAX_SUBSCRIBERS);
    
    /* Initialize statistics */
    memset(&cb_stats, 0, sizeof(cb_stats));
    
    cb_system_initialized = TRUE;
    klogs("CALLBACK: System initialized successfully\n");
    return SYS_OK;
}

/**
 * @brief Publishes a callback event
 *
 * @param id ID of the publishing process
 * @param type Type of callback event
 * @param param Parameters for the callback
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cb_publish(PROC_ID id, CB_TYPE type, CB_PARAM param) {
    if (!cb_system_initialized) {
        kloge("CALLBACK: System not initialized\n");
        return SYS_ERR;
    }
    
    if (validate_callback_params(id, type) != SYS_OK) {
        return SYS_ERR;
    }
    
    /* Check for queue overflow */
    if (vector_len(&publishers) >= CB_MAX_PUBLISHERS) {
        kloge("CALLBACK: Publisher queue full (%d events)\n", CB_MAX_PUBLISHERS);
        return SYS_ERR;
    }
    
    /* Create callback event */
    CALLBACK cb = {
        .publisher_id = id,
        .subscriber_id = UINT64_MAX,
        .type = type,
        .param = param,
        .timestamp = hpet_get_millis()
    };
    
    LOCK_LOCK(&cb_lock);
    
    /* Try immediate dispatch if there's a waiting subscriber */
    PROC_ID waiting_subscriber;
    if (is_subscriber_waiting(type, &waiting_subscriber)) {
        cb.subscriber_id = waiting_subscriber;
        if (match_and_dispatch_callback(&cb) == SYS_OK) {
            cb_stats.published_count++;
            update_queue_stats();
            UNLOCK_LOCK(&cb_lock);
            return SYS_OK;
        }
    }
    
    /* No immediate match, queue for later dispatch */
    if (vector_append(&publishers, cb) != 0) {
        kloge("CALLBACK: Failed to queue published event\n");
        UNLOCK_LOCK(&cb_lock);
        return SYS_ERR;
    }
    
    cb_stats.published_count++;
    update_queue_stats();
    
    UNLOCK_LOCK(&cb_lock);
    
    PROCESS *p;
    if ((p = sched_get_curr_proc())) {
        klogd("CALLBACK: Published type %s from process (%s: pid %d) (queued)\n", CB_TYPE_STRINGS[type], p->name, p->id);
    } else {
        klogd("CALLBACK: Process %d subscribed to type %d\n", id, CB_TYPE_STRINGS[type]);
    }
    return SYS_OK;
}

/**
 * @brief Subscribes to a specific callback type
 *
 * @param id ID of the subscribing process
 * @param type Type of callback to subscribe to
 * @param param Output parameter for received callback data
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cb_subscribe(PROC_ID id, CB_TYPE type, CB_PARAM *param) {
    if (!cb_system_initialized) {
        kloge("CALLBACK: System not initialized\n");
        return SYS_ERR;
    }
    
    if (!param) {
        kloge("CALLBACK: NULL parameter pointer\n");
        return SYS_ERR;
    }
    
    if (validate_callback_params(id, type) != SYS_OK) {
        return SYS_ERR;
    }
    
    /* Check for queue overflow */
    if (vector_len(&subscribers) >= CB_MAX_SUBSCRIBERS) {
        kloge("CALLBACK: Subscriber queue full (%d entries)\n", CB_MAX_SUBSCRIBERS);
        return SYS_ERR;
    }
    
    /* Create subscription entry */
    CALLBACK subscription = {
        .publisher_id = UINT64_MAX,
        .subscriber_id = id,
        .type = type,
        .param = 0,
        .timestamp = hpet_get_millis()
    };
    
    LOCK_LOCK(&cb_lock);
    
    if (vector_append(&subscribers, subscription) != 0) {
        kloge("CALLBACK: Failed to queue subscription\n");
        UNLOCK_LOCK(&cb_lock);
        return SYS_ERR;
    }
    
    update_queue_stats();
    UNLOCK_LOCK(&cb_lock);
    
    PROCESS *p;
    if ((p = sched_get_curr_proc())) {
        klogd("CALLBACK: (%s: pid %d) subscribed to type %s\n", p->name, p->id, CB_TYPE_STRINGS[type]);
    } else {
        klogd("CALLBACK: Process %d subscribed to type %s\n", id, CB_TYPE_STRINGS[type]);
    }
    
    /* Wait for callback to be delivered */
    CALLBACK delivered_cb = sched_wait_callback(subscription);
    *param = delivered_cb.param;
    
    return SYS_OK;
}

/**
 * @brief Dispatches pending callbacks to waiting subscribers
 *
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cb_dispatch(void) {
    if (!cb_system_initialized) {
        kloge("CALLBACK: System not initialized\n");
        return SYS_ERR;
    }
    
    LOCK_LOCK(&cb_lock);
    
    size_t processed_this_round = 0;
    uint8_t made_progress = TRUE;
    
    /* Process pending callbacks */
    while (made_progress && vector_len(&publishers) > 0) {
        made_progress = FALSE;
        
        for (size_t i = 0; i < vector_len(&publishers); i++) {
            const CALLBACK *cb = vector_ptr_at(&publishers, i);
            if (!cb) continue;
            
            /* Skip invalid callbacks */
            if (cb->type == CB_UNDEF) {
                vector_erase(&publishers, i);
                made_progress = TRUE;
                break;
            }
            
            /* Try to dispatch this callback */
            if (match_and_dispatch_callback(cb) == SYS_OK) {
                vector_erase(&publishers, i);
                processed_this_round++;
                processed_events++;
                made_progress = TRUE;
                break;
            }
        }
        
        /* Prevent infinite loops */
        if (processed_this_round > CB_MAX_PUBLISHERS) {
            klogw("CALLBACK: Dispatch loop limit reached\n");
            break;
        }
    }
    
    /* Periodic cleanup */
    if (processed_events >= CB_CLEANUP_THRESHOLD) {
        cleanup_processed_events();
    }
    
    update_queue_stats();
    UNLOCK_LOCK(&cb_lock);
    
    if (processed_this_round > 0) {
        klogd("CALLBACK: Dispatched %d callbacks\n", processed_this_round);
    }
    
    return SYS_OK;
}

/**
 * @brief Gets callback system statistics
 * 
 * @param stats Output buffer for statistics
 * @return STATUS SYS_OK if successful
 */
STATUS cb_get_stats(CALLBACK_STATS *stats) {
    if (!stats || !cb_system_initialized) {
        return SYS_ERR;
    }
    
    LOCK_LOCK(&cb_lock);
    *stats = cb_stats;
    UNLOCK_LOCK(&cb_lock);
    
    return SYS_OK;
}

/**
 * @brief Shuts down the callback system and cleans up resources
 * 
 * @return STATUS SYS_OK if successful
 */
STATUS cb_shutdown(void) {
    if (!cb_system_initialized) {
        return SYS_OK;
    }
    
    LOCK_LOCK(&cb_lock);
    
    /* Clear all queues */
    vector_clear(&publishers);
    vector_clear(&subscribers);
    
    /* Reset statistics */
    klogs("CALLBACK: Final stats - Published: %d, Dispatched: %d, Failed: %d\n",
          cb_stats.published_count, cb_stats.dispatched_count, cb_stats.failed_dispatches);
    
    cb_system_initialized = FALSE;
    
    UNLOCK_LOCK(&cb_lock);
    
    klogs("CALLBACK: System shutdown complete\n");
    return SYS_OK;
}