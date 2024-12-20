#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Return status codes
#define RHINO_SUCCESS        0
#define RHINO_RINGBUF_FULL  1
#define RHINO_INV_PARAM     2

// Ring buffer types
#define RINGBUF_TYPE_DYN    0    // Dynamic size items
#define RINGBUF_TYPE_FIX    1    // Fixed size items

#define RING_BUF_LEN sizeof(size_t)

typedef int kstat_t;

// Ring buffer structure
typedef struct {
    uint8_t *buf;          // Start of buffer
    uint8_t *end;          // End of buffer
    uint8_t *head;         // Read pointer
    uint8_t *tail;         // Write pointer
    size_t  freesize;      // Available space in buffer
    size_t  type;          // Buffer type (FIX/DYN)
    size_t  blk_size;      // Block size for fixed type
} k_ringbuf_t;

// Initialize ring buffer
kstat_t ringbuf_init(k_ringbuf_t *p_ringbuf, void *buf, size_t len, size_t type, size_t block_size)
{
    p_ringbuf->type = type;
    p_ringbuf->buf = buf;
    p_ringbuf->end = (uint8_t *)buf + len;
    p_ringbuf->blk_size = block_size;
    p_ringbuf->head = p_ringbuf->buf;
    p_ringbuf->tail = p_ringbuf->buf;
    p_ringbuf->freesize = len;
    
    return RHINO_SUCCESS;
}

// Push data into ring buffer
kstat_t ringbuf_push(k_ringbuf_t *p_ringbuf, void *data, size_t len)
{
    size_t len_bytes = 0;
    size_t split_len = 0;
    uint8_t c_len[RING_BUF_LEN] = {0};

    if (p_ringbuf->type == RINGBUF_TYPE_FIX) {
        if (p_ringbuf->freesize < p_ringbuf->blk_size) {
            return RHINO_RINGBUF_FULL;
        }

        if (p_ringbuf->tail == p_ringbuf->end) {
            p_ringbuf->tail = p_ringbuf->buf;
        }

        memcpy(p_ringbuf->tail, data, p_ringbuf->blk_size);
        p_ringbuf->tail += p_ringbuf->blk_size;
        p_ringbuf->freesize -= p_ringbuf->blk_size;
    } else {
        if ((len == 0u) || (len >= (uint32_t)-1)) {
            return RHINO_INV_PARAM;
        }

        len_bytes = RING_BUF_LEN;

        if (p_ringbuf->freesize < (len_bytes + len)) {
            return RHINO_RINGBUF_FULL;
        }

        memcpy(c_len, &len, RING_BUF_LEN);

        if (p_ringbuf->tail == p_ringbuf->end) {
            p_ringbuf->tail = p_ringbuf->buf;
        }

        // Copy length data to buffer
        split_len = p_ringbuf->end - p_ringbuf->tail;
        if (p_ringbuf->tail >= p_ringbuf->head && split_len < len_bytes && split_len > 0) {
            memcpy(p_ringbuf->tail, &c_len[0], split_len);
            len_bytes -= split_len;
            p_ringbuf->tail = p_ringbuf->buf;
            p_ringbuf->freesize -= split_len;
        }

        if (len_bytes > 0) {
            memcpy(p_ringbuf->tail, &c_len[split_len], len_bytes);
            p_ringbuf->freesize -= len_bytes;
            p_ringbuf->tail += len_bytes;
        }

        // Copy actual data
        if (p_ringbuf->tail == p_ringbuf->end) {
            p_ringbuf->tail = p_ringbuf->buf;
        }

        split_len = p_ringbuf->end - p_ringbuf->tail;
        if (p_ringbuf->tail >= p_ringbuf->head && split_len < len && split_len > 0) {
            memcpy(p_ringbuf->tail, data, split_len);
            data = (uint8_t *)data + split_len;
            len -= split_len;
            p_ringbuf->tail = p_ringbuf->buf;
            p_ringbuf->freesize -= split_len;
        }

        memcpy(p_ringbuf->tail, data, len);
        p_ringbuf->tail += len;
        p_ringbuf->freesize -= len;
    }

    return RHINO_SUCCESS;
}

