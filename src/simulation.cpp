/****************
 * Author: Sinan Demir
 * File: simulation.cpp
 * Date: 10/31/2025
 * Purpose: Implementation file of simulation
 *****************/

#include "simulation.h"
#include "vec3.h"
#include "eclipse.h"

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
    double r2 = dx*dx + dy*dy + dz*dz;
    double r = std::sqrt(r2);

    if (r < 1.0) return;

    double F = physics::constants::G * b.mass / r2;
    a.ax += F * (dx / r);
    a.ay += F * (dy / r);
    a.az += F * (dz / r);
}


void eulerStep(CelestialBody& body, double dt) {
    /***********************
     * eulerStep
     * @brief: Euler calculation step follows this calculation
     * @param: CelestialBody reference
     * @param: delta t (change in time)
     * @exception none
     * @return none
     ***********************/
    body.vx += body.ax * dt;
    body.vy += body.ay * dt;
    body.vz += body.az * dt;
    body.x  += body.vx * dt;
    body.y  += body.vy * dt;
    body.z  += body.vz * dt;
}

void resetAccelerations(std::vector<CelestialBody>& bodies) {
    /***********************
     * resetAccelerations
     * @brief: Sets acceleration components to zero for every body in the collection.
     ***********************/
    for (auto& b : bodies) {
        b.ax = b.ay = b.az = 0.0;
    }
}

void updateAccelerations(std::vector<CelestialBody>& bodies) {
    /***********************
     * updateAccelerations
     * @brief: Recomputes gravitational accelerations for the entire system.
     * @note: Uses computeGravitationalForce pairwise.
     ***********************/
    resetAccelerations(bodies);
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i != j) computeGravitationalForce(bodies[i], bodies[j]);
        }
    }
}

std::vector<StateDerivative> evaluateDerivatives(std::vector<CelestialBody>& bodies) {
    /***********************
     * evaluateDerivatives
     * @brief: Produces derivatives for RK4.
     ***********************/
    updateAccelerations(bodies);
    std::vector<StateDerivative> d(bodies.size());

    for (size_t i = 0; i < bodies.size(); ++i) {
        d[i].dx  = bodies[i].vx;
        d[i].dy  = bodies[i].vy;
        d[i].dz  = bodies[i].vz;
        d[i].dvx = bodies[i].ax;
        d[i].dvy = bodies[i].ay;
        d[i].dvz = bodies[i].az;
    }
    return d;
}

std::vector<CelestialBody> buildIntermediateState(
    const std::vector<CelestialBody>& bodies,
    const std::vector<StateDerivative>& d,
    double scale) {
    /***********************
     * buildIntermediateState
     * @brief: Generates an intermediate RK4 state.
     ***********************/
    std::vector<CelestialBody> next = bodies;

    for (size_t i = 0; i < bodies.size(); ++i) {
        next[i].x  += scale * d[i].dx;
        next[i].y  += scale * d[i].dy;
        next[i].z  += scale * d[i].dz;
        next[i].vx += scale * d[i].dvx;
        next[i].vy += scale * d[i].dvy;
        next[i].vz += scale * d[i].dvz;
    }
    return next;
}

void rk4Step(std::vector<CelestialBody>& bodies, double dt) {
    /***********************
     * rk4Step
     * @brief: Classical RK4 solver for N-body system.
     ***********************/
    if (bodies.empty()) return;

    auto k1 = evaluateDerivatives(bodies);
    auto s2 = buildIntermediateState(bodies, k1, dt * 0.5);
    auto k2 = evaluateDerivatives(s2);

    auto s3 = buildIntermediateState(bodies, k2, dt * 0.5);
    auto k3 = evaluateDerivatives(s3);

    auto s4 = buildIntermediateState(bodies, k3, dt);
    auto k4 = evaluateDerivatives(s4);

    const double sixth = dt / 6.0;
    for (size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].x  += sixth * (k1[i].dx  + 2*k2[i].dx  + 2*k3[i].dx  + k4[i].dx);
        bodies[i].y  += sixth * (k1[i].dy  + 2*k2[i].dy  + 2*k3[i].dy  + k4[i].dy);
        bodies[i].z  += sixth * (k1[i].dz  + 2*k2[i].dz  + 2*k3[i].dz  + k4[i].dz);
        bodies[i].vx += sixth * (k1[i].dvx + 2*k2[i].dvx + 2*k3[i].dvx + k4[i].dvx);
        bodies[i].vy += sixth * (k1[i].dvy + 2*k2[i].dvy + 2*k3[i].dvy + k4[i].dvy);
        bodies[i].vz += sixth * (k1[i].dvz + 2*k2[i].dvz + 2*k3[i].dvz + k4[i].dvz);
    }
}


