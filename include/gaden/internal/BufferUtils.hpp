#pragma once
#include <string.h>

namespace gaden
{
    // maximum size that the data can take pre-compression. We fix this here so that the PlaybackSimulation can know the upper bound of the uncompressed size and allocate its buffer accordingly
    inline constexpr size_t maxBufferSize = 5e6;

    class BufferWriter
    {
    public:
        BufferWriter() = delete;
        BufferWriter(char* _start, size_t _size)
        {
            start = _start;
            current = start;
            end = start + _size;
        }

        template <typename T>
        void Write(T* address)
        {
            Write(address, sizeof(T));
        }

        template <typename T>
        void Write(T* address, size_t size)
        {
            memcpy(current, address, size);
            current += size;
        }

        size_t currentOffset()
        {
            return current - start;
        }

    private:
        char* start;
        char* current;
        char* end;
    };

    class BufferReader
    {
    public:
        BufferReader() = delete;
        BufferReader(char* _start, size_t _size)
        {
            start = _start;
            current = start;
            end = start + _size;
        }

        template <typename T>
        void Read(T* address)
        {
            Read(address, sizeof(T));
        }

        template <typename T>
        void Read(T* address, size_t size)
        {
            memcpy(address, current, size);
            current += size;
        }

        size_t currentOffset()
        {
            return current - start;
        }

        void AdvancePointer(size_t offset)
        {
            current += offset;
        }

        bool Ended()
        {
            return current == end;
        }

    private:
        char* start;
        char* current;
        char* end;
    };
} // namespace gaden
