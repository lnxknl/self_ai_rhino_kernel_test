#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Return status codes
#define RHINO_SUCCESS           0
#define RHINO_NULL_PTR         1
#define RHINO_MUTEX_OWNER_ERR  2
#define RHINO_MUTEX_NOT_OWNER  3

// Mutex structure
typedef struct {
    pthread_mutex_t mutex;
    pthread_t owner;
    const char *name;
    int lock_count;  // For recursive mutex support
} kmutex_t;

// Initialize mutex
int krhino_mutex_create(kmutex_t *mutex, const char *name) {
    if (mutex == NULL || name == NULL) {
        return RHINO_NULL_PTR;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    
    int ret = pthread_mutex_init(&mutex->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    if (ret != 0) {
        return RHINO_NULL_PTR;
    }

    mutex->name = name;
    mutex->owner = 0;
    mutex->lock_count = 0;
    
    printf("Mutex '%s' created\n", name);
    return RHINO_SUCCESS;
}

// Lock mutex
int krhino_mutex_lock(kmutex_t *mutex) {
    if (mutex == NULL) {
        return RHINO_NULL_PTR;
    }

    int ret = pthread_mutex_lock(&mutex->mutex);
    if (ret != 0) {
        return RHINO_MUTEX_OWNER_ERR;
    }

    mutex->owner = pthread_self();
    mutex->lock_count++;
    
    printf("Thread %lu locked mutex '%s' (count: %d)\n", 
           (unsigned long)pthread_self(), mutex->name, mutex->lock_count);
    return RHINO_SUCCESS;
}

// Unlock mutex
int krhino_mutex_unlock(kmutex_t *mutex) {
    if (mutex == NULL) {
        return RHINO_NULL_PTR;
    }

    if (mutex->owner != pthread_self()) {
        return RHINO_MUTEX_NOT_OWNER;
    }

    mutex->lock_count--;
    printf("Thread %lu unlocked mutex '%s' (count: %d)\n", 
           (unsigned long)pthread_self(), mutex->name, mutex->lock_count);

    if (mutex->lock_count == 0) {
        mutex->owner = 0;
    }

    return pthread_mutex_unlock(&mutex->mutex);
}

// Delete mutex
int krhino_mutex_del(kmutex_t *mutex) {
    if (mutex == NULL) {
        return RHINO_NULL_PTR;
    }

    printf("Deleting mutex '%s'\n", mutex->name);
    return pthread_mutex_destroy(&mutex->mutex);
}

// Shared resource
int shared_counter = 0;

// Test function for threads
void* test_thread(void* arg) {
    kmutex_t* mutex = (kmutex_t*)arg;
    int i;
    
    for (i = 0; i < 3; i++) {
        // Lock mutex
        if (krhino_mutex_lock(mutex) != RHINO_SUCCESS) {
            printf("Thread %lu failed to lock mutex\n", (unsigned long)pthread_self());
            continue;
        }

        // Access shared resource
        shared_counter++;
        printf("Thread %lu: counter = %d\n", (unsigned long)pthread_self(), shared_counter);
        
        // Simulate some work
        usleep(100000);  // 100ms

        // Test recursive locking
        if (i == 1) {  // On second iteration
            printf("Thread %lu testing recursive lock\n", (unsigned long)pthread_self());
            krhino_mutex_lock(mutex);
            shared_counter++;
            printf("Thread %lu: counter = %d (recursive)\n", 
                   (unsigned long)pthread_self(), shared_counter);
            krhino_mutex_unlock(mutex);
        }

        // Unlock mutex
        krhino_mutex_unlock(mutex);
        
        // Small delay between iterations
        usleep(50000);  // 50ms
    }

    return NULL;
}

int main() {
    kmutex_t mutex;
    pthread_t threads[3];
    int i;

    printf("Testing Mutex Implementation\n");
    printf("----------------------------\n");

    // Create mutex
    if (krhino_mutex_create(&mutex, "test_mutex") != RHINO_SUCCESS) {
        printf("Failed to create mutex\n");
        return 1;
    }

    // Create threads
    printf("\nCreating threads...\n");
    for (i = 0; i < 3; i++) {
        if (pthread_create(&threads[i], NULL, test_thread, &mutex) != 0) {
            printf("Failed to create thread %d\n", i);
            return 1;
        }
    }

    // Wait for threads to complete
    for (i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    // Delete mutex
    krhino_mutex_del(&mutex);

    printf("\nFinal counter value: %d\n", shared_counter);
    printf("Test completed successfully!\n");

    return 0;
}
