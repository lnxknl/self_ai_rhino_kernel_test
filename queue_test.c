#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simplified definitions
typedef char * name_t;
typedef unsigned int size_t;
typedef unsigned char uint8_t;
typedef int kstat_t;

#define RHINO_SUCCESS 0
#define RHINO_INV_PARAM -1
#define NULL_PARA_CHK(para) if (para == NULL) return RHINO_INV_PARAM

// Simple ring buffer implementation
typedef struct {
    void **buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
} ring_buffer_t;

typedef struct {
    ring_buffer_t ring_buf;
    size_t size;
    size_t cur_num;
    size_t peak_num;
    name_t name;
} kqueue_t;

// Ring buffer functions
void ring_buffer_init(ring_buffer_t *rb, void **buffer, size_t size) {
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

int ring_buffer_push(ring_buffer_t *rb, void *item) {
    if (rb->count == rb->size) {
        return -1;  // Buffer full
    }
    rb->buffer[rb->tail] = item;
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count++;
    return 0;
}

int ring_buffer_pop(ring_buffer_t *rb, void **item) {
    if (rb->count == 0) {
        return -1;  // Buffer empty
    }
    *item = rb->buffer[rb->head];
    rb->head = (rb->head + 1) % rb->size;
    rb->count--;
    return 0;
}

// Queue functions
kstat_t queue_create(kqueue_t *queue, const name_t name, void **buffer, size_t msg_num) {
    NULL_PARA_CHK(queue);
    NULL_PARA_CHK(buffer);
    NULL_PARA_CHK(name);

    if (msg_num == 0) {
        return RHINO_INV_PARAM;
    }

    memset(queue, 0, sizeof(kqueue_t));
    ring_buffer_init(&queue->ring_buf, buffer, msg_num);
    queue->size = msg_num;
    queue->name = name;
    return RHINO_SUCCESS;
}

kstat_t queue_send(kqueue_t *queue, void *msg) {
    NULL_PARA_CHK(queue);
    
    if (ring_buffer_push(&queue->ring_buf, msg) != 0) {
        return RHINO_INV_PARAM;  // Queue full
    }
    
    queue->cur_num = queue->ring_buf.count;
    if (queue->cur_num > queue->peak_num) {
        queue->peak_num = queue->cur_num;
    }
    return RHINO_SUCCESS;
}

kstat_t queue_receive(kqueue_t *queue, void **msg) {
    NULL_PARA_CHK(queue);
    NULL_PARA_CHK(msg);
    
    if (ring_buffer_pop(&queue->ring_buf, msg) != 0) {
        return RHINO_INV_PARAM;  // Queue empty
    }
    
    queue->cur_num = queue->ring_buf.count;
    return RHINO_SUCCESS;
}

int main() {
    // Test the queue implementation
    const size_t QUEUE_SIZE = 5;
    void *buffer[QUEUE_SIZE];
    kqueue_t queue;
    
    // Create queue
    printf("Creating queue...\n");
    if (queue_create(&queue, "test_queue", buffer, QUEUE_SIZE) != RHINO_SUCCESS) {
        printf("Failed to create queue\n");
        return 1;
    }
    
    // Test sending messages
    printf("\nTesting message sending...\n");
    int data1 = 1, data2 = 2, data3 = 3;
    if (queue_send(&queue, &data1) == RHINO_SUCCESS) {
        printf("Sent message: %d\n", data1);
    }
    if (queue_send(&queue, &data2) == RHINO_SUCCESS) {
        printf("Sent message: %d\n", data2);
    }
    if (queue_send(&queue, &data3) == RHINO_SUCCESS) {
        printf("Sent message: %d\n", data3);
    }
    
    // Test receiving messages
    printf("\nTesting message receiving...\n");
    void *received_msg;
    while (queue_receive(&queue, &received_msg) == RHINO_SUCCESS) {
        printf("Received message: %d\n", *(int*)received_msg);
    }
    
    printf("\nQueue test completed!\n");
    return 0;
}
