#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

// 用于操作缓冲区的 DSL

typedef struct {
    uint8_t* buffer;
    size_t position;
    size_t capacity;
    size_t written_count;
} BufferWriter;

int BufferWriter_init(BufferWriter* writer, void* buffer, size_t capacity);
int BufferWriter_write(BufferWriter* writer, const void* buf, size_t size);
int BufferWriter_write_char(BufferWriter* writer, char ch);

size_t BufferWriter_advance(BufferWriter* writer, size_t size);

inline size_t BufferWriter_available(BufferWriter* writer) { return writer->capacity - writer->written_count; }

inline uint8_t* BufferWriter_available_buffer(BufferWriter* writer) { return writer->buffer + writer->position; }

inline void BufferWriter_clear(BufferWriter* writer)
{
    writer->position = 0;
    writer->written_count = 0;
}

#ifdef __cplusplus
}
#endif
