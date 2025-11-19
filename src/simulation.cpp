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
    vec3 dpos;  ///< Time derivative of position (velocity)
    vec3 dvel;  ///< Time derivative of velocity (acceleration)
};

/***********************
 * computeGravitationalForce
 * @brief: Computes mutual gravitational acceleration between two celestial bodies.
 * @param: a - first body (acceleration will be updated)
 * @param: b - second body (acceleration will be updated)
 * @exception: none
 * @return: none
 * @note: Applies Newton's law of universal gravitation.
 *        This function is intended to be called with i < j in an outer loop
 *        to preserve momentum symmetry and avoid double-counting.
 ***********************/
void computeGravitationalForce(CelestialBody& a, CelestialBody& b) {
    // Vector from a to b
    vec3 r_vec = b.position - a.position;
    double r2   = r_vec.length_squared();

    if (r2 < 1.0) {
        // Avoid singularities or extremely close approaches
        return;
    }

    double r     = std::sqrt(r2);
    double invr  = 1.0 / r;
    double invr3 = invr / r2;   // 1 / r^3

    // Acceleration directions:
    // a.acc =  G * m_b / r^3 * r_vec
    // b.acc = -G * m_a / r^3 * r_vec
    vec3 acc_dir = r_vec;

    vec3 acc_a = (physics::constants::G * b.mass * invr3) * acc_dir;
    vec3 acc_b = (physics::constants::G * a.mass * invr3) * (-acc_dir);

    a.acceleration += acc_a;
    b.acceleration += acc_b;
}

/***********************
 * eulerStep
 * @brief: Simple Euler integration step (unused in main loop but kept for reference).
 * @param: body - CelestialBody reference
 * @param: dt   - time step
 * @exception none
 * @return none
 ***********************/
void eulerStep(CelestialBody& body, double dt) {
    body.velocity += body.acceleration * dt;
    body.position += body.velocity     * dt;
}

/***********************
 * resetAccelerations
 * @brief: Sets acceleration vectors to zero for every body in the collection.
 ***********************/
void resetAccelerations(std::vector<CelestialBody>& bodies) {
    for (auto& b : bodies) {
        b.acceleration = vec3(0.0, 0.0, 0.0);
    }
}

/***********************
 * updateAccelerations
 * @brief: Recomputes gravitational accelerations for the entire system.
 * @note: Uses computeGravitationalForce pairwise with i < j
 *        to ensure Newton's 3rd law and avoid double-counting.
 ***********************/
void updateAccelerations(std::vector<CelestialBody>& bodies) {
    resetAccelerations(bodies);

    const std::size_t N = bodies.size();
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = i + 1; j < N; ++j) {
            computeGravitationalForce(bodies[i], bodies[j]);
        }
    }
}

/***********************
 * evaluateDerivatives
 * @brief: Produces derivatives for RK4 from the current state.
 * @param: bodies - current state
 * @return: vector of StateDerivative (dpos, dvel) for each body
 ***********************/
std::vector<StateDerivative> evaluateDerivatives(std::vector<CelestialBody>& bodies) {
    updateAccelerations(bodies);
    std::vector<StateDerivative> d(bodies.size());

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        d[i].dpos = bodies[i].velocity;
        d[i].dvel = bodies[i].acceleration;
    }
    return d;
}

/***********************
 * buildIntermediateState
 * @brief: Generates an intermediate RK4 state from base state and derivatives.
 * @param: bodies - base state
 * @param: d      - derivatives at this stage
 * @param: scale  - scaling factor (e.g. dt/2, dt)
 * @return: new vector<CelestialBody> representing intermediate state
 ***********************/
std::vector<CelestialBody> buildIntermediateState(
    const std::vector<CelestialBody>& bodies,
    const std::vector<StateDerivative>& d,
    double scale) {

    std::vector<CelestialBody> next = bodies;

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        next[i].position += scale * d[i].dpos;
        next[i].velocity += scale * d[i].dvel;
    }
    return next;
}

/***********************
 * rk4Step
 * @brief: Classical RK4 solver for N-body system.
 * @param: bodies - state to be advanced in time
 * @param: dt     - time step
 * @exception: none
 * @return: none
 ***********************/
void rk4Step(std::vector<CelestialBody>& bodies, double dt) {
    if (bodies.empty()) return;

    auto k1 = evaluateDerivatives(bodies);
    auto s2 = buildIntermediateState(bodies, k1, dt * 0.5);
    auto k2 = evaluateDerivatives(s2);

    auto s3 = buildIntermediateState(bodies, k2, dt * 0.5);
    auto k3 = evaluateDerivatives(s3);

    auto s4 = buildIntermediateState(bodies, k3, dt);
    auto k4 = evaluateDerivatives(s4);

    const double sixth = dt / 6.0;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].position += sixth * (k1[i].dpos + 2.0*k2[i].dpos + 2.0*k3[i].dpos + k4[i].dpos);
        bodies[i].velocity += sixth * (k1[i].dvel + 2.0*k2[i].dvel + 2.0*k3[i].dvel + k4[i].dvel);
    }
}

/********************
 * runSimulation
 * @brief: Runs 3D Sun–Earth–Moon simulation and writes CSV.
 * @param none
 * @return none
 * @exception none
 * @note: Outputs to orbit_three_body.csv
 *********************/
