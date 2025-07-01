#pragma once

#include <gaden/core/Assertions.hpp>
#include <string>
namespace gaden
{
    constexpr float R = 82.057338; // R is the ideal, or universal, gas constant, equal to the product of the Boltzmann constant and the Avogadro constant [cm³·atm/mol·K] Gas Constant

    // clang-format off

    // we need to keep the backing type as int to avoid breaking the binary serialization
    enum class GasType : int
    {
        unknown             = -1, // something is wrong in the config/results file
        ethanol             = 0,
        methane             = 1,
        hydrogen            = 2,
        propanol            = 3,
        chlorine            = 4,
        flurorine           = 5,
        acetone             = 6,
        neon                = 7,
        helium              = 8,
        biogas              = 9,
        butane              = 10,
        carbonDioxide       = 11,
        carbonMonoxide      = 12,
        smoke               = 13
    };
    // clang-format on

    // Molecular gas mass [g/mol]
    // SpecificGravity(Air) = 1 (as reference)
    // Specific gravity is the ratio of the density of a substance to the density of a reference substance; equivalently,
    // it is the ratio of the mass of a substance to the mass of a reference substance for the same given volume.
    constexpr std::array<float, 14> SpecificGravity = {
        1.0378, // ethanol   (heavier than air)
        0.5537, // methane   (lighter than air)
        0.0696, // hydrogen  (lighter than air)
        1.23,   // propanol   //gases heavier then air
        2.48,   // chlorine
        1.31,   // fluorine
        1.4529, // acetone   (heavier than air)
        0.7,    // neon	   //gases lighter than air
        0.138,  // helium
        0.8,    // sewage, biogas
        2.0061, // butane
        1.52,   // carbon dioxide
        0.967,  // carbon monoxide
        0.89    // smoke
    };

    inline std::string to_string(GasType type)
    {
        switch (type)
        {
        case GasType::unknown:
            return "unknown";
        case GasType::ethanol:
            return "ethanol";
        case GasType::methane:
            return "methane";
        case GasType::hydrogen:
            return "hydrogen";
        case GasType::propanol:
            return "propanol";
        case GasType::chlorine:
            return "chlorine";
        case GasType::flurorine:
            return "flurorine";
        case GasType::acetone:
            return "acetone";
        case GasType::neon:
            return "neon";
        case GasType::helium:
            return "helium";
        case GasType::biogas:
            return "biogas";
        case GasType::butane:
            return "butane";
        case GasType::carbonDioxide:
            return "carbonDioxide";
        case GasType::carbonMonoxide:
            return "carbonMonoxide";
        case GasType::smoke:
            return "smoke";
        }

        GADEN_ERROR("Invalid gas type: {}", (int)type);
        GADEN_TERMINATE;
    }

} // namespace gaden