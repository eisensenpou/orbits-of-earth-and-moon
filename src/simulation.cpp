

#include "simulation.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <fstream>  // for CSV output

void computeAcceleration(CelestialBody& earth, const CelestialBody& sun) {
    /***********************
     * computeAcceleration
     * @brief: this function calculates acceleation
     * @param: CelestialBody reference for earth
     * @param: CelestialBody reference for sun
     * @exception: none
     * @return: none
     * @note:
     ******************/
    double dx = earth.x - sun.x;
    double dy = earth.y - sun.y;
    double r3 = std::pow(dx*dx + dy*dy, 1.5);

    earth.ax = -constants::G * sun.mass * dx / r3;
    earth.ay = -constants::G * sun.mass * dy / r3;
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
     * @brief: runs the simulation for eart and sun, writes the data to csv.
     * @param none
     * @exception none
     * @return none
     * @note
     *********************/
    CelestialBody sun   {"Sun", constants::M_SUN, 0, 0, 0, 0, 0, 0};
    CelestialBody earth {"Earth", 5.972e24, 1.47098074e11, 0, 0, 30300, 0, 0};

    std::ofstream file("orbit.csv");
    file << "step,x,y,vx,vy,ax,ay\n";

    const int steps = 8766; // one year in hours

    for (int i = 0; i < steps; ++i) {
        computeAcceleration(earth, sun);
        eulerStep(earth, constants::DT);
        file << i << "," << earth.x << "," << earth.y << ","
             << earth.vx << "," << earth.vy << ","
             << earth.ax << "," << earth.ay << "\n";
    }

    file.close();
    std::cout << "✅ Simulation complete. Data saved to orbit.csv\n";
 }
