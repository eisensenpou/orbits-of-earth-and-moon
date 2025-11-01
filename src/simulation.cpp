/****************
 * Author: Sinan Demir
 * File: simulation.cpp
 * Date: 10/31/2025
 * Purpose: Implementation file of simulation
 *****************/

#include "simulation.h"


void computeGravitationalForce(CelestialBody& a, CelestialBody& b) {
    /***********************
     * computeGravitationalForce
     * @brief: Computes gravitational acceleration between two celestial bodies.
     * @param: a - first body (will have acceleration updated)
     * @param: b - second body (source of gravity)
     * @exception: none
     * @return: none
     * @note: Applies Newton's law of universal gravitation.
     ***********************/
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double r2 = dx * dx + dy * dy;
    double r = std::sqrt(r2);

    if (r < 1.0) return; // avoid division by zero if too close

    double F = constants::G * b.mass / r2;
    a.ax += F * (dx / r);
    a.ay += F * (dy / r);
}


void eulerStep(CelestialBody& body, double dt) {
    /***********************
     * eulerStep
     * @brief: Euler calculation step follows this calculation
     *         V = V0 + at
     *         x = x0 + v.t
     * @param: CelestialBody reference
     * @param: delta t (change in time)
     * @exception none
     * @return none
     * @note
     ***********************/
    body.vx += body.ax * dt;
    body.vy += body.ay * dt;
    body.x  += body.vx * dt;
    body.y  += body.vy * dt;
}

void runSimulation() {
    /********************
     * runSimulation
     * @brief: Runs the simulation for the Sun, Earth, and Moon; writes data to CSV.
     * @param none
     * @exception none
     * @return none
     * @note: Adds 3-body simulation (Sun–Earth–Moon system).
     *********************/

    // Define bodies
    CelestialBody sun   {"Sun",   constants::M_SUN, 0, 0, 0, 0, 0, 0};
    CelestialBody earth {"Earth", 5.972e24, 1.47098074e11, 0, 0, 30300, 0, 0};
    CelestialBody moon {
        "Moon",
        7.3477e22,
        1.47098074e11 + 384400000, // position offset
        0,
        0,
        30300 + 1022,              // velocity: Earth’s + orbital
        0,
        0
    };


    std::vector<CelestialBody*> bodies = {&sun, &earth, &moon};

    // Prepare output file
    std::ofstream file("orbit_three_body.csv");
    file << "step,x_sun,y_sun,x_earth,y_earth,x_moon,y_moon\n";

    const int steps = 8766; // one year (hourly)
    const double dt = constants::DT;

    for (int i = 0; i < steps; ++i) {
        // Reset accelerations
        for (auto* b : bodies) {
            b->ax = b->ay = 0;
        }

        // Compute forces pairwise
        for (size_t m = 0; m < bodies.size(); ++m) {
            for (size_t n = 0; n < bodies.size(); ++n) {
                if (m == n) continue;
                computeGravitationalForce(*bodies[m], *bodies[n]);
            }
        }

        // Euler step for all bodies
        for (auto* b : bodies) {
            eulerStep(*b, dt);
        }

        // Write data for visualization
        file << i << "," 
             << sun.x << "," << sun.y << ","
             << earth.x << "," << earth.y << ","
             << moon.x << "," << moon.y << "\n";
    }

    file.close();
    std::cout << "✅ Three-body simulation complete. Data saved to orbit_three_body.csv\n";
}
