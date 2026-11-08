#pragma once

#include "OrderedCells.hpp"
#include "Point.hpp"

#include <array>
#include <optional>
#include <vector>

namespace Slic3r {
class ExtrusionEntityCollection;
class Layer;
class ModelObject;
class PrintObject;

namespace PlanarGradient {

inline constexpr char TRANSITION_OPTION[] = "planar_material_transition";

struct Transition
{
    // Object-local point and normal. Width is measured in physical millimeters.
    Vec3d point;
    Vec3d normal;
    double width;
    std::array<unsigned int, 2> filaments; // A, B, one-based physical slots.

    std::vector<double> serialize() const;
};

struct PlanarInterface
{
    // Object-local plane, pointing from the first solid toward the second.
    Vec3d point;
    Vec3d normal;
};

std::optional<PlanarInterface> find_shared_planar_interface(const ModelObject& object, const Transform3d& transform);
bool has_transition(const ModelObject& object);
bool transition_changed(const ModelObject& before, const ModelObject& after);
std::optional<Transition> read_transition(const ModelObject& object);
void validate_transition(const PrintObject& object);
void collect_transition_extruders(const PrintObject& object, std::vector<unsigned int>& extruders);

// Builds the assignment in the same centered slicing frame as the model paths.
// An absent second slot means the entire layer is beyond one edge of the band.
class LayerTransition
{
public:
    LayerTransition(const PrintObject& object, const Layer& layer, const Transition& transition);

    const std::array<unsigned int, 2>& filaments() const { return m_filaments; }
    std::optional<size_t> pure_material() const { return m_pure_material; }
    void partition(const ExtrusionEntityCollection& source, ExtrusionEntityCollection& a, ExtrusionEntityCollection& b) const;

private:
    size_t material_at(const Vec2d& point) const;

    Vec3d m_plane_point;
    Vec3d m_normal;
    double m_z;
    double m_pitch;
    double m_z_pitch;
    double m_width;
    PlanarFraction m_field;
    OrderedCells m_cells;
    std::array<unsigned int, 2> m_filaments;
    std::optional<size_t> m_pure_material;
};

} // namespace PlanarGradient
} // namespace Slic3r
