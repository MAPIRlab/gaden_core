#pragma once

#include "gaden/Environment.hpp"
#include "gaden/datatypes/GasTypes.hpp"

namespace gaden::Airflow
{
    // from https://ieeexplore.ieee.org/document/10804051
    //------------------------------------------------------------
    class QuadrotorDisturbanceFarField
    {
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

        //  calculates the airflow velocity caused by a hovering quadrotor at point (r,s)
        //  r : radial distance
        //  s : flow-direction distance
        //  l : motor distance
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
            float centerLineV = CenterlineVelocity(s, initialJetVelocity);
            float speed = centerLineV / (denominator * denominator);
            return speed;
        }

    private:
        static float CenterlineVelocity(float s, float Uj)
        {
            constexpr float Bd = 10.11;

            return Uj * Bd / (s - s0);
        }

        static float JetHalfWidth(float s)
        {
            const float spreadingRate = 0.07668;
            return spreadingRate * (s - s0) + 0.1;
        }

        static float AirDensity(float pressure, float temperature)
        {
            constexpr float M = 28.966;
            const float r = R * 0.001;                        // express gas constant in L*atm / mol*K
            float density = pressure * M / (r * temperature); // g/L, which is equivalent to kg/m^3
            return density;
        }

    private:
        static constexpr float s0 = -5.817;
    };
} // namespace gaden::Airflow