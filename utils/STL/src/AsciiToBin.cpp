#include <gaden/internal/STL.hpp>

int main(int argc, char** argv)
{
    auto triangles = gaden::ParseSTLFile(argv[1]);
    gaden::WriteBinarySTL(argv[1], triangles);
    GADEN_INFO("Done!");
    return 0;
}