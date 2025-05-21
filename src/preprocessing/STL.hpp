#pragma once

#include "gaden/internal/Triangle.hpp"
#include <filesystem>
#include <fstream>

namespace gaden
{

    inline bool isASCII(const std::string& filename)
    {
        bool ascii = false;
        if (FILE* file = fopen(filename.c_str(), "r"))
        {
            // File exists!, keep going!
            char buffer[6];
            char* result = fgets(buffer, 6, file);

            if (std::string(buffer).find("solid") != std::string::npos)
                ascii = true;
            fclose(file);
        }
        else
        {
            GADEN_ERROR("File '{}' does not exist\n", filename.c_str());
        }
        return ascii;
    }

    inline std::vector<Triangle> ParseSTLFile(const std::filesystem::path& path)
    {
        bool ascii = isASCII(path);

        std::vector<Triangle> triangles;
        std::vector<gaden::Vector3> normals;

        if (ascii)
        {
            // first, we count how many triangles there are (we need to do this before reading the data
            //  to create a vector of the right size)
            std::ifstream infile(path.c_str());
            std::string line;
            int count = 0;

            while (std::getline(infile, line))
            {
                if (line.find("facet normal") != std::string::npos)
                {
                    count++;
                }
            }
            infile.seekg(0, std::ios_base::beg);

            // each points[i] contains one the three vertices of triangle i
            triangles.resize(count);
            normals.resize(count);
            // let's read the data
            std::getline(infile, line);
            int i = 0;
            while (line.find("endsolid") == std::string::npos)
            {
                while (line.find("facet normal") == std::string::npos)
                {
                    std::getline(infile, line);
                }
                size_t pos = line.find("facet");
                line.erase(0, pos + 12);
                float aux;
                std::stringstream ss(line);
                ss >> std::skipws >> aux;
                normals[i].x = aux;
                ss >> std::skipws >> aux;
                normals[i].y = aux;
                ss >> std::skipws >> aux;
                normals[i].z = aux;
                std::getline(infile, line);

                for (int j = 0; j < 3; j++)
                {
                    std::getline(infile, line);
                    size_t pos = line.find("vertex ");
                    line.erase(0, pos + 7);
                    std::stringstream ss(line);
                    ss >> std::skipws >> aux;
                    triangles[i][j].x = aux;
                    ss >> std::skipws >> aux;
                    triangles[i][j].y = aux;
                    ss >> std::skipws >> aux;
                    triangles[i][j].z = aux;
                }
                i++;
                // skipping lines here makes checking for the end of the file more convenient
                std::getline(infile, line);
                std::getline(infile, line);
                while (std::getline(infile, line) && line.length() == 0)
                    ;
            }
            infile.close();
        }
        else
        {
            std::ifstream infile(path.c_str(), std::ios_base::binary);
            infile.seekg(80 * sizeof(uint8_t), std::ios_base::cur); // skip the header
            uint32_t num_triangles;
            infile.read((char*)&num_triangles, sizeof(uint32_t));
            triangles.resize(num_triangles);
            normals.resize(num_triangles);

            for (int i = 0; i < num_triangles; i++)
            {
                infile.read((char*)&normals[i], 3 * sizeof(uint32_t)); // read the normal vector

                for (int j = 0; j < 3; j++)
                {
                    std::array<float, 3> vec;
                    infile.read((char*)&vec, 3 * sizeof(uint32_t)); // read the point
                    triangles[i][j].x = (vec[0]);
                    triangles[i][j].y = (vec[1]);
                    triangles[i][j].z = (vec[2]);
                }

                infile.seekg(sizeof(uint16_t), std::ios_base::cur); // skip the attribute data
            }
            infile.close();
        }
        return triangles;
    }
} // namespace gaden