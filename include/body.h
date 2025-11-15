/****************
 * Author: Sinan Demir
 * File: body.h
 * Date: 10/31/2025
 * Purpose: Defines CelestialBody ADT
 *****************/


#ifndef BODY_H
#define BODY_H

#include <string>

struct CelestialBody{
    std::string name;
    double mass;
    double x,y,z;     // positions
    double vx, vy, vz;  // velocity components
    double ax, ay, az;  // accelarator components
}; //end CelestialBody struct.

#endif //BODY_H