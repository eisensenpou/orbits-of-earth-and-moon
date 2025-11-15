/****************
 * Author: Sinan Demir
 * File: utils.h
 * Date: 10/31/2025
 * Purpose: Defines physical constants in constants namespace
 *****************/
#ifndef UTILS_H
#define UTILS_H

#include <cmath>

namespace constants {
    constexpr double G      = 6.67430e-11;
    constexpr double M_SUN  = 1.9891e30;
    constexpr double DT     = 3600;         // one hour in seconds
    constexpr double MOON_INCLINATION = 5.145 * (M_PI / 180.0); // radians
};


#endif //UTILS_H