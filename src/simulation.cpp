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
    double dvx;
    double dvy;
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
        derivatives[i].dvx = bodies[i].ax;
        derivatives[i].dvy = bodies[i].ay;
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
        next[i].vx += scale * derivatives[i].dvx;
        next[i].vy += scale * derivatives[i].dvy;
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
        bodies[i].vx += sixth * (k1[i].dvx + 2.0 * k2[i].dvx + 2.0 * k3[i].dvx + k4[i].dvx);
        bodies[i].vy += sixth * (k1[i].dvy + 2.0 * k2[i].dvy + 2.0 * k3[i].dvy + k4[i].dvy);
    }
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
    std::vector<CelestialBody> bodies = {
        {"Sun",   constants::M_SUN, 0, 0, 0, 0, 0, 0},
        {"Earth", 5.972e24, 1.47098074e11, 0, 0, 30300, 0, 0},
        {
            "Moon",
            7.3477e22,
            1.47098074e11 + 384400000, // position offset
            0,
            0,
            30300 + 1022,              // velocity: Earth’s + orbital
            0,
            0
        }
    };

    CelestialBody& sun   = bodies[0];
    CelestialBody& earth = bodies[1];
    CelestialBody& moon  = bodies[2];

    // Prepare output file
    std::ofstream file("orbit_three_body.csv");
    file << "step,x_sun,y_sun,x_earth,y_earth,x_moon,y_moon\n";

    const int steps = 8766; // one year (hourly)
    const double dt = constants::DT;

    for (int i = 0; i < steps; ++i) {
        rk4Step(bodies, dt);

        // Write data for visualization
        file << i << "," 
             << sun.x << "," << sun.y << ","
             << earth.x << "," << earth.y << ","
             << moon.x << "," << moon.y << "\n";
    }

    file.close();
    std::cout << "✅ Three-body simulation complete. Data saved to orbit_three_body.csv\n";
}
