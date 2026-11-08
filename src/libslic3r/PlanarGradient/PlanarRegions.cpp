#include "PlanarRegions.hpp"

#include "OrderedCells.hpp"
#include "ClipperUtils.hpp"
#include "Exception.hpp"
#include "Print.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace Slic3r::PlanarGradient {

static const ConfigOptionFloats* region_option(const ModelObject& object)
{
    const auto* option = object.config.get().option<ConfigOptionFloats>(REGION_OPTION);
    return option != nullptr && !option->values.empty() ? option : nullptr;
}

bool has_region_option(const ModelObject& object) { return region_option(object) != nullptr; }

bool region_option_changed(const ModelObject& before, const ModelObject& after)
{
    const auto* old_option = region_option(before);
    const auto* new_option = region_option(after);
    if (old_option == nullptr || new_option == nullptr)
        return old_option != new_option;
    return old_option->values != new_option->values;
}

namespace {

constexpr double ALIGNMENT_MM       = 1e-4;
constexpr int    MAX_CELLS_PER_AXIS = 128;

int aligned_cells(double extent, double pitch)
{
    const double cells = std::round(extent / pitch);
    if (cells < 1 || cells > MAX_CELLS_PER_AXIS || std::abs(extent - cells * pitch) > ALIGNMENT_MM)
        throw SlicingError("Experimental planar regions require a grid-aligned rectangular envelope with at most 128 cells per axis.");
    return static_cast<int>(cells);
}

struct MaterialRegions
{
    int a = -1;
    int b = -1;
};

MaterialRegions material_regions(const PrintObjectRegions::LayerRangeRegions& range)
{
    MaterialRegions ids;
    for (const auto& region : range.painted_regions) {
        if (region.extruder_id == 1)
            ids.a = region.region->print_object_region_id();
        else if (region.extruder_id == 2)
            ids.b = region.region->print_object_region_id();
    }
    if (ids.a < 0 || ids.b < 0 || ids.a == ids.b)
        throw SlicingError("Experimental planar regions could not resolve two distinct physical material regions.");
    return ids;
}

ExPolygons b_mask(const BoundingBox& bounds, int nx, int ny, int z, double pitch, const OrderedCells& grid, const PlanarFraction& field)
{
    Polygons cells;
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            const CellIndex index{x, y, z};
            if (grid.assign(index, field.fraction_b(grid.center_mm(index))) != Material::B)
                continue;
            const Point min = bounds.min + Point::new_scale(x * pitch, y * pitch);
            Point       max = bounds.min + Point::new_scale((x + 1) * pitch, (y + 1) * pitch);
            if (x + 1 == nx)
                max.x() = bounds.max.x();
            if (y + 1 == ny)
                max.y() = bounds.max.y();
            cells.push_back(BoundingBox(min, max).polygon());
        }
    }
    return union_ex(cells);
}

} // namespace

void apply_regions(PrintObject& object, const std::function<void()>& throw_if_canceled)
{
    const auto* option = region_option(*object.model_object());
    if (option == nullptr)
        return;

    const auto& spec = option->values;
    if (spec.size() != 4 || !std::all_of(spec.begin(), spec.end(), [](double value) { return std::isfinite(value); }) || spec[0] <= 0.0 ||
        spec[1] < 0.0 || spec[2] <= ALIGNMENT_MM || spec[3] <= ALIGNMENT_MM)
        throw SlicingError("Experimental planar regions need a positive plane offset and pitches, and a nonnegative transition width.");

    const auto& config       = object.config();
    const auto& print_config = object.print()->config();
    const auto& model        = *object.model_object();
    const auto& ranges       = object.shared_regions()->layer_ranges;
    if (print_config.filament_diameter.size() != 2 || print_config.nozzle_diameter.size() != 2 || model.volumes.size() != 1 ||
        !model.volumes.front()->is_model_part() || model.is_mm_painted() || model.is_fuzzy_skin_painted() || ranges.size() != 1 ||
        ranges.front().volume_regions.size() != 1 || config.interlocking_beam || config.elefant_foot_compensation.value != 0.0 ||
        config.xy_hole_compensation.value != 0.0 || config.xy_contour_compensation.value != 0.0 || object.layer_count() == 0)
        throw SlicingError("Experimental planar regions require two physical slots and one unpainted rectangular solid, without XY "
                           "compensation, elephant-foot compensation or interlocking.");

    const double nozzle = std::max(print_config.nozzle_diameter.get_at(0), print_config.nozzle_diameter.get_at(1));
    if (spec[2] < 2.0 * nozzle)
        throw SlicingError("Experimental planar region XY pitch must be at least twice the largest nozzle diameter.");

    const int             parent = ranges.front().volume_regions.front().region->print_object_region_id();
    const MaterialRegions ids    = material_regions(ranges.front());
    const auto            first  = to_expolygons(object.get_layer(0)->regions()[parent]->slices.surfaces);
    if (first.empty())
        throw SlicingError("Experimental planar regions require a rectangular footprint on the first layer.");
    const BoundingBox bounds = get_extents(first);
    const auto        size   = bounds.size();
    const double      width  = unscale<double>(size.x());
    const int         nx     = aligned_cells(width, spec[2]);
    const int         ny     = aligned_cells(unscale<double>(size.y()), spec[2]);
    const auto*       last   = object.get_layer(static_cast<int>(object.layer_count() - 1));
    const int         nz     = aligned_cells(last->slice_z + last->height / 2, spec[3]);
    if (spec[0] + spec[1] / 2 > width || spec[0] - spec[1] / 2 < 0.0 || spec[0] >= width)
        throw SlicingError("Experimental planar transition must fit inside the rectangular envelope.");

    const OrderedCells   grid({}, {spec[2], spec[2], spec[3]}, {});
    const PlanarFraction field({spec[0], 0, 0}, {1, 0, 0}, spec[1]);
    const double         rect_area      = double(size.x()) * double(size.y());
    const double         area_tolerance = scaled<double>(ALIGNMENT_MM) * (double(size.x()) + double(size.y()));
    ExPolygons           mask;
    int                  mask_z = -1;

    for (std::size_t i = 0; i < object.layer_count(); ++i) {
        throw_if_canceled();
        auto& layer    = *object.get_layer(static_cast<int>(i));
        auto& source   = layer.regions()[parent]->slices;
        auto  envelope = to_expolygons(source.surfaces);
        if (envelope.size() != 1 || !envelope.front().holes.empty() || get_extents(envelope).min != bounds.min ||
            get_extents(envelope).max != bounds.max || std::abs(area(envelope) - rect_area) > area_tolerance)
            throw SlicingError("Experimental planar regions require the same axis-aligned rectangular footprint on every layer.");

        const int z = static_cast<int>(std::floor(layer.slice_z / spec[3]));
        if (z < 0 || z >= nz || layer.slice_z - layer.height / 2 < z * spec[3] - ALIGNMENT_MM ||
            layer.slice_z + layer.height / 2 > (z + 1) * spec[3] + ALIGNMENT_MM)
            throw SlicingError("Experimental planar regions require each slice layer to fit inside one Z cell band.");
        if (z != mask_z) {
            mask   = b_mask(bounds, nx, ny, z, spec[2], grid, field);
            mask_z = z;
        }
        auto b = intersection_ex(envelope, mask);
        auto a = diff_ex(envelope, b);
        source.clear();
        layer.regions()[ids.a]->slices.append(std::move(a), stInternal);
        layer.regions()[ids.b]->slices.append(std::move(b), stInternal);
    }
}

} // namespace Slic3r::PlanarGradient
