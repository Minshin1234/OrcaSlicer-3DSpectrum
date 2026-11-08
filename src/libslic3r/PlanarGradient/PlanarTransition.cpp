#include "PlanarTransition.hpp"

#include "ClipperUtils.hpp"
#include "Exception.hpp"
#include "ExtrusionEntityCollection.hpp"
#include "Layer.hpp"
#include "PlanarRegions.hpp"
#include "Print.hpp"

#include <algorithm>
#include <cmath>

namespace Slic3r::PlanarGradient {

namespace {

const std::vector<double>* values(const ModelObject& object)
{
    const auto* option = object.config.get().option<ConfigOptionFloats>(TRANSITION_OPTION);
    return option && !option->values.empty() ? &option->values : nullptr;
}

Point3 point3(const Vec3d& v) { return {v.x(), v.y(), v.z()}; }

// A multipart transition is limited to a fully shared planar interface. Both
// bodies must occupy opposite half-spaces within the shared face's projection.
constexpr double contact_tolerance = 1e-4;

bool shared_planar_interface(const ModelObject& object, const Transform3d& transform, const Vec3d& point, const Vec3d& normal)
{
    const Vec3d u = normal.unitOrthogonal();
    const Vec3d v = normal.cross(u);
    ExPolygons faces[2];
    for (const ModelVolume* volume : object.volumes) {
        const Transform3d matrix = transform * volume->get_matrix();
        const auto& mesh = volume->mesh().its;
        Points projected;
        std::vector<double> distances;
        bool below = false, above = false;
        for (const Vec3f& vertex : mesh.vertices) {
            const Vec3d delta = matrix * vertex.cast<double>() - point;
            const double distance = normal.dot(delta);
            projected.push_back(Point::new_scale(u.dot(delta), v.dot(delta)));
            distances.push_back(distance);
            below |= distance < -contact_tolerance;
            above |= distance > contact_tolerance;
        }
        if (below == above)
            return false;

        Polygons face, footprint;
        for (const auto& triangle : mesh.indices) {
            Polygon polygon(Points{projected[triangle[0]], projected[triangle[1]], projected[triangle[2]]});
            if (polygon.area() == 0.)
                continue;
            polygon.make_counter_clockwise();
            if (std::abs(distances[triangle[0]]) <= contact_tolerance &&
                std::abs(distances[triangle[1]]) <= contact_tolerance &&
                std::abs(distances[triangle[2]]) <= contact_tolerance)
                face.push_back(polygon);
            footprint.push_back(std::move(polygon));
        }
        const size_t side = above ? 1 : 0;
        if (face.empty() || !faces[side].empty())
            return false;
        faces[side] = union_ex(face);
        if (!diff_ex(union_ex(footprint), offset_ex(faces[side], scaled<float>(contact_tolerance))).empty())
            return false;
    }
    return !faces[0].empty() && !faces[1].empty() &&
        !intersection_ex(faces[0], faces[1]).empty() &&
        diff_ex(faces[0], offset_ex(faces[1], scaled<float>(contact_tolerance))).empty() &&
        diff_ex(faces[1], offset_ex(faces[0], scaled<float>(contact_tolerance))).empty();
}

Vec3d slicing_normal(const PrintObject& object, const Transition& transition)
{
    return (object.trafo().linear().inverse().transpose() * transition.normal).normalized();
}

double xy_pitch(const PrintObject& object, const Transition& transition)
{
    const auto& nozzles = object.print()->config().nozzle_diameter;
    return 3. * std::max(nozzles.get_at(transition.filaments[0] - 1), nozzles.get_at(transition.filaments[1] - 1));
}

void add_crossing(double start, double end, double boundary, std::vector<double>& cuts)
{
    if (boundary > std::min(start, end) && boundary < std::max(start, end))
        cuts.push_back((boundary - start) / (end - start));
}

Point interpolated_point(const Point& first, const Point& last, double t)
{
    const Vec2d point = first.cast<double>() + t * (last - first).cast<double>();
    return Point(coord_t(std::llround(point.x())), coord_t(std::llround(point.y())));
}

} // namespace

std::optional<PlanarInterface> find_shared_planar_interface(const ModelObject& object, const Transform3d& transform)
{
    if (object.volumes.size() != 2 ||
        !std::all_of(object.volumes.begin(), object.volumes.end(), [](const ModelVolume* volume) { return volume->is_model_part(); }))
        return std::nullopt;

    std::array<std::vector<Vec3d>, 2> vertices;
    std::array<Vec3d, 2> centers;
    for (size_t side = 0; side < vertices.size(); ++side) {
        const ModelVolume& volume = *object.volumes[side];
        const auto& mesh = volume.mesh().its;
        if (mesh.vertices.empty())
            return std::nullopt;
        const Transform3d matrix = transform * volume.get_matrix();
        vertices[side].reserve(mesh.vertices.size());
        centers[side].setZero();
        for (const Vec3f& vertex : mesh.vertices) {
            vertices[side].push_back(matrix * vertex.cast<double>());
            centers[side] += vertices[side].back();
        }
        centers[side] /= double(vertices[side].size());
    }

    // A fully shared face is a supporting plane for both meshes. Look for
    // triangles inside the other body's bounds before checking that plane.
    const size_t source = vertices[0].size() <= vertices[1].size() ? 0 : 1;
    BoundingBoxf3 other_bounds;
    for (const Vec3d& vertex : vertices[1 - source])
        other_bounds.merge(vertex);
    other_bounds.offset(2. * contact_tolerance);

    std::vector<PlanarInterface> tested;
    for (const auto& triangle : object.volumes[source]->mesh().its.indices) {
        const Vec3d& a = vertices[source][triangle[0]];
        const Vec3d& b = vertices[source][triangle[1]];
        const Vec3d& c = vertices[source][triangle[2]];
        if (!other_bounds.contains(a) || !other_bounds.contains(b) || !other_bounds.contains(c))
            continue;
        Vec3d normal = (b - a).cross(c - a);
        if (normal.squaredNorm() == 0.)
            continue;
        normal.normalize();
        if (normal.dot(centers[1] - centers[0]) < 0.)
            normal = -normal;
        if (normal.dot(centers[0] - a) >= 0. || normal.dot(centers[1] - a) <= 0.)
            continue;
        if (std::any_of(tested.begin(), tested.end(), [&](const PlanarInterface& plane) {
                return normal.isApprox(plane.normal, 1e-10) &&
                    std::abs(normal.dot(a - plane.point)) < contact_tolerance * 0.01;
            }))
            continue;
        if (std::any_of(vertices[0].begin(), vertices[0].end(), [&](const Vec3d& p) { return normal.dot(p - a) > contact_tolerance; }) ||
            std::any_of(vertices[1].begin(), vertices[1].end(), [&](const Vec3d& p) { return normal.dot(p - a) < -contact_tolerance; }))
            continue;
        tested.push_back({a, normal});
        if (!shared_planar_interface(object, transform, a, normal))
            continue;

        Vec3d center = Vec3d::Zero();
        size_t count = 0;
        for (const Vec3d& vertex : vertices[source]) {
            if (std::abs(normal.dot(vertex - a)) <= contact_tolerance) {
                center += vertex;
                ++count;
            }
        }
        center /= double(count);
        center -= normal.dot(center - a) * normal;
        return PlanarInterface{transform.inverse() * center, (transform.linear().transpose() * normal).normalized()};
    }
    return std::nullopt;
}

std::vector<double> Transition::serialize() const
{
    return {point.x(), point.y(), point.z(), normal.x(), normal.y(), normal.z(), width,
            double(filaments[0]), double(filaments[1])};
}

bool has_transition(const ModelObject& object) { return values(object) != nullptr; }

bool transition_changed(const ModelObject& before, const ModelObject& after)
{
    const auto* a = values(before);
    const auto* b = values(after);
    return a && b ? *a != *b : a != b;
}

std::optional<Transition> read_transition(const ModelObject& object)
{
    const auto* data = values(object);
    if (!data)
        return std::nullopt;
    const auto& v = *data;
    auto valid_slot = [](double slot) { return slot >= 1. && slot <= 64. && std::floor(slot) == slot; };
    if (v.size() != 9 || !std::all_of(v.begin(), v.end(), [](double n) { return std::isfinite(n); }) ||
        v[6] <= 0. || !valid_slot(v[7]) || !valid_slot(v[8]) || v[7] == v[8])
        return std::nullopt;

    Vec3d normal(v[3], v[4], v[5]);
    const double magnitude = normal.stableNorm();
    if (!std::isfinite(magnitude) || magnitude == 0.)
        return std::nullopt;
    return Transition{Vec3d(v[0], v[1], v[2]), normal / magnitude, v[6],
                      {static_cast<unsigned int>(v[7]), static_cast<unsigned int>(v[8])}};
}

void collect_transition_extruders(const PrintObject& object, std::vector<unsigned int>& extruders)
{
    if (const auto transition = read_transition(*object.model_object()))
        for (unsigned int slot : transition->filaments)
            if (slot <= object.print()->config().filament_diameter.size())
                extruders.push_back(slot - 1);
}

void validate_transition(const PrintObject& object)
{
    const ModelObject& model = *object.model_object();
    if (!has_transition(model))
        return;
    const auto transition = read_transition(model);
    if (!transition)
        throw SlicingError("Material transition needs a finite plane, a positive width and two distinct physical filament slots.");

    const auto& print = object.print()->config();
    for (unsigned int slot : transition->filaments)
        if (slot > print.filament_diameter.size() || print.filament_soluble.get_at(slot - 1) || print.filament_is_support.get_at(slot - 1))
            throw SlicingError("Choose two available, non-support filaments for the material transition.");
    if (model.volumes.empty() || model.volumes.size() > 2 ||
        !std::all_of(model.volumes.begin(), model.volumes.end(), [](const ModelVolume* volume) { return volume->is_model_part(); }) ||
        model.is_mm_painted() || model.is_fuzzy_skin_painted() || has_region_option(model) ||
        print.spiral_mode || object.config().interlocking_beam || object.config().flush_into_objects || object.config().flush_into_infill)
        throw SlicingError("Material transition needs one solid or two touching solid parts, without material painting, experimental regions, spiral mode, interlocking or flushing into model paths.");

    if (model.volumes.size() == 2) {
        const Transform3d transform = object.trafo_centered();
        const Vec3d normal = (transform.linear().inverse().transpose() * transition->normal).normalized();
        if (!shared_planar_interface(model, transform, transform * transition->point, normal))
            throw SlicingError("For two solid parts, place the transition plane on their fully shared planar face.");
        for (const auto& range : object.shared_regions()->layer_ranges) {
            if (range.volume_regions.size() != 2 ||
                range.volume_regions[0].region->print_object_region_id() != range.volume_regions[1].region->print_object_region_id())
                throw SlicingError("The two interface parts need the same base print settings. Choose their filaments in Material transition.");
        }
    }

    const auto& nozzles = print.nozzle_diameter;
    const double nozzle_a = nozzles.get_at(transition->filaments[0] - 1);
    const double nozzle_b = nozzles.get_at(transition->filaments[1] - 1);
    if (nozzle_a <= 0. || std::abs(nozzle_a - nozzle_b) > EPSILON)
        throw SlicingError("The material transition requires matching nozzle diameters.");
}

LayerTransition::LayerTransition(const PrintObject& object, const Layer& layer, const Transition& transition)
    : m_plane_point(object.trafo_centered() * transition.point),
      m_normal(slicing_normal(object, transition)),
      m_z(layer.slice_z),
      m_pitch(xy_pitch(object, transition)),
      m_z_pitch(object.config().layer_height.value),
      m_width(transition.width),
      m_field(point3(m_plane_point), point3(m_normal), m_width),
      m_cells({}, {m_pitch, m_pitch, m_z_pitch}, {}),
      m_filaments(transition.filaments)
{
    const BoundingBox bounds = get_extents(layer.lslices);
    if (!bounds.defined)
        return;
    const Vec2d lo = unscale(bounds.min).array() - m_pitch;
    const Vec2d hi = unscale(bounds.max).array() + m_pitch;
    const Vec3d min_point(m_normal.x() >= 0. ? lo.x() : hi.x(), m_normal.y() >= 0. ? lo.y() : hi.y(), m_z);
    const Vec3d max_point(m_normal.x() >= 0. ? hi.x() : lo.x(), m_normal.y() >= 0. ? hi.y() : lo.y(), m_z);
    if (m_field.fraction_b(point3(max_point)) == 0.)
        m_pure_material = 0;
    else if (m_field.fraction_b(point3(min_point)) == 1.)
        m_pure_material = 1;
}

size_t LayerTransition::material_at(const Vec2d& point) const
{
    const CellIndex cell{int32_t(std::floor(point.x() / m_pitch)),
                         int32_t(std::floor(point.y() / m_pitch)),
                         int32_t(std::floor(m_z / m_z_pitch))};
    const double fraction = m_field.fraction_b({point.x(), point.y(), m_z});
    return m_cells.assign(cell, fraction) == Material::B ? 1 : 0;
}

void LayerTransition::partition(const ExtrusionEntityCollection& source, ExtrusionEntityCollection& a,
                                ExtrusionEntityCollection& b) const
{
    if (m_pure_material) {
        (*m_pure_material == 0 ? a : b) = source;
        return;
    }
    std::array<ExtrusionEntityCollection*, 2> output{&a, &b};
    a.no_sort = b.no_sort = source.no_sort;

    // Split centerlines at shared endpoints, without generating new contours or changing bead dimensions.
    auto split_path = [this](const ExtrusionPath& path, ExtrusionEntityCollection& a_paths, ExtrusionEntityCollection& b_paths) {
        std::array<ExtrusionEntityCollection*, 2> paths{&a_paths, &b_paths};
        std::array<Polylines, 2> fragments;
        std::optional<size_t> previous;
        const Points& points = path.polyline.points;
        std::vector<double> cuts;
        for (size_t i = 1; i < points.size(); ++i) {
            const Vec2d start = unscale(points[i - 1]);
            const Vec2d end = unscale(points[i]);
            cuts.assign({0., 1.});
            for (int axis = 0; axis < 2; ++axis) {
                const double low = std::min(start[axis], end[axis]);
                const double high = std::max(start[axis], end[axis]);
                for (double grid = std::floor(low / m_pitch) + 1.; grid * m_pitch < high; ++grid)
                    add_crossing(start[axis], end[axis], grid * m_pitch, cuts);
            }
            const double d0 = m_normal.dot(Vec3d(start.x(), start.y(), m_z) - m_plane_point);
            const double d1 = m_normal.dot(Vec3d(end.x(), end.y(), m_z) - m_plane_point);
            add_crossing(d0, d1, -0.5 * m_width, cuts);
            add_crossing(d0, d1, 0.5 * m_width, cuts);
            std::sort(cuts.begin(), cuts.end());
            bool previous_in_segment = false;
            for (size_t j = 1; j < cuts.size(); ++j) {
                const Point from = interpolated_point(points[i - 1], points[i], cuts[j - 1]);
                const Point to = interpolated_point(points[i - 1], points[i], cuts[j]);
                if (from == to)
                    continue;
                const size_t material = material_at(start + (end - start) * (0.5 * (cuts[j - 1] + cuts[j])));
                auto& lines = fragments[material];
                if (previous == material) {
                    // A grid crossing without a tool change need not become a G-code move.
                    if (previous_in_segment)
                        lines.back().points.back() = to;
                    else
                        lines.back().points.push_back(to);
                } else {
                    lines.emplace_back(Points{from, to});
                }
                previous = material;
                previous_in_segment = true;
            }
        }
        if (!previous)
            return;
        if (fragments[0].empty() || fragments[1].empty()) {
            paths[*previous]->append(path);
            return;
        }
        if (dynamic_cast<const ExtrusionPathSloped*>(&path))
            throw SlicingError("Material transition cannot divide a sloped extrusion path.");
        for (size_t material = 0; material < paths.size(); ++material)
            for (auto& line : fragments[material]) {
                if (dynamic_cast<const ExtrusionPathOriented*>(&path)) {
                    auto fragment = static_cast<const ExtrusionPathOriented&>(path);
                    fragment.polyline = std::move(line);
                    paths[material]->append(fragment);
                } else {
                    paths[material]->append(ExtrusionPath(std::move(line), path));
                }
            }
    };

    auto split_group = [&](const ExtrusionEntity& group, const ExtrusionPaths& paths) {
        ExtrusionEntityCollection fragments[2];
        for (const auto& path : paths)
            split_path(path, fragments[0], fragments[1]);
        const bool mixed = !fragments[0].empty() && !fragments[1].empty();
        for (size_t material = 0; material < output.size(); ++material) {
            if (fragments[material].empty())
                continue;
            if (mixed)
                output[material]->append(std::move(fragments[material].entities));
            else
                output[material]->append(group);
        }
    };

    for (const ExtrusionEntity* entity : source.entities) {
        if (const auto* path = dynamic_cast<const ExtrusionPath*>(entity))
            split_path(*path, a, b);
        else if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(entity))
            split_group(*loop, loop->paths);
        else if (const auto* multipath = dynamic_cast<const ExtrusionMultiPath*>(entity))
            split_group(*multipath, multipath->paths);
        else if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(entity)) {
            ExtrusionEntityCollection children[2];
            partition(*collection, children[0], children[1]);
            for (size_t material = 0; material < output.size(); ++material)
                if (!children[material].empty())
                    output[material]->append(std::move(children[material]));
        } else {
            throw SlicingError("Material transition encountered an unsupported model extrusion path.");
        }
    }
}

} // namespace Slic3r::PlanarGradient
