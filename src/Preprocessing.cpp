#include "gaden/Preprocessing.hpp"
#include "gaden/internal/Utils.hpp"
#include "preprocessing/STL.hpp"
#include "preprocessing/TriangleBoxIntersection.hpp"

namespace gaden
{

    Environment Preprocessing::ParseSTLModels(const std::vector<std::filesystem::path>& mainModels,
                                              const std::vector<std::filesystem::path>& outletModels)
    {
        // get the dimensions of the environment
        //---------------------------------------
        BoundingBox boundingBox{.min = {FLT_MAX, FLT_MAX, FLT_MAX},
                                .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        for (const auto& model : mainModels)
            boundingBox.Grow(findDimensions(model));

        for (const auto& model : outletModels)
            boundingBox.Grow(findDimensions(model));

        // parse the files
        //---------------------------------------
        for (const auto& model : mainModels)
        {
        }
        for (const auto& model : outletModels)
        {
        }
    }

    WindSequence Preprocessing::ParseOpenFoamVectorCloud(const std::vector<std::filesystem::path>& files)
    {
    }

    Preprocessing::BoundingBox Preprocessing::findDimensions(const std::filesystem::path& model)
    {
    }

    void Preprocessing::occupy(std::vector<Triangle>& triangles, Environment& env, Environment::CellState value_to_write)
    {
        int numberOfProcessedTriangles = 0; // for logging, doesn't actually do anything
        std::mutex mtx;
        // Let's occupy the enviroment!
#pragma omp parallel for
        for (int i = 0; i < triangles.size(); i++)
        {
            Triangle triangle = triangles.at(i);
            // We try to find all the cells that some triangle goes through
            Vector3i p1 = triangle.p1 - env.description.minCoord / env.description.cellSize;
            Vector3i p2 = triangle.p2 - env.description.minCoord / env.description.cellSize;
            Vector3i p3 = triangle.p3 - env.description.minCoord / env.description.cellSize;

            // triangle Bounding Box
            BoundingBox boundingBox{.min = p1, .max = p1};
            boundingBox.Grow(p2);
            boundingBox.Grow(p3);

            // we have a special check for triangles that are perfectly aligned with the axes planes
            // this is because the overlap test can fail (due to numerical issues) if that's the case and the triangle is right at the limit of the cell
            Vector3 normal = triangle.normal();
            bool isParallel = (Approx(normal.y, 0) && Approx(normal.z, 0)) ||
                              (Approx(normal.x, 0) && Approx(normal.z, 0)) ||
                              (Approx(normal.x, 0) && Approx(normal.y, 0));

            // run over the list of cells in the BB and check for actual intersections
            for (size_t row = boundingBox.min.y; row <= boundingBox.max.y && row < env.description.dimensions.y; row++)
            {
                for (size_t col = boundingBox.min.x; col <= boundingBox.max.x && col < env.description.dimensions.x; col++)
                {
                    for (size_t height = boundingBox.min.z; height <= boundingBox.max.z && height < env.description.dimensions.z; height++)
                    {
                        // check if the triangle goes through this cell
                        // special case for triangles that are parallel to the coordinate axes because the discretization can cause
                        // problems if they fall right on the boundary of two cells
                        Vector3 cellCenter = env.coordsOfCellCenter({col, row, height});
                        float halfCellSize = env.description.cellSize * 0.5;

                        if ((isParallel && pointInTriangle(cellCenter, triangles[i], halfCellSize)) //
                            || triBoxOverlap(cellCenter, triangles[i], halfCellSize))
                        {
                            mtx.lock();
                            env.at(Vector3i{col, row, height}) = value_to_write; 
                            mtx.unlock();
                        }
                    }
                }
            }

            // log progress
            if (i > numberOfProcessedTriangles + triangles.size() / 10)
            {
                mtx.lock();
                GADEN_INFO("{}%", (int)((100 * i) / triangles.size()));
                numberOfProcessedTriangles = i;
                mtx.unlock();
            }
        }
    }

    void Preprocessing::BoundingBox::Grow(const Vector3& point)
    {
        min.x = std::min(min.x, point.x);
        min.x = std::min(min.y, point.y);
        min.x = std::min(min.z, point.z);
    }

    void Preprocessing::BoundingBox::Grow(const Preprocessing::BoundingBox& other)
    {
        Grow(other.min);
        Grow(other.max);
    }

} // namespace gaden