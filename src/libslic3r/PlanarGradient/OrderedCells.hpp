#pragma once

#include "PlanarFraction.hpp"

#include <cstdint>
#include <optional>

namespace Slic3r::PlanarGradient {

using CellIndex = std::array<std::int32_t, 3>;

enum class Material { A, B };

class OrderedCells
{
public:
    // Origin is the lower corner of cell (0, 0, 0). Pitch is positive on each axis.
    // Phase shifts the repeating thresholds in index space, without moving the grid.
    OrderedCells(Point3 origin_mm, Point3 pitch_mm, CellIndex phase);

    Point3 center_mm(const CellIndex& cell) const;

    // A missing fraction represents an empty cell. Occupied fractions must be finite in [0, 1].
    // Uniform fractions have at most 1/128 absolute error over a complete 4x4x4 period.
    std::optional<Material> assign(const CellIndex& cell, std::optional<double> fraction_b) const;

private:
    Point3    m_origin_mm;
    Point3    m_pitch_mm;
    CellIndex m_phase;
};

} // namespace Slic3r::PlanarGradient
