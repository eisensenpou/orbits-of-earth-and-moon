/******************
 * Author: Sinan Demir
 * File: conservations.cpp
 * Purpose: Implementation of conservation-law diagnostics for the 3-body simulator
 * Date: 11/16/2025
 *********************/

#include "conservations.h"

namespace physics {

// RECALL constants::G = 6.67430e-11 (m^3 kg^-1 s^-2)


// ---- helper: Euclidean distance ---- //
static inline double distance(const CelestialBody& a, const CelestialBody& b){
/*****************
 * Distance between two bodies
 * @param: a - first body
 * @param: b - second body
 * @return: Euclidean distance between a and b
 * @exception: none
 * @note: uses std::sqrt from <cmath>
 *****************/
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// ---- helper: r × (m v) ---- //
static inline std::array<double,3> angular_term(const CelestialBody& b){
/*****************
 * Computes angular momentum term r × (m v) for a body
 * @param: b - celestial body
 * @return: 3D vector of angular momentum contribution
 * @exception: none
 * @note: uses std::array from <array>
 *****************/
    return {
        b.mass * (b.y * b.vz - b.z * b.vy),
        b.mass * (b.z * b.vx - b.x * b.vz),
        b.mass * (b.x * b.vy - b.y * b.vx)
    };
}

Conservations compute(const CelestialBody& sun,
                      const CelestialBody& earth,
                      const CelestialBody& moon){
/*****************
 * Computes conservation diagnostics for the 3-body system
 * @param: sun - Sun body
 * @param: earth - Earth body
 * @param: moon - Moon body
 * @return: Conservations struct with energy and momentum values
 * @exception: none
 * @note: uses helper functions distance() and angular_term()
 * ****************/
    Conservations C;

    // ---- Kinetic Energy ---- //
    auto v2 = [](const CelestialBody& b) {
        return b.vx*b.vx + b.vy*b.vy + b.vz*b.vz;
    };

    C.kinetic_energy =
          0.5 * sun.mass   * v2(sun)
        + 0.5 * earth.mass * v2(earth)
        + 0.5 * moon.mass  * v2(moon);

    // ---- Potential Energy ---- //
    double r_se = distance(sun, earth);
    double r_sm = distance(sun, moon);
    double r_em = distance(earth, moon);

    C.potential_energy =
    -constants::G * (
        sun.mass   * earth.mass / r_se +
        sun.mass   * moon.mass  / r_sm +
        earth.mass * moon.mass  / r_em
    );

    // ---- Total Energy ---- //
    C.total_energy = C.kinetic_energy + C.potential_energy;

    // ---- Linear Momentum ---- //
    C.P[0] = sun.mass * sun.vx + earth.mass * earth.vx + moon.mass * moon.vx;
    C.P[1] = sun.mass * sun.vy + earth.mass * earth.vy + moon.mass * moon.vy;
    C.P[2] = sun.mass * sun.vz + earth.mass * earth.vz + moon.mass * moon.vz;

    // ---- Angular Momentum ---- //
    auto Ls = angular_term(sun);
    auto Le = angular_term(earth);
    auto Lm = angular_term(moon);

    C.L[0] = Ls[0] + Le[0] + Lm[0];
    C.L[1] = Ls[1] + Le[1] + Lm[1];
    C.L[2] = Ls[2] + Le[2] + Lm[2];

    return C;
}

} // namespace physics
