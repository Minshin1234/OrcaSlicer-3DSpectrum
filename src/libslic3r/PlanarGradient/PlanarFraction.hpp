#pragma once

#include <array>

namespace Slic3r::PlanarGradient {

using Point3 = std::array<double, 3>;

// Coordinates and transition width are in millimeters in the same specimen frame.
// The normal points from material A to B and is normalized during construction.
// The field is unbounded. Callers decide where the interface applies.
class PlanarFraction
{
public:
    // Throws std::invalid_argument for non-finite inputs, a zero normal or a negative width.
    PlanarFraction(Point3 plane_point_mm, Point3 normal_a_to_b, double width_mm);

    // Returns the B fraction. At zero width the plane itself belongs to B.
    // Throws std::invalid_argument for non-finite points and std::overflow_error
    // if the signed-distance calculation overflows.
    double fraction_b(const Point3& point_mm) const;

private:
    Point3 m_plane_point_mm;
    Point3 m_unit_normal;
    double m_width_mm;
};

} // namespace Slic3r::PlanarGradient