void runSimulation() {
    /********************
     * runSimulation
     * @brief: Runs 3D Sun–Earth–Moon simulation and writes CSV.
     * @param none
     * @return none
     * @exception none
     * @note: Outputs to orbit_three_body.csv
     *********************/

    std::vector<CelestialBody> bodies = {
        {"Sun",   physics::constants::M_SUN, 0,0,0, 0,0,0, 0,0,0},
        {"Earth", physics::constants::M_EARTH, 0,0,0, 0,0,0, 0,0,0},
        {"Moon",  physics::constants::M_MOON, 0,0,0, 0,0,0, 0,0,0}
    };

    CelestialBody& sun   = bodies[0];
    CelestialBody& earth = bodies[1];
    CelestialBody& moon  = bodies[2];

    // ============================
    // 1. Orbital parameters
    // ============================

    const double r_earth = physics::constants::EARTH_PERIHELION;
    const double r_moon  = physics::constants::MOON_ORBIT_RADIUS;
    const double v_earth = std::sqrt(physics::constants::G * physics::constants::M_SUN / r_earth);
    const double v_moon  = std::sqrt(physics::constants::G * (earth.mass + moon.mass) / r_moon);

    // ============================
    // 2. Initial positions
    // ============================
    earth.x = r_earth;
    moon.x  = r_earth;
    moon.y  = r_moon;

    // ============================
    // 3. Initial velocities
    // ============================
    earth.vy = v_earth;
    moon.vx  = -v_moon;
    moon.vy  = v_earth;

    // ============================
    // 4. Moon orbital inclination
    // ============================
    {
    double inc = physics::constants::MOON_INCLINATION;
    double c = std::cos(inc);
    double s = std::sin(inc);

    // --- POSITION ROTATION ---
    double ry = moon.y - earth.y;
    double rz = moon.z - earth.z;

    double new_ry = ry * c - rz * s;
    double new_rz = ry * s + rz * c;

    moon.y = earth.y + new_ry;
    moon.z = earth.z + new_rz;

    // --- VELOCITY ROTATION ---
    double rvy = moon.vy - earth.vy;
    double rvz = moon.vz - earth.vz;

    double new_rvy = rvy * c - rvz * s;
    double new_rvz = rvy * s + rvz * c;

    moon.vy = earth.vy + new_rvy;
    moon.vz = earth.vz + new_rvz;
    }

    // ============================
    // 5. Barycenter correction
    // ============================
    double px = sun.mass*sun.vx + earth.mass*earth.vx + moon.mass*moon.vx;
    double py = sun.mass*sun.vy + earth.mass*earth.vy + moon.mass*moon.vy;
    double pz = sun.mass*sun.vz + earth.mass*earth.vz + moon.mass*moon.vz;

    sun.vx -= px / sun.mass;
    sun.vy -= py / sun.mass;
    sun.vz -= pz / sun.mass;

    // tiny z correction
    {
        double r = moon.mass / earth.mass;
        earth.z  = -r * moon.z;
        earth.vz = -r * moon.vz;
    }

    // ============================
    // 6. Conservations check
    // ============================
    physics::Conservations C0 = physics::compute(sun, earth, moon);
    double E0 = C0.total_energy;
    double L0 = std::sqrt(C0.L[0]*C0.L[0] + C0.L[1]*C0.L[1] + C0.L[2]*C0.L[2]);

    // ============================
    // 6. CSV output
    // ============================
    std::ofstream file("orbit_three_body.csv");
    file << "step,"
         << "x_sun,y_sun,z_sun,"
         << "x_earth,y_earth,z_earth,"
         << "x_moon,y_moon,z_moon,"
         << "shadow_x,shadow_y,shadow_z,"
         << "umbra_r,penumbra_r,eclipse_type,"
         << "E_total,KE,PE,"
         << "Lx,Ly,Lz,Lmag,"
         << "Px,Py,Pz,Pmag,"
         << "dE_rel,dL_rel,dP_rel\n";

    const int steps = 8766;
    const double dt = physics::constants::DT;

    // ============================
    // 7. Main loop
    // ============================
    for (int i = 0; i < steps; ++i) {
        rk4Step(bodies, dt);

        // -------- Conservation diagnostics --------
        physics::Conservations C = physics::compute(sun, earth, moon);
        double Lmag = std::sqrt(
            C.L[0]*C.L[0] +
            C.L[1]*C.L[1] +
            C.L[2]*C.L[2]
        );
        double dE = (C.total_energy - E0) / std::abs(E0);
        double dL = (Lmag - L0) / L0;

        // ---- Baseline Linear Momentum ----
        double P0 = std::sqrt(
            C0.P[0] * C0.P[0] +
            C0.P[1] * C0.P[1] +
            C0.P[2] * C0.P[2]
        );

        double Pmag = std::sqrt(
            C.P[0] * C.P[0] +
            C.P[1] * C.P[1] +
            C.P[2] * C.P[2]
        );
        // Relative linear momentum drift
        double dP = (Pmag - P0) / (P0 == 0 ? 1 : P0);

        // -------- Eclipse computation --------
        vec3 S(sun.x,   sun.y,   sun.z);
        vec3 E(earth.x, earth.y, earth.z);
        vec3 M(moon.x,  moon.y,  moon.z);

        EclipseResult eclipse = computeSolarEclipse(S, E, M);

        file << i << ","
             << sun.x   << "," << sun.y   << "," << sun.z   << ","
             << earth.x << "," << earth.y << "," << earth.z << ","
             << moon.x  << "," << moon.y  << "," << moon.z  << ","
             << eclipse.shadowCenter.x() << ","
             << eclipse.shadowCenter.y() << ","
             << eclipse.shadowCenter.z() << ","
             << eclipse.umbraRadius      << ","
             << eclipse.penumbraRadius   << ","
             << eclipse.eclipseType      << ","
             // Energy-related
             << C.total_energy     << ","
             << C.kinetic_energy   << ","
             << C.potential_energy << ","
             // Angular momentum
             << C.L[0] << "," << C.L[1] << "," << C.L[2] << ","
             << Lmag   << ","
             // Linear momentum
             << C.P[0] << "," << C.P[1] << "," << C.P[2] << ","
             << Pmag   << ","
             // Drifts
             << dE     << ","
             << dL     << ","
             << dP
             << "\n";
    }

    file.close();
    std::cout << "✅ 3D simulation + eclipse data written to orbit_three_body.csv\n";
}
