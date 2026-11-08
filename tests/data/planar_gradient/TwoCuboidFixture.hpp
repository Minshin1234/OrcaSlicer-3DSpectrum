#pragma once

#include "PlanarFraction.hpp"

#include <cstddef>

namespace Slic3r::PlanarGradient::TestFixtures {

struct Cuboid
{
    Point3 min_mm;
    Point3 max_mm;
};

// Distinct solid bodies in one frame. Only their faces at x = 12 mm touch.
inline constexpr Cuboid MATERIAL_A{{4.0, -3.0, 1.0}, {12.0, 3.0, 5.0}};
inline constexpr Cuboid MATERIAL_B{{12.0, -3.0, 1.0}, {22.0, 3.0, 5.0}};
inline constexpr Point3 PLANE_POINT_MM{12.0, 0.0, 3.0};
inline constexpr Point3 NORMAL_A_TO_B{1.0, 0.0, 0.0};

inline bool contains(const Cuboid& box, const Point3& point)
{
    for (std::size_t axis = 0; axis < point.size(); ++axis)
        if (point[axis] < box.min_mm[axis] || point[axis] >= box.max_mm[axis])
            return false;
    return true;
}

} // namespace Slic3r::PlanarGradient::TestFixtures
