


#ifndef SIMULATION_H
#define SIMULATION_H

#include <iostream>
#include "body.h"
#include <vector>

//void computeAcceleration(CelestialBody& earth, const CelestialBody& sun);
void computeGravitationalForce(CelestialBody& a, CelestialBody& b);
void eulerStep(CelestialBody& body, double dt);
void runSimulation();

#endif //SIMULATION_H