void runSimulation() {

    std::vector<CelestialBody> bodies = {
        {"Sun",   physics::constants::M_SUN,   0,0,0, 0,0,0, 0,0,0},
        {"Earth", physics::constants::M_EARTH, 0,0,0, 0,0,0, 0,0,0},
        {"Moon",  physics::constants::M_MOON,  0,0,0, 0,0,0, 0,0,0}
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
    earth.position = vec3(r_earth, 0.0, 0.0);
    moon.position  = vec3(r_earth, r_moon, 0.0);

    // ============================
    // 3. Initial velocities
    // ============================
    earth.velocity = vec3(0.0, v_earth, 0.0);
    moon.velocity  = vec3(-v_moon, v_earth, 0.0);

    // ============================
    // 4. Moon orbital inclination
    // ============================
    {
        double inc = physics::constants::MOON_INCLINATION;
        double c   = std::cos(inc);
        double s   = std::sin(inc);

        // --- POSITION ROTATION (about x-axis) ---
        double ry = moon.position.y() - earth.position.y();
        double rz = moon.position.z() - earth.position.z();

        double new_ry = ry * c - rz * s;
        double new_rz = ry * s + rz * c;

        moon.position = vec3(
            earth.position.x(),
            earth.position.y() + new_ry,
            earth.position.z() + new_rz
        );

        // --- VELOCITY ROTATION (about x-axis) ---
        double rvy = moon.velocity.y() - earth.velocity.y();
        double rvz = moon.velocity.z() - earth.velocity.z();

        double new_rvy = rvy * c - rvz * s;
        double new_rvz = rvy * s + rvz * c;

        moon.velocity = vec3(
            moon.velocity.x(),                       // keep x-component
            earth.velocity.y() + new_rvy,
            earth.velocity.z() + new_rvz
        );
    }

    // ============================
    // 5. Barycenter correction
    // ============================
    {
        // total linear momentum P = Σ m v
        vec3 P = sun.mass   * sun.velocity
               + earth.mass * earth.velocity
               + moon.mass  * moon.velocity;

        // Shift Sun velocity so that total momentum is zero (barycentric frame)
        sun.velocity = sun.velocity - (P / sun.mass);
    }

    // tiny z correction
    {
        double r = moon.mass / earth.mass;

        earth.position = vec3(
            earth.position.x(),
            earth.position.y(),
            -r * moon.position.z()
        );

        earth.velocity = vec3(
            earth.velocity.x(),
            earth.velocity.y(),
            -r * moon.velocity.z()
        );
    }

    // ============================
    // 6. Conservations check
    // ============================
    physics::Conservations C0 = physics::compute(bodies);
    double E0 = C0.total_energy;

    const double L0 = std::sqrt(
        C0.L[0]*C0.L[0] +
        C0.L[1]*C0.L[1] +
        C0.L[2]*C0.L[2]
    );

    const double P0mag = std::sqrt(
        C0.P[0]*C0.P[0] +
        C0.P[1]*C0.P[1] +
        C0.P[2]*C0.P[2]
    );

    // ============================
    // 7. CSV output (N-body generic)
    // ============================
    std::ofstream file("orbit_three_body.csv");

    /**********************************************
     * CSV HEADER (Generic for any N bodies)
     **********************************************/
    file << "step,";
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        file << "x_" << bodies[i].name << ","
             << "y_" << bodies[i].name << ","
             << "z_" << bodies[i].name << ",";
    }

    file << "shadow_x,shadow_y,shadow_z,"
         << "umbra_r,penumbra_r,eclipse_type,"
         << "E_total,KE,PE,"
         << "Lx,Ly,Lz,Lmag,"
         << "Px,Py,Pz,Pmag,"
         << "dE_rel,dL_rel,dP_rel\n";

    const int    steps = 8766;
    const double dt    = physics::constants::DT;

    // ============================
    // 8. Main integration loop
    // ============================
    for (int i = 0; i < steps; ++i) {
        rk4Step(bodies, dt);

        // --- Conservation diagnostics (now N-body aware) ---
        physics::Conservations C = physics::compute(bodies);

        double Lmag = std::sqrt(
            C.L[0]*C.L[0] +
            C.L[1]*C.L[1] +
            C.L[2]*C.L[2]
        );

        double dE = (C.total_energy - E0) / std::abs(E0);
        double dL = (Lmag - L0)         / L0;

        double Pmag = std::sqrt(
            C.P[0]*C.P[0] +
            C.P[1]*C.P[1] +
            C.P[2]*C.P[2]
        );

        double dP = (Pmag - P0mag) / (P0mag == 0 ? 1.0 : P0mag);

        // -------- Eclipse computation --------
        vec3 S = sun.position;
        vec3 E = earth.position;
        vec3 M = moon.position;

        EclipseResult eclipse = computeSolarEclipse(S, E, M);

        // ============================
        // CSV ROW (Generic N-body)
        // ============================
        file << i << ",";

        for (const auto& b : bodies) {
            file << b.position.x() << ","
                 << b.position.y() << ","
                 << b.position.z() << ",";
        }

        file << eclipse.shadowCenter.x() << ","
             << eclipse.shadowCenter.y() << ","
             << eclipse.shadowCenter.z() << ","
             << eclipse.umbraRadius      << ","
             << eclipse.penumbraRadius   << ","
             << eclipse.eclipseType      << ","
             << C.total_energy           << ","
             << C.kinetic_energy         << ","
             << C.potential_energy       << ","
             << C.L[0] << "," << C.L[1] << "," << C.L[2] << ","
             << Lmag << ","
             << C.P[0] << "," << C.P[1] << "," << C.P[2] << ","
             << Pmag << ","
             << dE << "," << dL << "," << dP << "\n";
    }

    file.close();
    std::cout << "✅ 3D simulation + eclipse data written to orbit_three_body.csv\n";
}
