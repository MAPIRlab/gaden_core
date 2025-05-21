#pragma once
#include <string.h>

namespace gaden
{

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
} // namespace gaden
