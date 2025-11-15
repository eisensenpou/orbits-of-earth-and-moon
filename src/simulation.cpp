/****************
 * Author: Sinan Demir
 * File: simulation.cpp
 * Date: 10/31/2025
 * Purpose: Implementation file of simulation
 *****************/

#include "simulation.h"

/****************
 * struct StateDerivative
 * Purpose: Captures instantaneous derivatives for position and velocity components.
 *****************/
struct StateDerivative {
    double dx;
    double dy;
    double dz;
    double dvx;
    double dvy;
    double dvz;
};


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
    double dz = b.z - a.z;
    double r2 = dx * dx + dy * dy + dz * dz;
    double r = std::sqrt(r2);

    if (r < 1.0) return; // avoid division by zero if too close

    double F = constants::G * b.mass / r2; // acceleration force
    a.ax += F * (dx / r);
    a.ay += F * (dy / r);
    a.az += F * (dz / r);
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
    body.vz += body.az * dt;
    body.x  += body.vx * dt;
    body.y  += body.vy * dt;
    body.z += body.vz * dt;  // we won't use euler's method but it's here for completeness
}

void resetAccelerations(std::vector<CelestialBody>& bodies) {
    /***********************
     * resetAccelerations
     * @brief: Sets acceleration components to zero for every body in the collection.
     * @param: bodies - vector of CelestialBody instances
     * @exception none
     * @return none
     * @note: Helper for both Euler and RK4 integrators.
     ***********************/
    for (auto& body : bodies) {
        body.ax = 0.0;
        body.ay = 0.0;
        body.az = 0.0;
    }
}

void updateAccelerations(std::vector<CelestialBody>& bodies) {
    /***********************
     * updateAccelerations
     * @brief: Recomputes gravitational accelerations for the entire system.
     * @param: bodies - vector of CelestialBody instances
     * @exception none
     * @return none
     * @note: Uses computeGravitationalForce pairwise.
     ***********************/
    resetAccelerations(bodies);
    for (size_t m = 0; m < bodies.size(); ++m) {
        for (size_t n = 0; n < bodies.size(); ++n) {
            if (m == n) continue;
            computeGravitationalForce(bodies[m], bodies[n]);
        }
    }
}

std::vector<StateDerivative> evaluateDerivatives(std::vector<CelestialBody>& bodies) {
    /***********************
     * evaluateDerivatives
     * @brief: Produces derivatives (dx/dt, dy/dt, dvx/dt, dvy/dt) for every body.
     * @param: bodies - state vector used for derivative evaluation
     * @exception none
     * @return: collection of StateDerivative entries aligned with bodies vector.
     * @note: Invokes updateAccelerations to ensure accelerations are current.
     ***********************/
    updateAccelerations(bodies);
    std::vector<StateDerivative> derivatives(bodies.size());
    for (size_t i = 0; i < bodies.size(); ++i) {
        derivatives[i].dx  = bodies[i].vx;
        derivatives[i].dy  = bodies[i].vy;
        derivatives[i].dz = bodies[i].vz;
        derivatives[i].dvx = bodies[i].ax;
        derivatives[i].dvy = bodies[i].ay;
        derivatives[i].dvz = bodies[i].az;
    }
    return derivatives;
}

std::vector<CelestialBody> buildIntermediateState(
    const std::vector<CelestialBody>& bodies,
    const std::vector<StateDerivative>& derivatives,
    double scale) {
    /***********************
     * buildIntermediateState
     * @brief: Generates an intermediate RK4 state by applying derivatives with a scale factor.
     * @param: bodies - reference state
     * @param: derivatives - derivative evaluations aligned with bodies
     * @param: scale - dt multiplier for the RK4 stage
     * @exception none
     * @return: new vector of CelestialBody reflecting the partial step
     * @note: Keeps original bodies unmodified.
     ***********************/
    std::vector<CelestialBody> next = bodies;
    for (size_t i = 0; i < bodies.size(); ++i) {
        next[i].x  += scale * derivatives[i].dx;
        next[i].y  += scale * derivatives[i].dy;
        next[i].z += scale * derivatives[i].dz;
        next[i].vx += scale * derivatives[i].dvx;
        next[i].vy += scale * derivatives[i].dvy;
        next[i].vz += scale * derivatives[i].dvz;
    }
    return next;
}

void rk4Step(std::vector<CelestialBody>& bodies, double dt) {
    /***********************
     * rk4Step
     * @brief: Advances the N-body system using a classical 4th-order Runge-Kutta step.
     * @param: bodies - vector containing the simulation state
     * @param: dt - integration timestep
     * @exception none
     * @return none
     * @note: Provides higher accuracy than Euler while preserving existing API.
     ***********************/
    if (bodies.empty()) return;

    auto k1 = evaluateDerivatives(bodies);
    auto state_k2 = buildIntermediateState(bodies, k1, 0.5 * dt);
    auto k2 = evaluateDerivatives(state_k2);

    auto state_k3 = buildIntermediateState(bodies, k2, 0.5 * dt);
    auto k3 = evaluateDerivatives(state_k3);

    auto state_k4 = buildIntermediateState(bodies, k3, dt);
    auto k4 = evaluateDerivatives(state_k4);

    const double sixth = dt / 6.0;
    for (size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].x  += sixth * (k1[i].dx  + 2.0 * k2[i].dx  + 2.0 * k3[i].dx  + k4[i].dx);
        bodies[i].y  += sixth * (k1[i].dy  + 2.0 * k2[i].dy  + 2.0 * k3[i].dy  + k4[i].dy);
        bodies[i].z  += sixth * (k1[i].dz  + 2.0 * k2[i].dz  + 2.0 * k3[i].dz  + k4[i].dz);
        bodies[i].vx += sixth * (k1[i].dvx + 2.0 * k2[i].dvx + 2.0 * k3[i].dvx + k4[i].dvx);
        bodies[i].vy += sixth * (k1[i].dvy + 2.0 * k2[i].dvy + 2.0 * k3[i].dvy + k4[i].dvy);
        bodies[i].vz += sixth * (k1[i].dvz + 2.0 * k2[i].dvz + 2.0 * k3[i].dvz + k4[i].dvz);
    }
}

