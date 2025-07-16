#pragma once
#include "gaden/core/Logging.hpp"
#include <cstddef> // IWYU pragma: keep
#include <cstdint>
#include <fstream>

namespace zlib
{
#include <zlib.h>
}

#include <libbsc/libbsc.h>

class LibBSC
{
public:
    static inline bool ValidateResult(int result)
    {
        if (result < LIBBSC_NO_ERROR)
        {
            GADEN_ERROR("LibBSC error:");
            switch (result)
            {
            case LIBBSC_BAD_PARAMETER:
                GADEN_ERROR("\tBad parameter! Please check README file for more information.");
                break;
            case LIBBSC_NOT_ENOUGH_MEMORY:
                GADEN_ERROR("\tNot enough memory! Please check README file for more information.");
                break;
            case LIBBSC_NOT_SUPPORTED:
                GADEN_ERROR("\tSpecified compression method is not supported on this platform!");
                break;
            case LIBBSC_GPU_ERROR:
                GADEN_ERROR("\tGeneral GPU failure! Please check README file for more information.");
                break;
            case LIBBSC_GPU_NOT_SUPPORTED:
                GADEN_ERROR("\tYour GPU is not supported! Please check README file for more information.");
                break;
            case LIBBSC_GPU_NOT_ENOUGH_MEMORY:
                GADEN_ERROR("\tNot enough GPU memory! Please check README file for more information.");
                break;

            default:
                GADEN_ERROR("Internal program error, please contact the author!");
            }
            return false;
        }
        else
            return true;
    }

    static inline int Compress(const uint8_t* input, size_t inputSize, uint8_t* output)
    {
        if (!initialized)
        {
            bsc_init(features);
            initialized = true;
        }

        int outputSize = bsc_compress(input, output, inputSize, LZPHashSize, LZPMinLen, LIBBSC_BLOCKSORTER_ST3, LIBBSC_CODER_QLFC_STATIC, features);
        ValidateResult(outputSize);

        return outputSize;
    }

    static inline bool Decompress(const uint8_t* input, uint8_t* output)
    {
        if (!initialized)
        {
            bsc_init(features);
            initialized = true;
        }

        int inputSize = 0;
        int outputSize = 0;
        int result = bsc_block_info(input, LIBBSC_HEADER_SIZE, &inputSize, &outputSize, features);
        if (!ValidateResult(result))
            return false;

        result = bsc_decompress(input, inputSize, output, outputSize, features);

        return ValidateResult(result);
    }

    static int DecompressedSize(std::ifstream& fileStream)
    {
        uint8_t header[LIBBSC_HEADER_SIZE];
        size_t pos = fileStream.tellg();
        fileStream.read((char*)header, LIBBSC_HEADER_SIZE);
        fileStream.seekg(pos);

        int inputSize = 0;
        int outputSize = 0;
        int result = bsc_block_info(header, LIBBSC_HEADER_SIZE, &inputSize, &outputSize, features);
        ValidateResult(result);
        return outputSize;
    }

private:
    static inline bool initialized = false;

    static constexpr int LZPHashSize = 15;
    static constexpr int LZPMinLen = 128;
    static constexpr int features = LIBBSC_FEATURE_FASTMODE | LIBBSC_FEATURE_MULTITHREADING;
};
