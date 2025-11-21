/**********
 * barycenter.h
 * Author: Sinan Demir
 * Date: 11/20/2025
 * Purpose: Declares barycenter normalization function
 ***********/

#pragma once
#include <vector>
#include "body.h"
#include "vec3.h"

namespace physics {

void normalizeToBarycenter(std::vector<CelestialBody>& bodies);

} // end namespace physics
