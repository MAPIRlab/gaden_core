#include "gaden/Preprocessing.hpp"
#include "gaden/internal/MathUtils.hpp"
#include "gaden/internal/STL.hpp"
#include "preprocessing/TriangleBoxIntersection.hpp"
#include <algorithm>
#include <queue>

namespace gaden
{

    Environment Preprocessing::ParseSTLModels(const std::vector<std::filesystem::path>& mainModels,
                                              const std::vector<std::filesystem::path>& outletModels,
                                              float cellSize,
                                              Vector3 emptyPoint)
    {
        // parse the files and get the dimensions of the environment
        //---------------------------------------
        BoundingBox boundingBox;

        std::vector<Triangle> allObstacleTriangles;
        std::vector<Triangle> allOutletTriangles;
        for (const auto& model : mainModels)
        {
            std::vector<Triangle> triangles = ParseSTLFile(model);
            std::move(triangles.begin(), triangles.end(), std::back_inserter(allObstacleTriangles));
        }

        for (const auto& model : outletModels)
        {
            std::vector<Triangle> triangles = ParseSTLFile(model);
            std::move(triangles.begin(), triangles.end(), std::back_inserter(allOutletTriangles));
        }

        boundingBox.Grow(findDimensions(allObstacleTriangles));
        boundingBox.Grow(findDimensions(allOutletTriangles));

        Vector3i dimensions = vmath::ceil((boundingBox.max - boundingBox.min) / cellSize);
        Environment environment{
            .versionMajor = gaden::versionMajor,
            .versionMinor = gaden::versionMinor,
            .description = Environment::Description{
                .dimensions = dimensions,
                .minCoord = boundingBox.min,
                .maxCoord = boundingBox.max,
                .cellSize = cellSize},
            .cells = std::vector<Environment::CellState>(dimensions.x * dimensions.y * dimensions.z, Environment::CellState::Uninitialized)};

        // fill in the environment cell grid
        //---------------------------------------
        Occupy(allObstacleTriangles, environment, Environment::CellState::Obstacle);
        Occupy(allOutletTriangles, environment, Environment::CellState::Outlet);

        Fill(environment, emptyPoint);
        return environment;
    }

    WindSequence Preprocessing::ParseOpenFoamVectorCloud(const std::vector<std::filesystem::path>& files,
                                                         const Environment& env,
                                                         LoopConfig loopConfig)
    {
        std::vector<std::vector<Vector3>> windIterations;
        windIterations.reserve(files.size());
        for (const auto& filename : files)
        {
            // let's parse the file
            std::ifstream infile(filename.c_str());

            // Depending on the verion of Paraview used to export the file, lines might be (Point, vector) OR (vector, Point)
            // so we need to check the header before we know where to put what
            std::string line;
            struct ParsedLine
            {
                float point[3];
                float windVector[3];
            };
            ParsedLine parsedLine;
            float* firstPartOfLine;
            float* secondPartOfLine;
            {
                std::getline(infile, line);
                size_t pos = line.find(",");
                std::string firstElement = line.substr(0, pos);

                if (firstElement.find("Points") != std::string::npos)
                {
                    firstPartOfLine = parsedLine.point;
                    secondPartOfLine = parsedLine.windVector;
                }
                else
                {
                    firstPartOfLine = parsedLine.windVector;
                    secondPartOfLine = parsedLine.point;
                }
            }

            std::vector<Vector3> wind(env.numCells(), Vector3(0, 0, 0));

            while (std::getline(infile, line))
            {
                if (line.length() != 0)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        size_t pos = line.find(",");
                        firstPartOfLine[i] = atof(line.substr(0, pos).c_str());
                        line.erase(0, pos + 1);
                    }

                    for (int i = 0; i < 3; i++)
                    {
                        size_t pos = line.find(",");
                        secondPartOfLine[i] = atof(line.substr(0, pos).c_str());
                        line.erase(0, pos + 1);
                    }

                    // assign each of the points we have information about to the nearest cell
                    Vector3i idx = env.coordsToIndices({parsedLine.point[0],
                                                        parsedLine.point[1],
                                                        parsedLine.point[2]});

                    size_t index3D = env.indexFrom3D(idx);
                    wind[index3D] = {parsedLine.windVector[0],
                                     parsedLine.windVector[1],
                                     parsedLine.windVector[2]};
                }
            }