void runSimulation() {
    /********************
     * runSimulation
     * @brief: Runs the simulation for the Sun, Earth, and Moon; writes data to CSV (in 3D).
     * @param none
     * @exception none
     * @return none
     * @note: Now supports full 3D positions and velocities.
     *********************/

    std::vector<CelestialBody> bodies = {
        // name   mass              x      y   z     vx   vy   vz
        {"Sun",   constants::M_SUN, 0.0,   0.0, 0.0,  0.0, 0.0, 0.0},
        {"Earth", 5.972e24,          0.0,   0.0, 0.0,  0.0, 0.0, 0.0},
        {"Moon",  7.3477e22,         0.0,   0.0, 0.0,  0.0, 0.0, 0.0}
    };

    CelestialBody& sun   = bodies[0];
    CelestialBody& earth = bodies[1];
    CelestialBody& moon  = bodies[2];

    // ============================
    // 1. Define orbital parameters
    // ============================

    const double r_earth = 1.47098074e11;     // perihelion ~1 AU
    const double r_moon  = 384400000.0;       // mean lunar distance (m)

    // Earth circular orbital speed around Sun
    const double v_earth = std::sqrt(constants::G * constants::M_SUN / r_earth);

    // Moon circular speed around Earth
    const double v_moon  = std::sqrt(constants::G * (earth.mass + moon.mass) / r_moon);

    // ================================
    // 2. Initial positions (geometric)
    // ================================

    // Earth starts at +x axis
    earth.x = r_earth;
    earth.y = 0.0;
    earth.z = 0.0;

    // Moon initially displaced along +y from Earth
    moon.x = earth.x;
    moon.y = earth.y + r_moon;
    moon.z = 0.0;

    // ================================
    // 3. Initial velocities (tangential)
    // ================================

    // Earth moves tangentially +y direction
    earth.vx = 0.0;
    earth.vy = v_earth;     // NOTE: THIS is the major fix—your version had vx=30300
    earth.vz = 0.0;

    // Moon velocity = Earth velocity + tangential component (prograde)
    // Since moon is at +y from Earth, tangential velocity is in −x direction
    moon.vx = earth.vx - v_moon;
    moon.vy = earth.vy;
    moon.vz = 0.0;

    // ======================================================
    // 4. Tilt the Moon’s orbit by 5.145° about the x-axis
    // ======================================================
    {
        double inc = constants::MOON_INCLINATION;
        double cosI = std::cos(inc);
        double sinI = std::sin(inc);

        // --- Rotate Moon position relative to Earth ---
        double rel_y = moon.y - earth.y;
        double rel_z = moon.z - earth.z;

        moon.y = earth.y + rel_y * cosI;
        moon.z = earth.z + rel_y * sinI;

        // --- Rotate Moon velocity relative to Earth ---
        double rel_vy = moon.vy - earth.vy;
        double rel_vz = moon.vz - earth.vz;

        moon.vy = earth.vy + rel_vy * cosI;
        moon.vz = earth.vz + rel_vy * sinI;
    }

    // ======================================================
    // 5. Make barycenter stationary (recommended)
    // ======================================================

    double px = sun.mass * sun.vx + earth.mass * earth.vx + moon.mass * moon.vx;
    double py = sun.mass * sun.vy + earth.mass * earth.vy + moon.mass * moon.vy;
    double pz = sun.mass * sun.vz + earth.mass * earth.vz + moon.mass * moon.vz;

    sun.vx -= px / sun.mass;
    sun.vy -= py / sun.mass;
    sun.vz -= pz / sun.mass;

    // ======================================================
    // 6. Optional tiny barycenter correction (your original)
    // ======================================================
    {
        double mass_ratio = moon.mass / earth.mass;
        earth.z  = -mass_ratio * moon.z;
        earth.vz = -mass_ratio * moon.vz;
    }

    // ======================================================
    // 7. CSV output file
    // ======================================================

    std::ofstream file("orbit_three_body.csv");
    file << "step,"
         << "x_sun,y_sun,z_sun,"
         << "x_earth,y_earth,z_earth,"
         << "x_moon,y_moon,z_moon\n";

    const int steps = 8766;            // 1 year (hourly)
    const double dt = constants::DT;   // your Δt

    for (int i = 0; i < steps; ++i) {
        rk4Step(bodies, dt);

        file << i << ","
             << sun.x   << "," << sun.y   << "," << sun.z   << ","
             << earth.x << "," << earth.y << "," << earth.z << ","
             << moon.x  << "," << moon.y  << "," << moon.z  << "\n";
    }

    file.close();
    std::cout << "✅ 3D three-body simulation complete. Data saved to orbit_three_body.csv\n";
}