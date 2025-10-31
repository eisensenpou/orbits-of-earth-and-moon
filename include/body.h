/****************
*Author: Sinan Demir   
*File: body.h
*Purpose: 
*****************/


#ifndef BODY_H
#define BODY_H

#include <string>

struct CelestialBody{
    std::string name;
    double mass;
    double x,y;     // positions
    double vx, vy;  // velocity components
    double ax, ay;  // accelarator components
};

#endif //BODY_H