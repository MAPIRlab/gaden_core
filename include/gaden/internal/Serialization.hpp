#pragma once
#include <fstream>
#include <gaden/internal/compression.hpp>
#include <string.h>
#include <string>
#include <vector>

namespace gaden
{
    namespace serialization
    {

        // maximum size that the data can take pre-compression. We fix this here so that the PlaybackSimulation can know the upper bound of the uncompressed size and allocate its buffer accordingly
        inline constexpr size_t defaultBufferSize = 5e6;

        // used in the uncompressed header for the result files produced by RunningSimulation (after 3.0)
        // format of the header is: 
        // ID[13] CompressionMode[8] UncompressedSize[64]
        inline const char resultIdentifier[] = "GADEN_RESULT";

        // written right after the header in post-3.0 files
        enum class CompressionMode : uint8_t
        {
            UNCOMPRESSED = 0,
            ZLIB = 1,
            LIBBSC = 2
        };

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
                static_assert(std::is_trivially_copyable_v<T>,
                              "Type is not trivially copyable! If your type contains dynamic memory (strings, vectors, etc) you must explicitly define how to serialize it");

                Write(address, sizeof(T));
            }

            void WriteString(const std::string* address)
            {
                size_t size = address->length();
                Write(&size);
                Write(address->data(), size);
            }

            template <typename T>
            void WriteVector(const std::vector<T>* address)
            {
                size_t size = address->size();
                Write(&size);
                Write(address->data(), size * sizeof(T));
            }

            void Write(const void* address, size_t size)
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
                static_assert(std::is_trivially_copyable_v<T>,
                              "Type is not trivially copyable! If your type contains dynamic memory (strings, vectors, etc) you must explicitly define how to serialize it");

                Read(address, sizeof(T));
            }

            void Read(void* address, size_t size)
            {
                memcpy(address, current, size);
                current += size;
            }

            void ReadString(std::string* address)
            {
                size_t size;
                Read(&size);
                address->resize(size);
                Read(address->data(), size);
            }

            template <typename T>
            void ReadVector(std::vector<T>* address)
            {
                size_t size;
                Read(&size);
                address->resize(size);
                Read(address->data(), size * sizeof(T));
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

        inline size_t TotalFileSize(std::ifstream& fileStream)
        {
            size_t pos = fileStream.tellg();
            fileStream.seekg(0, std::ios_base::end);
            size_t size = fileStream.tellg();

            // restore the stream position
            fileStream.seekg(pos, std::ios_base::beg);
            return size;
        }

        inline size_t RemainingFileSize(std::ifstream& fileStream)
        {
            size_t pos = fileStream.tellg();
            fileStream.seekg(0, std::ios_base::end);
            size_t size = (size_t)fileStream.tellg() - pos;

            // restore the stream position
            fileStream.seekg(pos, std::ios_base::beg);
            return size;
        }

        // check if the file starts with the correct header
        inline bool IsModernResultsFile(std::ifstream& fileStream)
        {
            size_t size = TotalFileSize(fileStream);
            if (size < sizeof(resultIdentifier))
                return false;
            size_t pos = fileStream.tellg();

            // reade the data from the file
            fileStream.seekg(0, std::ios_base::beg);
            std::string contents;
            contents.resize(sizeof(resultIdentifier) - 1); // minus the null termination
            fileStream.read(contents.data(), contents.length());

            // return the stream read position to where it was
            fileStream.seekg(pos);

            return contents == resultIdentifier;
        }

        inline void SkipGadenHeader(std::ifstream& fileStream)
        {
            if (IsModernResultsFile(fileStream))
            {
                fileStream.seekg(0);
                fileStream.seekg(sizeof(resultIdentifier),std::ios_base::cur);
                fileStream.seekg(sizeof(CompressionMode),std::ios_base::cur);
                fileStream.seekg(sizeof(size_t),std::ios_base::cur);
            }
        }

        struct FileCompressionMetadata
        {
            CompressionMode mode;
            size_t uncompressedSize;
        };

        // modern files contain this data. Old files do not, so this will just return the default max buffer size (which, to be fair, should always be enough)
        inline FileCompressionMetadata GetCompressionMetadata(std::ifstream& fileStream)
        {
            if (!IsModernResultsFile(fileStream))
                return {CompressionMode::ZLIB, defaultBufferSize};

            size_t pos = fileStream.tellg();
            fileStream.seekg(0);

            fileStream.seekg(sizeof(resultIdentifier));

            CompressionMode mode;
            fileStream.read((char*)&mode, sizeof(mode));

            size_t size;
            fileStream.read((char*)&size, sizeof(size_t));
            fileStream.seekg(pos);

            return {mode, size};
        }

    } // namespace serialization
} // namespace gaden