// Pop data from ring buffer
kstat_t ringbuf_pop(k_ringbuf_t *p_ringbuf, void *pdata, size_t *plen)
{
    size_t len_bytes = 0;
    size_t len = 0;
    size_t split_len = 0;
    uint8_t c_len[RING_BUF_LEN] = {0};

    if (p_ringbuf->head == p_ringbuf->tail) {
        return RHINO_RINGBUF_FULL;
    }

    if (p_ringbuf->type == RINGBUF_TYPE_FIX) {
        if (p_ringbuf->head == p_ringbuf->end) {
            p_ringbuf->head = p_ringbuf->buf;
        }

        memcpy(pdata, p_ringbuf->head, p_ringbuf->blk_size);
        p_ringbuf->head += p_ringbuf->blk_size;
        p_ringbuf->freesize += p_ringbuf->blk_size;
        *plen = p_ringbuf->blk_size;
    } else {
        len_bytes = RING_BUF_LEN;

        if (p_ringbuf->head == p_ringbuf->end) {
            p_ringbuf->head = p_ringbuf->buf;
        }

        split_len = p_ringbuf->end - p_ringbuf->head;
        if (split_len < len_bytes && split_len > 0) {
            memcpy(&c_len[0], p_ringbuf->head, split_len);
            len_bytes -= split_len;
            p_ringbuf->head = p_ringbuf->buf;
            p_ringbuf->freesize += split_len;
        }

        if (len_bytes > 0) {
            memcpy(&c_len[split_len], p_ringbuf->head, len_bytes);
            p_ringbuf->head += len_bytes;
            p_ringbuf->freesize += len_bytes;
        }

        memcpy(&len, c_len, RING_BUF_LEN);
        if (len == 0 || len >= (uint32_t)-1) {
            return RHINO_INV_PARAM;
        }

        *plen = len;

        // Get data
        if (p_ringbuf->head == p_ringbuf->end) {
            p_ringbuf->head = p_ringbuf->buf;
        }

        split_len = p_ringbuf->end - p_ringbuf->head;
        if (split_len < len && split_len > 0) {
            memcpy(pdata, p_ringbuf->head, split_len);
            pdata = (uint8_t *)pdata + split_len;
            len -= split_len;
            p_ringbuf->head = p_ringbuf->buf;
            p_ringbuf->freesize += split_len;
        }

        memcpy(pdata, p_ringbuf->head, len);
        p_ringbuf->head += len;
        p_ringbuf->freesize += len;
    }

    return RHINO_SUCCESS;
}

// Check if ring buffer is empty
uint8_t ringbuf_is_empty(k_ringbuf_t *p_ringbuf)
{
    return (p_ringbuf->head == p_ringbuf->tail);
}

// Reset ring buffer
kstat_t ringbuf_reset(k_ringbuf_t *p_ringbuf)
{
    p_ringbuf->head = p_ringbuf->buf;
    p_ringbuf->tail = p_ringbuf->buf;
    p_ringbuf->freesize = p_ringbuf->end - p_ringbuf->buf;
    return RHINO_SUCCESS;
}

// Test function for fixed-size ring buffer
void test_fixed_ringbuf(void)
{
    printf("\nTesting Fixed-Size Ring Buffer:\n");
    printf("--------------------------------\n");

    // Create buffer
    const size_t BUF_SIZE = 20;
    const size_t BLOCK_SIZE = 4;
    uint8_t buffer[BUF_SIZE];
    k_ringbuf_t ringbuf;

    // Initialize ring buffer
    ringbuf_init(&ringbuf, buffer, BUF_SIZE, RINGBUF_TYPE_FIX, BLOCK_SIZE);
    
    // Test data
    int test_data[] = {1234, 5678, 9012, 3456, 7890};
    size_t len;
    int read_data;

    // Push data
    printf("Pushing data: ");
    for (int i = 0; i < 5; i++) {
        if (ringbuf_push(&ringbuf, &test_data[i], BLOCK_SIZE) == RHINO_SUCCESS) {
            printf("%d ", test_data[i]);
        } else {
            printf("\nBuffer full at %d\n", i);
            break;
        }
    }
    printf("\n");

    // Pop data
    printf("Popping data: ");
    while (ringbuf_pop(&ringbuf, &read_data, &len) == RHINO_SUCCESS) {
        printf("%d ", read_data);
    }
    printf("\n");
}

// Test function for dynamic-size ring buffer
void test_dynamic_ringbuf(void)
{
    printf("\nTesting Dynamic-Size Ring Buffer:\n");
    printf("----------------------------------\n");

    // Create buffer
    const size_t BUF_SIZE = 100;
    uint8_t buffer[BUF_SIZE];
    k_ringbuf_t ringbuf;

    // Initialize ring buffer
    ringbuf_init(&ringbuf, buffer, BUF_SIZE, RINGBUF_TYPE_DYN, 0);
    
    // Test strings
    const char *test_strings[] = {"Hello", "World", "Ring", "Buffer", "Test"};
    size_t len;
    char read_buffer[20];

    // Push strings
    printf("Pushing strings: ");
    for (int i = 0; i < 5; i++) {
        size_t str_len = strlen(test_strings[i]) + 1;  // Include null terminator
        if (ringbuf_push(&ringbuf, (void*)test_strings[i], str_len) == RHINO_SUCCESS) {
            printf("%s ", test_strings[i]);
        } else {
            printf("\nBuffer full at %d\n", i);
            break;
        }
    }
    printf("\n");

    // Pop strings
    printf("Popping strings: ");
    while (ringbuf_pop(&ringbuf, read_buffer, &len) == RHINO_SUCCESS) {
        printf("%s ", read_buffer);
    }
    printf("\n");
}

int main()
{
    // Test both fixed and dynamic ring buffers
    test_fixed_ringbuf();
    test_dynamic_ringbuf();
    
    return 0;
}
