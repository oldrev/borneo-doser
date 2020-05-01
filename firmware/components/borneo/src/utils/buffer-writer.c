#include <memory.h>
#include <assert.h>

#include "borneo/common.h"
#include "borneo/utils/buffer-writer.h"

int BufferWriter_init(BufferWriter* writer, void* buffer, size_t capacity)
{
    assert(writer != NULL);
    assert(buffer != NULL);
    assert(capacity > 0);

    memset(writer, 0, sizeof(BufferWriter));
    writer->buffer = (uint8_t*)buffer;
    writer->capacity = capacity;
    return 0;
}

int BufferWriter_write(BufferWriter* writer, const void* buf, size_t size)
{
    assert(writer != NULL);
    assert(buf != NULL);

    if (size == 0) {
        return 0;
    }

    if (size > (writer->capacity - writer->written_count)) {
        return -1;
    }

    memcpy(&writer->buffer[writer->position], buf, size);

    writer->position += size;
    writer->written_count += size;
    return size;
}

int BufferWriter_write_char(BufferWriter* writer, char ch)
{
    assert(writer != NULL);

    if ((writer->capacity - writer->written_count) < 1) {
        return -1;
    }
    writer->buffer[writer->position] = (uint8_t)ch;
    writer->position++;
    writer->written_count++;
    return 1;
}

size_t BufferWriter_advance(BufferWriter* writer, size_t size)
{
    assert(writer != NULL);

    if ((writer->capacity - writer->written_count) < size) {
        return -1;
    }
    writer->position += size;
    writer->written_count += size;
    return size;
}