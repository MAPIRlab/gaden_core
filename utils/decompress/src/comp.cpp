#include "gaden/core/Assertions.hpp"
#include "gaden/core/Logging.hpp"
#include "gaden/internal/Serialization.hpp"
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

        // calculate the buffer sizes and resize if needed
        std::ifstream infile(input, std::ios_base::binary | std::ios_base::ate);
        size_t uncompressedSize = gaden::serialization::SizeRequiredToUncompress(infile);
        size_t compressedSize = gaden::serialization::RemainingFileSize(infile);

        std::vector<uint8_t> rawBuffer(uncompressedSize);
        std::vector<uint8_t> compressedBuffer(compressedSize);

        // read the file
        // the stream position is set to the start of the compressed area (even in modern files with an uncompressed header) by SizeRequiredToUncompress()
        infile.read((char*)compressedBuffer.data(), compressedSize);
        infile.close();

        // decompress the contents
        zlib::uLongf bufferSize = rawBuffer.size();
        zlib::uncompress(rawBuffer.data(), &uncompressedSize, compressedBuffer.data(), compressedBuffer.size());
        
        // write out
        std::ofstream outfile(output);
        outfile.write((char*)rawBuffer.data(), bufferSize);
    }
}
