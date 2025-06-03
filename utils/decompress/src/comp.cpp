#include "gaden/core/Assertions.hpp"
#include "gaden/core/Logging.hpp"
#include "gaden/internal/BufferUtils.hpp"
#include <fstream>
#include <gaden/internal/compression.hpp>
#include <iostream>
#include <vector>

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

        // get size
        std::ifstream infile(input, std::ios_base::binary | std::ios_base::ate);
        size_t streamSize = infile.tellg();
        infile.seekg(0, std::ios_base::beg);

        // read the compressed data to a buffer
        std::vector<uint8_t> compressedBuffer(gaden::maxBufferSize);
        infile.read((char*)compressedBuffer.data(), streamSize);
        infile.close();

        // decompress the contents
        std::vector<uint8_t> rawBuffer(gaden::maxBufferSize);
        zlib::uLongf bufferSize = rawBuffer.size();
        zlib::uncompress(rawBuffer.data(), &bufferSize, compressedBuffer.data(), compressedBuffer.size());

        // write out
        std::ofstream outfile(output);
        outfile.write((char*)rawBuffer.data(), bufferSize);
    }
}
