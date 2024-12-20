#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Event options
#define EVENT_OPT_AND          0x01  // Wait for all flags
#define EVENT_OPT_OR           0x02  // Wait for any flag
#define EVENT_OPT_CLEAR        0x04  // Clear flags after get

// Return status codes
#define RHINO_SUCCESS          0
#define RHINO_NULL_PTR        1
#define RHINO_EVENT_NOT_READY 2
#define RHINO_TIMEOUT         3

// Event structure
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    const char *name;
    uint32_t flags;
} kevent_t;

// Initialize event
int krhino_event_create(kevent_t *event, const char *name, uint32_t flags) {
    if (event == NULL || name == NULL) {
        return RHINO_NULL_PTR;
    }

    pthread_mutex_init(&event->mutex, NULL);
    pthread_cond_init(&event->cond, NULL);
    event->name = name;
    event->flags = flags;

    printf("Event '%s' created with initial flags: 0x%08X\n", name, flags);
    return RHINO_SUCCESS;
}

// Set event flags
int krhino_event_set(kevent_t *event, uint32_t flags, uint8_t opt) {
    if (event == NULL) {
        return RHINO_NULL_PTR;
    }

    pthread_mutex_lock(&event->mutex);
    
    if (opt & EVENT_OPT_CLEAR) {
        event->flags = flags;
    } else {
        event->flags |= flags;
    }
    
    printf("Event '%s' flags set to: 0x%08X\n", event->name, event->flags);
    
    // Wake up all waiting threads
    pthread_cond_broadcast(&event->cond);
    pthread_mutex_unlock(&event->mutex);

    return RHINO_SUCCESS;
}

// Get (wait for) event flags
int krhino_event_get(kevent_t *event, uint32_t flags, uint8_t opt,
                    uint32_t *actl_flags, int timeout_ms) {
    if (event == NULL || actl_flags == NULL) {
        return RHINO_NULL_PTR;
    }

    pthread_mutex_lock(&event->mutex);
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    
    int ret = RHINO_SUCCESS;
    while (1) {
        *actl_flags = event->flags;
        
        int flags_met = 0;
        if (opt & EVENT_OPT_AND) {
            flags_met = (event->flags & flags) == flags;
        } else {  // EVENT_OPT_OR
            flags_met = (event->flags & flags) != 0;
        }
        
        if (flags_met) {
            if (opt & EVENT_OPT_CLEAR) {
                event->flags &= ~flags;
            }
            break;
        }
        
        if (timeout_ms == 0) {
            ret = RHINO_EVENT_NOT_READY;
            break;
        }
        
        // Wait for event with timeout
        int wait_ret = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
        if (wait_ret != 0) {
            ret = RHINO_TIMEOUT;
            break;
        }
    }
    
    pthread_mutex_unlock(&event->mutex);
    return ret;
}

// Delete event
int krhino_event_del(kevent_t *event) {
    if (event == NULL) {
        return RHINO_NULL_PTR;
    }

    printf("Deleting event '%s'\n", event->name);
    pthread_mutex_destroy(&event->mutex);
    pthread_cond_destroy(&event->cond);
    return RHINO_SUCCESS;
}

// Thread function for event consumer
void* consumer_thread(void* arg) {
    kevent_t* event = (kevent_t*)arg;
    uint32_t flags_to_wait = 0x03;  // Wait for bits 0 and 1
    uint32_t actual_flags;
    
    printf("Consumer waiting for flags: 0x%08X\n", flags_to_wait);
    
    // Wait for both flags (AND operation)
    int ret = krhino_event_get(event, flags_to_wait, EVENT_OPT_AND | EVENT_OPT_CLEAR,
                              &actual_flags, 5000);
    
    if (ret == RHINO_SUCCESS) {
        printf("Consumer got flags: 0x%08X\n", actual_flags);
    } else if (ret == RHINO_TIMEOUT) {
        printf("Consumer timeout waiting for flags\n");
    } else {
        printf("Consumer error: %d\n", ret);
    }
    
    return NULL;
}

// Thread function for event producer
void* producer_thread(void* arg) {
    kevent_t* event = (kevent_t*)arg;
    
    // Set flags one by one with delays
    printf("Producer setting flag 0x01\n");
    krhino_event_set(event, 0x01, 0);
    usleep(100000);  // 100ms delay
    
    printf("Producer setting flag 0x02\n");
    krhino_event_set(event, 0x02, 0);
    
    return NULL;
}

int main() {
    kevent_t event;
    pthread_t consumer, producer;

    printf("Event Flag Test\n");
    printf("--------------\n");

    // Create event
    if (krhino_event_create(&event, "test_event", 0) != RHINO_SUCCESS) {
        printf("Failed to create event\n");
        return 1;
    }

    // Create consumer and producer threads
    pthread_create(&consumer, NULL, consumer_thread, &event);
    usleep(50000);  // Small delay to ensure consumer starts first
    pthread_create(&producer, NULL, producer_thread, &event);

    // Wait for threads to complete
    pthread_join(consumer, NULL);
    pthread_join(producer, NULL);

    // Delete event
    krhino_event_del(&event);

    printf("Test completed successfully!\n");
    return 0;
}
