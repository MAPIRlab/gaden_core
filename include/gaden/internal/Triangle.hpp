#pragma once

#include "gaden/core/Logging.hpp"
#include "gaden/core/Vectors.hpp"
namespace gaden
{

    struct Triangle
    {
        Vector3 p1;
        Vector3 p2;
        Vector3 p3;

        Triangle()
        {}
        Triangle(const Vector3& p1, const Vector3& p2, const Vector3& p3)
        {
            this->p1 = p1;
            this->p2 = p2;
            this->p3 = p3;
        }

        Vector3& operator[](int i)
        {
            if (i == 0)
                return p1;
            else if (i == 1)
                return p2;
            else if (i == 2)
                return p3;
            else
            {
                GADEN_ERROR("Indexing error when accessing the gaden::Vector3s in triangle! Index must be >= 2");
                return p1;
            }
        }

        Vector3 normal() const
        {
            return vmath::cross(p1-p2, p1-p3);
        }
    };
} // namespace gaden