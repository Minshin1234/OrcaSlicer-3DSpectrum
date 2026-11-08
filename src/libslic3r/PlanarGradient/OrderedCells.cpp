#include "OrderedCells.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace Slic3r::PlanarGradient {

OrderedCells::OrderedCells(Point3 origin_mm, Point3 pitch_mm, CellIndex phase)
    : m_origin_mm(origin_mm), m_pitch_mm(pitch_mm), m_phase(phase)
{
    for (std::size_t axis = 0; axis < m_origin_mm.size(); ++axis) {
        if (!std::isfinite(m_origin_mm[axis]) || !std::isfinite(m_pitch_mm[axis]) || m_pitch_mm[axis] <= 0.0)
            throw std::invalid_argument("Ordered cells need a finite origin and finite positive pitch");
    }
}

Point3 OrderedCells::center_mm(const CellIndex& cell) const
{
    Point3 center{};
    for (std::size_t axis = 0; axis < center.size(); ++axis) {
        center[axis] = std::fma(static_cast<double>(cell[axis]) + 0.5, m_pitch_mm[axis], m_origin_mm[axis]);
        if (!std::isfinite(center[axis]))
            throw std::overflow_error("Ordered cell center exceeds numeric range");
    }
    return center;
}

std::optional<Material> OrderedCells::assign(const CellIndex& cell, std::optional<double> fraction_b) const
{
    if (!fraction_b)
        return std::nullopt;
    if (!std::isfinite(*fraction_b) || *fraction_b < 0.0 || *fraction_b > 1.0)
        throw std::invalid_argument("Ordered cells need a finite material fraction in [0, 1]");

    std::array<std::uint32_t, 3> wrapped{};
    for (std::size_t axis = 0; axis < wrapped.size(); ++axis)
        wrapped[axis] = (static_cast<std::uint32_t>(cell[axis]) + static_cast<std::uint32_t>(m_phase[axis])) & 3u;

    // Rank each octant once. Low coordinate bits select the major digit to spread nearby ranks.
    constexpr std::array<unsigned, 8> OCTANT_RANK{0, 4, 6, 2, 7, 3, 1, 5};
    const auto                        octant = [&wrapped](unsigned bit) {
        return ((wrapped[0] >> bit) & 1u) | (((wrapped[1] >> bit) & 1u) << 1) | (((wrapped[2] >> bit) & 1u) << 2);
    };
    const unsigned rank = 8 * OCTANT_RANK[octant(0)] + OCTANT_RANK[octant(1)];
    return *fraction_b >= (rank + 0.5) / 64.0 ? Material::B : Material::A;
}

} // namespace Slic3r::PlanarGradient
