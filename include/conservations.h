/******************
 * Author: Sinan Demir
 * File: conservation.h
 * Purpose: Conservation-law diagnostics (C++)
 * Date: 11/16/2025
 *********************/

#ifndef CONSERVATIONS_H
#define CONSERVATIONS_H

#include "body.h"
#include <array>
#include <cmath>
#include "utils.h"

namespace physics {

struct Conservations {

    // --- Energies --- //
    double kinetic_energy      = 0.0;
    double potential_energy    = 0.0;
    double total_energy        = 0.0;

    // --- Linear Momentum --- //
    std::array<double, 3> P{0.0, 0.0, 0.0};

    // --- Angular Momentum --- //
    std::array<double, 3> L{0.0, 0.0, 0.0};
};

// Computes all conservation values for 3 bodies
Conservations compute(
    const CelestialBody& sun,
    const CelestialBody& earth,
    const CelestialBody& moon
);

} // namespace physics

#endif // CONSERVATIONS_H
