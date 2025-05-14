#include <stdio.h>

/* start ring buffer */
#define BUFFER_SIZE 16
// Circular buffer structure
typedef struct {
    char data[BUFFER_SIZE];
    volatile int head;     // Index for reading (consumer)
    volatile int tail;     // Index for writing (producer)
    volatile int count;    // Number of items in the buffer
} RingBuffer;
RingBuffer bufc;

// Initialize the buffer
void init_ring_buffer(RingBuffer *buffer) {
    printf("init_buffer\n");
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

// Add a character to the buffer (called from interrupt handler)
void append_char(RingBuffer *buffer, char c) {
    //printf("> appendchar\n");
    // If buffer is full, overwrite oldest data
    if (buffer->count == BUFFER_SIZE) {
        printf("> appendchar buffer overflow\n");
        buffer->head = (buffer->head + 1) % BUFFER_SIZE; // Move head forward
        buffer->count--; // Make room for new char
    }

    // Add the new character
    buffer->data[buffer->tail] = c;
    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    buffer->count++;
}

bool has_next_char(RingBuffer *buffer) {
    return (buffer->count != 0);
}

// Get the character at the head of the buffer (busy-waits if empty)
char get_next_char(RingBuffer *buffer) {
    // Wait until there's at least one character
    if (buffer->count == 0) { return -1; }

    // Get the character
    char c = buffer->data[buffer->head];
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->count--;

    return c;
}
/* end ring buffer */
