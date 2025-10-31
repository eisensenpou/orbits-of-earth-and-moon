


#ifndef SIMULATION_H
#define SIMULATION_H

#include <iostream>
#include "body.h"

void computeAcceleration(CelestialBody& earth, const CelestialBody& sun);
void eulerStep(CelestialBody& body, double dt);
void runSimulation();

#endif //SIMULATION_H