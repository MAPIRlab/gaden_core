#pragma once

#include "gaden/Environment.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/internal/MathUtils.hpp"

namespace gaden::Airflow
{
    // from https://ieeexplore.ieee.org/document/10804051
    //------------------------------------------------------------
    class QuadrotorDisturbance
    {
        //  calculates the airflow velocity caused by a hovering quadrotor at point (r,s)
        //  r : radial distance
        //  s : flow-direction distance
        //  l : motor distance
    public:
        static void ModifyField(Vector3 dronePosition,
                                std::vector<Vector3>& airflowField,
                                Environment env,
                                float motorDistance,
                                float droneMass,
                                float rotorRadius,
                                float pressure,
                                float temperature)
        {
#pragma omp parallel for
            for (size_t i = 0; i < airflowField.size(); i++)
            {
                Vector3 coordsPoint = env.coordsOfCellCenter(env.indicesFrom1D(i));
                Vector3 relativePosition = coordsPoint - dronePosition;
                float s = -relativePosition.z;
                float r = vmath::length(Vector2(relativePosition.x, relativePosition.y));

                airflowField.at(i) = Speed(r, s, motorDistance, droneMass, rotorRadius, pressure, temperature) * vmath::normalized(relativePosition);
            }
        }

        static float Speed(float r, float s,
                              float motorDistance,
                              float droneMass,
                              float rotorRadius,
                              float pressure,
                              float temperature)
        {
            float halfw = JetHalfWidth(s);
            r = r / motorDistance;
            s = s / motorDistance;
            halfw = halfw / motorDistance;

            float xi = r / halfw;

            constexpr uint np = 4;
            float initialJetVelocity = sqrt((droneMass * 9.8) / (2 * AirDensity(pressure, temperature) * M_PI * rotorRadius * rotorRadius) * np);

            float denominator = (1 + (sqrt(2) - 1) * xi * xi);
            return CenterlineVelocity(s, initialJetVelocity) / (denominator * denominator);
        }

    private:
        static float CenterlineVelocity(float s, float Uj)
        {
            constexpr float Bd = 10.11;

            return Uj * Bd / (s - s0);
        }

        static float JetHalfWidth(float s)
        {
            constexpr float spreadingAngle = 12.f * Deg2Rad;
            const float spreadingRate = std::tan(spreadingAngle / 2.f);

            return spreadingRate * (s - s0);
        }

        static float AirDensity(float pressure, float temperature)
        {
            constexpr float M = 28.966;
            return pressure * M / (R * temperature);
        }

    private:
        static constexpr float s0 = -5.817;
    };
} // namespace gaden::Airflow