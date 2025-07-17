#include "gaden/core/Assertions.hpp"
#include "gaden/core/Logging.hpp"
#include "gaden/internal/Serialization.hpp"
#include <fstream>
#include <gaden/internal/compression.hpp>
#include <iostream>
#include <vector>

using namespace gaden;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        GADEN_ERROR("Correct format is \"decompress inputFile outputFile\"");
        GADEN_TERMINATE;
    }
    else
    {
        std::string input = argv[1];
        std::string output = argv[2];

        // calculate the buffer sizes and resize if needed
        std::vector<uint8_t> rawBuffer;
        std::vector<uint8_t> compressedBuffer;
        std::ifstream infile(input, std::ios_base::binary);
        bool isModernFile = serialization::IsModernResultsFile(infile);
        serialization::FileCompressionMetadata compressionMetadata = serialization::GetCompressionMetadata(infile);

        // figure out how the file was compressed. Old files always used DEFLATE (zlib), but new ones have an enum in the header
        serialization::SkipGadenHeader(infile);

        // size the buffers
        size_t compressedSize = serialization::RemainingFileSize(infile);
        // uncompressed files
        if (compressionMetadata.uncompressedSize == 0)
            compressionMetadata.uncompressedSize = compressedSize;

        if (compressedBuffer.size() < compressedSize)
        {
            size_t newSize = compressedSize * 1.5;
            compressedBuffer.resize(newSize);
            GADEN_INFO("Resizing compressedBuffer to {} bytes", newSize);
        }

        if (rawBuffer.size() < compressionMetadata.uncompressedSize)
        {
            size_t newSize = compressionMetadata.uncompressedSize * 1.5;
            rawBuffer.resize(newSize);
            GADEN_INFO("Resizing rawBuffer to {} bytes", newSize);
        }

        // read the file
        // the stream position is set to the start of the compressed area (even in modern files with an uncompressed header)
        infile.read((char*)compressedBuffer.data(), compressedSize);
        infile.close();

        // decompress the contents
        if (compressionMetadata.mode == serialization::CompressionMode::LIBBSC)
            LibBSC::Decompress(compressedBuffer.data(), rawBuffer.data());
        else if (compressionMetadata.mode == serialization::CompressionMode::ZLIB)
            zlib::uncompress(rawBuffer.data(), &compressionMetadata.uncompressedSize, compressedBuffer.data(), compressedBuffer.size());
        else
            memcpy(rawBuffer.data(), compressedBuffer.data(), compressionMetadata.uncompressedSize);

        // write out
        std::ofstream outfile(output);
        outfile.write((char*)rawBuffer.data(), compressionMetadata.uncompressedSize);
    }
}
