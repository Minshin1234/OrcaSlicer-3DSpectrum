#pragma once

#include <functional>

namespace Slic3r {
class ModelObject;
class PrintObject;

namespace PlanarGradient {

// [plane X from envelope minimum, transition width, XY pitch, Z pitch], in mm.
inline constexpr char REGION_OPTION[] = "experimental_planar_material_regions";

bool has_region_option(const ModelObject& object);
bool region_option_changed(const ModelObject& before, const ModelObject& after);
void apply_regions(PrintObject& object, const std::function<void()>& throw_if_canceled);

} // namespace PlanarGradient
} // namespace Slic3r
