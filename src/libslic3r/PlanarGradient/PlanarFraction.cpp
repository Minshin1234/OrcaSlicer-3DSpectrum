#include "PlanarFraction.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace Slic3r::PlanarGradient {

static bool is_finite(const Point3& point)
{
    return std::all_of(point.begin(), point.end(), [](double value) { return std::isfinite(value); });
}

PlanarFraction::PlanarFraction(Point3 plane_point_mm, Point3 normal_a_to_b, double width_mm)
    : m_plane_point_mm(plane_point_mm), m_unit_normal(normal_a_to_b), m_width_mm(width_mm)
{
    if (!is_finite(plane_point_mm) || !is_finite(normal_a_to_b) || !std::isfinite(width_mm) || width_mm < 0.0)
        throw std::invalid_argument("Planar gradient needs finite inputs and a nonnegative width");

    const double scale = std::max({std::abs(m_unit_normal[0]), std::abs(m_unit_normal[1]), std::abs(m_unit_normal[2])});
    if (scale == 0.0)
        throw std::invalid_argument("Planar gradient needs a nonzero normal");

    // Scale first so the norm cannot underflow or overflow.
    for (double& component : m_unit_normal)
        component /= scale;
    const double length = std::hypot(m_unit_normal[0], m_unit_normal[1], m_unit_normal[2]);
    for (double& component : m_unit_normal)
        component /= length;
}

double PlanarFraction::fraction_b(const Point3& point_mm) const
{
    if (!is_finite(point_mm))
        throw std::invalid_argument("Planar gradient sample needs a finite point");

    double signed_distance_mm = 0.0;
    for (std::size_t axis = 0; axis < m_unit_normal.size(); ++axis) {
        if (m_unit_normal[axis] != 0.0)
            signed_distance_mm += (point_mm[axis] - m_plane_point_mm[axis]) * m_unit_normal[axis];
    }
    if (!std::isfinite(signed_distance_mm))
        throw std::overflow_error("Planar gradient signed distance exceeds numeric range");

    if (m_width_mm == 0.0)
        return signed_distance_mm < 0.0 ? 0.0 : 1.0;

    return std::clamp(0.5 + signed_distance_mm / m_width_mm, 0.0, 1.0);
}

} // namespace Slic3r::PlanarGradient