            windIterations.push_back(wind);
            infile.close();
        }

        WindSequence sequence;
        sequence.Initialize(windIterations, env.numCells(), loopConfig);
        return sequence;
    }

    std::optional<EnvironmentConfiguration> Preprocessing::Preprocess(EnvironmentConfigMetadata const& metadata)
    {
        try
        {
            EnvironmentConfiguration config;
            std::vector<std::filesystem::path> envModels;
            for (auto& model : metadata.envModels)
                envModels.push_back(model.path);

            std::vector<std::filesystem::path> outletModels;
            for (auto& model : metadata.outletModels)
                outletModels.push_back(model.path);

            config.environment = ParseSTLModels(envModels, outletModels, metadata.cellSize, metadata.emptyPoint);

            if (metadata.uniformWind)
                config.windSequence = WindSequence::CreateUniformWind(metadata.GetWindFiles()[0], config.environment.numCells());
            else
                config.windSequence = ParseOpenFoamVectorCloud(metadata.GetWindFiles(), config.environment, {});
            return config;
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Exception while trying to run preprocessing: '{}'", e.what());
            return std::nullopt;
        }
    }

    Preprocessing::BoundingBox Preprocessing::findDimensions(const std::vector<Triangle>& triangles)
    {
        BoundingBox boundingBox;
        for (const auto& triangle : triangles)
        {
            boundingBox.Grow(triangle.p1);
            boundingBox.Grow(triangle.p2);
            boundingBox.Grow(triangle.p3);
        }
        return boundingBox;
    }

    void Preprocessing::Occupy(std::vector<Triangle>& triangles, Environment& env, Environment::CellState value_to_write)
    {
        int numberOfProcessedTriangles = 0; // for logging, doesn't actually do anything
        std::mutex mtx;
        // Let's occupy the enviroment!
#pragma omp parallel for
        for (int i = 0; i < triangles.size(); i++)
        {
            Triangle triangle = triangles.at(i);
            // We try to find all the cells that some triangle goes through
            Vector3i p1 = env.coordsToIndices(triangle.p1);
            Vector3i p2 = env.coordsToIndices(triangle.p2);
            Vector3i p3 = env.coordsToIndices(triangle.p3);

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
                            env.atRef(Vector3i{col, row, height}) = value_to_write;
                            mtx.unlock();
                        }
                    }
                }
            }

            // log progress
            // if (i > numberOfProcessedTriangles + triangles.size() / 10)
            // {
            //     mtx.lock();
            //     GADEN_INFO("{}%", (int)((100 * i) / triangles.size()));
            //     numberOfProcessedTriangles = i;
            //     mtx.unlock();
            // }
        }
    }

    void Preprocessing::Fill(Environment& environment, Vector3 emptyPoint)
    {
        // essentially a flood fill algorithm
        // start from emptyPoint, and replace any uninitialized cells you find with free ones
        // occupied cells block the propagation, so the only cells that will remain uninitialized at the end are the ones that are unreachable from the seed point
        // i.e. inside of obstacles
        std::queue<Vector3i> q;
        {
            Vector3i indices = environment.coordsToIndices(emptyPoint);

            q.emplace(indices);

            if (environment.at(indices) != Environment::CellState::Uninitialized)
                GADEN_ERROR("'Empty point' provided is corresponds to space that had been directly occupied by the mesh! This is almost certainly a mistake!");
            environment.atRef(indices) = Environment::CellState::Free;
        }

        while (!q.empty())
        {
            Vector3i oldPoint = q.front();
            q.pop();

            // if oldPoint+offset is non_initialized, set it to free and add its indices to the queue to keep propagating
            auto compareAndAdd = [&](Vector3i offset)
            {
                Vector3i currentPoint = oldPoint + offset;
                if (environment.at(currentPoint) == Environment::CellState::Uninitialized)
                {
                    environment.atRef(currentPoint) = Environment::CellState::Free;
                    q.emplace(currentPoint);
                }
            };

            compareAndAdd({1, 0, 0});
            compareAndAdd({-1, 0, 0});

            compareAndAdd({0, 1, 0});
            compareAndAdd({0, -1, 0});

            compareAndAdd({0, 0, 1});
            compareAndAdd({0, 0, -1});
        }

        // Done with the propagation! Mark anything still uninitialized as occupied
        for (auto& cell : environment.cells)
            if (cell == Environment::CellState::Uninitialized)
                cell = Environment::CellState::Obstacle;
    }

    void Preprocessing::BoundingBox::Grow(const Vector3& point)
    {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);

        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    void Preprocessing::BoundingBox::Grow(const Preprocessing::BoundingBox& other)
    {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);

        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }

} // namespace gaden