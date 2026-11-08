#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../data/planar_gradient/TwoCuboidFixture.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCodeReader.hpp"
#include "libslic3r/PlanarGradient/OrderedCells.hpp"
#include "libslic3r/PlanarGradient/PlanarRegions.hpp"
#include "libslic3r/PlanarGradient/PlanarTransition.hpp"
#include "libslic3r/Exception.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Format/STEP.hpp"
#include "libslic3r/TriangleSelector.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <STEPControl_Writer.hxx>
#include <boost/filesystem.hpp>

#include <array>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <set>

using namespace Slic3r;
using namespace Slic3r::PlanarGradient;
using namespace Slic3r::PlanarGradient::TestFixtures;
using Catch::Approx;

namespace {

constexpr int    NX           = static_cast<int>(MATERIAL_B.max_mm[0] - MATERIAL_A.min_mm[0]);
constexpr int    NY           = static_cast<int>(MATERIAL_B.max_mm[1] - MATERIAL_A.min_mm[1]);
constexpr int    NZ           = static_cast<int>(MATERIAL_B.max_mm[2] - MATERIAL_A.min_mm[2]);
constexpr double LAYER_HEIGHT = 0.2;

enum class Route { Control, Volumes, Paint, Direct, InvalidPitch };

std::array<int, NZ> add_material_volumes(ModelObject& object)
{
    const OrderedCells   grid(MATERIAL_A.min_mm, {1, 1, 1}, {});
    const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, 4.0);
    std::array<int, NZ>  b_cells{};
    for (int z = 0; z < NZ; ++z) {
        for (int y = 0; y < NY; ++y) {
            std::array<Material, NX> row{};
            for (int x = 0; x < NX; ++x) {
                const CellIndex index{x, y, z};
                const auto      center = grid.center_mm(index);
                REQUIRE((contains(MATERIAL_A, center) || contains(MATERIAL_B, center)));
                row[x] = *grid.assign(index, field.fraction_b(center));
                b_cells[z] += row[x] == Material::B;
            }
            // Coalesce consecutive cells along X before passing them to the ordinary volume slicer.
            for (int start = 0; start < NX;) {
                int end = start + 1;
                while (end < NX && row[end] == row[start])
                    ++end;
                TriangleMesh mesh = make_cube(end - start, 1., 1.);
                mesh.translate(static_cast<float>(start), static_cast<float>(y), static_cast<float>(z));
                auto* volume = object.add_volume(std::move(mesh));
                volume->config.set_key_value("extruder", new ConfigOptionInt(row[start] == Material::B ? 2 : 1));
                start = end;
            }
        }
    }
    return b_cells;
}

void paint_end(ModelVolume& volume)
{
    const auto&      mesh = volume.mesh().its;
    TriangleSelector selector(volume.mesh());
    const double     max_x   = volume.mesh().bounding_box().max.x();
    int              painted = 0;
    for (std::size_t i = 0; i < mesh.indices.size(); ++i) {
        const auto& face = mesh.indices[i];
        if (mesh.vertices[face[0]].x() == max_x && mesh.vertices[face[1]].x() == max_x && mesh.vertices[face[2]].x() == max_x) {
            selector.set_facet(static_cast<int>(i), EnforcerBlockerType::Extruder2);
            ++painted;
        }
    }
    REQUIRE(painted == 2);
    REQUIRE(volume.mmu_segmentation_facets.set(selector));
}

double area_mm2(const ExPolygons& polygons) { return area(polygons) * SCALING_FACTOR * SCALING_FACTOR; }

constexpr int    LAYERS     = 20;
constexpr double BEAD_WIDTH = 0.45;

struct ToolpathMetrics
{
    std::array<double, 2> commanded_volume{};
    double                upper_path_mm          = 0.0;
    double                uncovered_mm           = 0.0;
    double                uncovered_nonbridge_mm = 0.0;
};

ToolpathMetrics audit_deposition(const GCodeProcessorResult& result)
{
    std::array<Polygons, LAYERS>  footprints;
    std::array<Polylines, LAYERS> bridges;
    std::array<Polylines, LAYERS> ordinary;
    ToolpathMetrics               metrics;
    const double                  filament_area = PI * 1.75 * 1.75 / 4.0;

    for (std::size_t i = 1; i < result.moves.size(); ++i) {
        const auto& move = result.moves[i];
        const auto  role = move.extrusion_role;
        if (move.type != EMoveType::Extrude || !(is_perimeter(role) || is_infill(role) || role == erGapFill))
            continue;

        const auto& from  = result.moves[i - 1].position;
        const auto& to    = move.position;
        const int   layer = static_cast<int>(std::lround(to.z() / LAYER_HEIGHT)) - 1;
        REQUIRE((layer >= 0 && layer < LAYERS));
        REQUIRE(std::abs(to.z() - (layer + 1) * LAYER_HEIGHT) < 0.001);
        REQUIRE(std::abs(from.z() - to.z()) < 0.001);
        REQUIRE_FALSE(move.is_arc_move_with_interpolation_points());
        REQUIRE(move.extruder_id < metrics.commanded_volume.size());
        REQUIRE(move.width > 0.0f);
        REQUIRE(move.delta_extruder > 0.0f);

        metrics.commanded_volume[move.extruder_id] += move.delta_extruder * filament_area;
        const Point start = Point::new_scale(from.x(), from.y());
        const Point end   = Point::new_scale(to.x(), to.y());
        if (start == end)
            continue;
        const Polyline line(start, end);
        append(footprints[layer], offset(line, scaled<float>(move.width / 2), ClipperLib::jtRound, 2.0, ClipperLib::etOpenRound));
        (is_bridge(role) ? bridges[layer] : ordinary[layer]).push_back(line);
    }

    for (int layer = 1; layer < LAYERS; ++layer) {
        REQUIRE_FALSE(footprints[layer - 1].empty());
        const ExPolygons previous = union_ex(footprints[layer - 1]);
        for (const auto& path : ordinary[layer])
            metrics.upper_path_mm += unscale<double>(path.length());
        for (const auto& path : bridges[layer])
            metrics.upper_path_mm += unscale<double>(path.length());
        for (const auto& path : diff_pl(ordinary[layer], previous))
            metrics.uncovered_nonbridge_mm += unscale<double>(path.length());
        for (const auto& path : diff_pl(bridges[layer], previous))
            metrics.uncovered_mm += unscale<double>(path.length());
    }
    metrics.uncovered_mm += metrics.uncovered_nonbridge_mm;

    return metrics;
}

struct Metrics
{
    double             min_component_span = std::numeric_limits<double>::infinity();
    double             max_b_area_error   = 0.0;
    double             b_volume           = 0.0;
    std::size_t        max_components     = 0;
    std::size_t        tool_changes       = 0;
    std::size_t        bridge_moves       = 0;
    ToolpathMetrics    deposition;
    std::size_t        erased_at_bead_width = 0;
    std::size_t        split_at_bead_width  = 0;
    std::size_t        erased_at_two_beads  = 0;
    std::size_t        split_at_two_beads   = 0;
    std::set<unsigned> extruding_tools;
    std::set<unsigned> perimeter_tools;
};

struct TemporaryGcode
{
    explicit TemporaryGcode(const char* pattern = "planar-routing-%%%%-%%%%-%%%%.gcode")
        : path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(pattern))
    {}
    const boost::filesystem::path path;
    ~TemporaryGcode()
    {
        boost::system::error_code error;
        boost::filesystem::remove(path, error);
    }
};

struct CommandAudit
{
    std::set<unsigned int>                tools;
    std::map<int, std::set<unsigned int>> layer_tools;
    std::array<double, 2>                 pure_length{};
    std::array<double, 2>                 opposite_side_length{};
    double                                max_wrong_side_mm = 0.;
    size_t                                tool_changes      = 0;
    size_t                                mixed_layers      = 0;
};

CommandAudit audit_offline_commands(const std::string& path, const Transition& spec, const Transform3d& placement, const BoundingBoxf& reach)
{
    const Vec3d                 center = placement * spec.point;
    const Vec3d                 normal = (placement.linear().inverse().transpose() * spec.normal).normalized();
    const auto                  a      = spec.filaments[0] - 1;
    const auto                  b      = spec.filaments[1] - 1;
    CommandAudit                audit;
    std::optional<unsigned int> active_tool;
    ExtrusionRole               role           = erNone;
    bool                        relative_e     = false;
    bool                        offline_marker = false;
    GCodeReader                 reader;
    REQUIRE(reader.parse_file(path, [&](const GCodeReader& state, const GCodeReader::GCodeLine& line) {
        const std::string_view cmd     = line.cmd();
        const std::string_view comment = line.comment();
        offline_marker |= comment == " H2D OFFLINE ONLY - DO NOT PRINT";
        if (comment.substr(0, 10) == " FEATURE: ")
            role = ExtrusionEntity::string_to_role(comment.substr(10));
        else if (comment.substr(0, 5) == "TYPE:")
            role = ExtrusionEntity::string_to_role(comment.substr(5));
        if (cmd == "M83" || cmd == "M82")
            relative_e = cmd == "M83";
        CHECK(cmd != "M620");
        CHECK(cmd != "M621");
        CHECK(cmd != "G91");
        if (!cmd.empty() && cmd.front() == 'T') {
            REQUIRE((cmd == "T0" || cmd == "T1"));
            const unsigned int next = cmd.back() - '0';
            audit.tool_changes += active_tool && *active_tool != next;
            active_tool = next;
        }
        if (!is_perimeter(role) && !is_infill(role) && role != erGapFill)
            return;
        REQUIRE((cmd != "G2" && cmd != "G3"));
        if (cmd != "G1" || !line.has_e() || line.e() <= 0. || line.dist_XY(state) == 0.)
            return;
        REQUIRE(relative_e);
        REQUIRE(active_tool);
        REQUIRE((*active_tool == a || *active_tool == b));
        const Vec3d from(state.x(), state.y(), state.z() - 0.5 * LAYER_HEIGHT);
        const Vec3d to(line.new_X(state), line.new_Y(state), line.new_Z(state) - 0.5 * LAYER_HEIGHT);
        CHECK(reach.contains(Vec2d(from.head<2>())));
        CHECK(reach.contains(Vec2d(to.head<2>())));
        CHECK(from.z() == Approx(to.z()).margin(0.001));
        const double d0         = normal.dot(from - center);
        const double d1         = normal.dot(to - center);
        const double lo         = std::min(d0, d1);
        const double hi         = std::max(d0, d1);
        const double half_width = 0.5 * spec.width;
        const double length     = line.dist_XY(state);
        if (*active_tool == a) {
            audit.max_wrong_side_mm = std::max(audit.max_wrong_side_mm, hi - half_width);
            if (hi < -half_width - 0.002)
                audit.pure_length[0] += length;
            if (lo > 0.002 && hi < half_width - 0.002)
                audit.opposite_side_length[0] += length;
        } else {
            audit.max_wrong_side_mm = std::max(audit.max_wrong_side_mm, -half_width - lo);
            if (lo > half_width + 0.002)
                audit.pure_length[1] += length;
            if (hi < -0.002 && lo > -half_width + 0.002)
                audit.opposite_side_length[1] += length;
        }
        audit.tools.insert(*active_tool);
        audit.layer_tools[int(std::lround(line.new_Z(state) / LAYER_HEIGHT))].insert(*active_tool);
    }));
    REQUIRE(offline_marker);
    for (const auto& [layer, tools] : audit.layer_tools)
        audit.mixed_layers += tools.size() == 2;
    CAPTURE(audit.max_wrong_side_mm, audit.tool_changes, audit.mixed_layers, audit.pure_length, audit.opposite_side_length);
    CHECK(audit.tools == std::set<unsigned int>{a, b});
    CHECK(audit.max_wrong_side_mm <= 0.002);
    CHECK(audit.pure_length[0] > 0.);
    CHECK(audit.pure_length[1] > 0.);
    return audit;
}

DynamicPrintConfig offline_h2d_config(const char* filename = "Bambu Lab H2D 0.4 dual - offline.json")
{
    DynamicPrintConfig                 profile;
    ConfigSubstitutionContext          substitutions{ForwardCompatibilitySubstitutionRule::Disable};
    std::map<std::string, std::string> metadata;
    std::string                        reason;
    const std::string                  path   = std::string(TEST_RESOURCES_DIR) + "/profiles/offline/" + filename;
    const int                          status = profile.load_from_json(path, substitutions, false, metadata, reason);
    INFO(reason);
    REQUIRE(status == 0);
    CHECK(substitutions.substitutions.empty());
    CHECK(substitutions.unrecogized_keys.empty());
    return profile;
}

DynamicPrintConfig routing_config()
{
    auto config = DynamicPrintConfig::full_print_config();
    config.set_num_extruders(2);
    config.set_num_filaments(2);
    for (const auto& key : config.keys()) {
        const ConfigOption* option = config.option(key);
        if (option->type() == coEnums) {
            ConfigOption* replacement = print_config_def.get(key)->create_default_option();
            replacement->set(option);
            config.set_key_value(key, replacement);
        }
    }
    config.set_key_value("filament_colour", new ConfigOptionStrings({"#276394", "#E5A44D"}));
    config.set_key_value("filament_diameter", new ConfigOptionFloats({1.75, 1.75}));
    config.set_key_value("nozzle_diameter", new ConfigOptionFloats({0.4, 0.4}));
    config.set_key_value("wall_filament", new ConfigOptionInt(1));
    for (const char* key : {"line_width", "outer_wall_line_width", "inner_wall_line_width"})
        config.set_key_value(key, new ConfigOptionFloatOrPercent(BEAD_WIDTH, false));
    config.set_key_value("layer_height", new ConfigOptionFloat(LAYER_HEIGHT));
    config.set_key_value("initial_layer_print_height", new ConfigOptionFloat(LAYER_HEIGHT));
    config.set_key_value("elefant_foot_compensation", new ConfigOptionFloat(0.0));
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(false));
    config.set_key_value("enable_arc_fitting", new ConfigOptionBool(false));
    config.set_key_value("enable_support", new ConfigOptionBool(false));
    config.set_key_value("layer_change_gcode", new ConfigOptionString("G92 E0"));
    return config;
}

Metrics audit(Route route, const DynamicPrintConfig* machine_overlay = nullptr)
{
    Model               model;
    ModelObject&        object = *model.add_object();
    std::array<int, NZ> target_b{};
    if (route == Route::Volumes) {
        target_b = add_material_volumes(object);
    } else {
        auto& envelope = *object.add_volume(make_cube(NX, NY, NZ));
        if (route == Route::Paint)
            paint_end(envelope);
    }
    if (route == Route::Direct || route == Route::InvalidPitch) {
        object.config.set_key_value(REGION_OPTION, new ConfigOptionFloats({8.0, 4.0, route == Route::Direct ? 1.0 : 0.5, 1.0}));
    }
    auto* instance = object.add_instance();
    auto  config   = routing_config();
    if (machine_overlay) {
        config.apply(*machine_overlay);
        instance->set_offset({175.0, 160.0, 0.0});
    }

    Print print;
    print.auto_assign_extruders(&object);
    print.apply(model, config);
    const auto validation = print.validate();
    INFO(validation.string);
    REQUIRE(validation.string.empty());
    print.set_status_silent();
    REQUIRE(print.objects().size() == 1);
    auto& printed = *print.get_object(0);
    printed.slice();
    REQUIRE(printed.layer_count() == 20);

    const OrderedCells   grid(MATERIAL_A.min_mm, {1, 1, 1}, {});
    const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, 4.0);
    Metrics              metrics;
    const Layer*         previous = nullptr;
    for (const Layer* layer : printed.layers()) {
        CAPTURE(layer->slice_z);
        std::array<ExPolygons, 2> material;
        for (const LayerRegion* region : layer->regions()) {
            if (region->slices.empty())
                continue;
            const int id = region->region().config().wall_filament.value;
            REQUIRE((id == 1 || id == 2));
            append(material[id - 1], to_expolygons(region->slices.surfaces));
        }
        for (auto& polygons : material)
            polygons = union_ex(polygons);

        auto region_union = material[0];
        append(region_union, material[1]);
        region_union = union_ex(region_union);
        CHECK(area_mm2(region_union) == Approx(NX * NY).margin(0.01));
        CHECK(area_mm2(intersection_ex(material[0], material[1])) < 0.01);
        if (previous)
            CHECK(area_mm2(intersection_ex(region_union, previous->lslices)) == Approx(NX * NY).margin(0.01));
        previous = layer;

        const double b_area = area_mm2(material[1]);
        metrics.b_volume += b_area * layer->height;
        const int z = static_cast<int>(layer->slice_z);
        REQUIRE((z >= 0 && z < NZ));
        if (route == Route::Volumes || route == Route::Direct) {
            const Point origin     = get_extents(region_union).min;
            int         mismatches = 0;
            if (route == Route::Direct)
                target_b[z] = 0;
            for (int y = 0; y < NY; ++y)
                for (int x = 0; x < NX; ++x) {
                    const CellIndex index{x, y, z};
                    const auto      expected = grid.assign(index, field.fraction_b(grid.center_mm(index)));
                    const Point     point    = origin + Point::new_scale(x + 0.5, y + 0.5);
                    mismatches += expolygons_contain(material[1], point) != (expected == Material::B);
                    if (route == Route::Direct)
                        target_b[z] += expected == Material::B;
                }
            CHECK(mismatches == 0);
            metrics.max_b_area_error = std::max(metrics.max_b_area_error, std::abs(b_area - target_b[z]));
        }
        metrics.max_components = std::max(metrics.max_components, material[0].size() + material[1].size());
        for (const auto& polygons : material)
            for (const auto& polygon : polygons) {
                if (route == Route::Direct || route == Route::Volumes) {
                    const auto one = offset_ex(polygon, -scaled<float>(BEAD_WIDTH / 2));
                    const auto two = offset_ex(polygon, -scaled<float>(BEAD_WIDTH));
                    metrics.erased_at_bead_width += one.empty();
                    metrics.split_at_bead_width += one.size() > 1;
                    metrics.erased_at_two_beads += two.empty();
                    metrics.split_at_two_beads += two.size() > 1;
                }
                const auto size            = get_extents(polygon.contour).size();
                metrics.min_component_span = std::min(metrics.min_component_span, unscale<double>(std::min(size.x(), size.y())));
            }
    }

    print.process();
    TemporaryGcode gcode;

    GCodeProcessorResult result;
    print.export_gcode(gcode.path.string(), &result, nullptr);
    if (route == Route::Volumes || route == Route::Direct)
        metrics.deposition = audit_deposition(result);
    if (machine_overlay) {
        const auto* area = machine_overlay->option<ConfigOptionPoints>("printable_area");
        REQUIRE(area != nullptr);
        const BoundingBoxf bounds(area->values);
        std::ifstream      file(gcode.path.string());
        REQUIRE(file.good());
        bool has_offline_marker = false;
        for (std::string line; std::getline(file, line);) {
            has_offline_marker |= line.find("; H2D OFFLINE ONLY - DO NOT PRINT") != std::string::npos;
            CHECK(line.rfind("M620", 0) != 0);
            CHECK(line.rfind("M621", 0) != 0);
        }
        CHECK(has_offline_marker);
        for (const auto& move : result.moves) {
            if (move.type != EMoveType::Extrude ||
                !(is_perimeter(move.extrusion_role) || is_infill(move.extrusion_role) || move.extrusion_role == erGapFill))
                continue;
            CHECK(bounds.contains(move.position.head<2>().cast<double>()));
        }
    }
    for (const auto& move : result.moves) {
        if (move.type == EMoveType::Tool_change)
            ++metrics.tool_changes;
        if (move.type != EMoveType::Extrude)
            continue;
        metrics.extruding_tools.insert(move.extruder_id);
        if (move.extrusion_role == erExternalPerimeter)
            metrics.perimeter_tools.insert(move.extruder_id);
        if (move.extrusion_role == erBridgeInfill || move.extrusion_role == erInternalBridgeInfill)
            ++metrics.bridge_moves;
    }
    if (route == Route::Direct) {
        // Exercise the same Print with the option removed after slicing and exporting.
        REQUIRE(object.config.erase(REGION_OPTION));
        print.apply(model, config);
        auto& plain = *print.get_object(0);
        plain.slice();
        REQUIRE(plain.layer_count() == 20);
        for (const Layer* layer : plain.layers()) {
            CHECK(area_mm2(layer->lslices) == Approx(NX * NY).margin(0.01));
            for (const LayerRegion* region : layer->regions())
                if (!region->slices.empty())
                    CHECK(region->region().config().wall_filament.value == 1);
        }
    }
    return metrics;
}

} // namespace

TEST_CASE("Compare opt-in ordered volumes and end-face painting against a solid control", "[planar-routing]")
{
    Route route = Route::Control;
    SECTION("ordinary solid") { route = Route::Control; }
    SECTION("ordered material volumes") { route = Route::Volumes; }
    SECTION("end-face painting") { route = Route::Paint; }
    SECTION("direct production regions") { route = Route::Direct; }
    SECTION("reject too-small pitch") { route = Route::InvalidPitch; }
    if (route == Route::InvalidPitch) {
        REQUIRE_THROWS_AS(audit(route), SlicingError);
        return;
    }

    const auto m = audit(route);
    CAPTURE(m.min_component_span, m.max_b_area_error, m.b_volume, m.max_components, m.tool_changes, m.bridge_moves, m.extruding_tools,
            m.perimeter_tools);
    CHECK(m.min_component_span > 0.0);
    CHECK_FALSE(m.perimeter_tools.empty());
    if (route == Route::Control) {
        CHECK(m.b_volume == Approx(0.0));
        CHECK(m.extruding_tools == std::set<unsigned>{0});
        CHECK(m.tool_changes == 0);
    } else {
        CHECK(m.extruding_tools == std::set<unsigned>{0, 1});
        CHECK(m.perimeter_tools == std::set<unsigned>{0, 1});
        CHECK(m.tool_changes > 0);
        if (route == Route::Volumes || route == Route::Direct) {
            CHECK(m.b_volume == Approx(240.0).margin(0.1));
            CAPTURE(m.deposition.commanded_volume, m.deposition.upper_path_mm, m.deposition.uncovered_mm,
                    m.deposition.uncovered_nonbridge_mm, m.erased_at_bead_width, m.split_at_bead_width, m.erased_at_two_beads,
                    m.split_at_two_beads);
            CHECK(m.deposition.commanded_volume[0] > 0.0);
            CHECK(m.deposition.commanded_volume[1] > 0.0);
            CHECK(m.deposition.upper_path_mm > 0.0);
            CHECK(m.deposition.uncovered_mm <= m.deposition.upper_path_mm);
            CHECK(m.erased_at_bead_width == 0);
            CHECK(m.max_b_area_error < 0.05);
            CHECK(m.tool_changes <= 2 * 20);
            CHECK(m.min_component_span >= 1.0 - 1e-4);
        }
    }
}

TEST_CASE("H2D offline machine geometry loads and contains both planar tools", "[planar-routing][h2d-offline]")
{
    const auto  profile = offline_h2d_config();
    const auto* nozzle  = profile.option<ConfigOptionFloats>("nozzle_diameter");
    REQUIRE(nozzle != nullptr);
    CHECK(nozzle->values == std::vector<double>{0.4, 0.4});
    const auto* height = profile.option<ConfigOptionFloat>("printable_height");
    REQUIRE(height != nullptr);
    CHECK(height->value == 325.0);
    const auto* bed = profile.option<ConfigOptionPoints>("printable_area");
    REQUIRE(bed != nullptr);
    REQUIRE(bed->values.size() == 4);
    const BoundingBoxf bounds(bed->values);
    CHECK(bounds.min == Vec2d(25, 0));
    CHECK(bounds.max == Vec2d(325, 320));
    const auto m = audit(Route::Direct, &profile);
    CAPTURE(m.tool_changes, m.b_volume, m.deposition.commanded_volume);
    CHECK(m.b_volume == Approx(240.0).margin(0.1));
    CHECK(m.max_b_area_error < 0.05);
    CHECK(m.extruding_tools == std::set<unsigned>{0, 1});
    CHECK(m.perimeter_tools == std::set<unsigned>{0, 1});
    CHECK(m.tool_changes > 0);
}

TEST_CASE("H2D offline process respects the generic acceleration cap", "[h2d-offline]")
{
    Model model;
    auto& object = *model.add_object();
    object.add_volume(make_cube(10, 10, 4));
    auto& instance = *object.add_instance();
    instance.set_offset({175, 160, 0});
    const Transform3d placement = instance.get_matrix();
    const Transition  spec{placement.inverse() * object.instance_bounding_box(0).center(), Vec3d::UnitX(), 4., {1, 2}};
    object.config.set_key_value(TRANSITION_OPTION, new ConfigOptionFloats(spec.serialize()));

    auto config = routing_config();
    config.apply(offline_h2d_config());
    Print print;
    print.set_status_silent();
    print.apply(model, config);
    StringObjectException original_warning{};
    REQUIRE(print.validate(&original_warning).string.empty());
    INFO(original_warning.string);
    REQUIRE(original_warning.string.find("machine_max_acceleration_extruding") != std::string::npos);

    config.apply(offline_h2d_config("H2D offline 0.20 mm - conservative.json"));
    print.apply(model, config);
    StringObjectException warning{};
    REQUIRE(print.validate(&warning).string.empty());
    INFO(warning.string);
    CHECK(warning.string.empty());

    print.process();
    TemporaryGcode       gcode;
    GCodeProcessorResult result;
    print.export_gcode(gcode.path.string(), &result, nullptr);
    const auto audit = audit_offline_commands(gcode.path.string(), spec, placement, BoundingBoxf({25, 0}, {325, 320}));
    CHECK(audit.mixed_layers == 20);

    const double cap = print.config().machine_max_acceleration_extruding.get_at(0);
    REQUIRE(cap == 1500.);
    size_t      acceleration_commands = 0;
    GCodeReader reader;
    REQUIRE(reader.parse_file(gcode.path.string(), [&](const GCodeReader&, const GCodeReader::GCodeLine& line) {
        CHECK(line.cmd() != "M201");
        if (line.cmd() != "M204")
            return;
        float value;
        REQUIRE(line.has_value('S', value));
        CHECK(value <= cap);
        ++acceleration_commands;
    }));
    CHECK(acceleration_commands > 0);
}

TEST_CASE("H2D vendor export contract matches offline tool and reach invariants", "[h2d-offline]")
{
    TemporaryGcode     step_file("planar-vendor-assembly-%%%%-%%%%-%%%%.step");
    const auto&        step_path = step_file.path;
    STEPControl_Writer writer;
    REQUIRE(writer.Transfer(BRepPrimAPI_MakeBox(gp_Pnt(163, 154, 0), 10, 12, 4).Shape(), STEPControl_AsIs) == IFSelect_RetDone);
    REQUIRE(writer.Transfer(BRepPrimAPI_MakeBox(gp_Pnt(173, 154, 0), 14, 12, 4).Shape(), STEPControl_AsIs) == IFSelect_RetDone);
    REQUIRE(writer.Write(step_path.string().c_str()) == IFSelect_RetDone);

    Model model;
    bool  cancelled = false;
    long  faces     = -1;
    REQUIRE(load_step(step_path.string().c_str(), &model, cancelled, 0.003, 0.5, false, nullptr, nullptr, faces));
    REQUIRE_FALSE(cancelled);
    REQUIRE(model.objects.size() == 1);
    auto& object = *model.objects.front();
    REQUIRE(object.volumes.size() == 2);
    model.add_default_instances();
    auto&             instance  = *object.instances.front();
    const Transform3d placement = instance.get_matrix();

    const auto face = find_shared_planar_interface(object, placement);
    REQUIRE(face);
    CHECK(face->point.isApprox(Vec3d(173, 160, 2), 1e-4));
    CHECK(face->normal.isApprox(Vec3d::UnitX(), 1e-4));

    for (auto* volume : object.volumes)
        volume->config.set_key_value("extruder", new ConfigOptionInt(1));

    Transition spec_ab{face->point, face->normal, 8., {1, 2}};
    object.config.set_key_value(TRANSITION_OPTION, new ConfigOptionFloats(spec_ab.serialize()));

    auto config = routing_config();
    config.apply(offline_h2d_config());
    config.apply(offline_h2d_config("H2D offline 0.20 mm - conservative.json"));

    Print print;
    print.set_status_silent();
    print.apply(model, config);
    StringObjectException warning{};
    REQUIRE(print.validate(&warning).string.empty());
    CHECK(warning.string.empty());

    print.process();
    TemporaryGcode       gcode_ab;
    GCodeProcessorResult result;
    print.export_gcode(gcode_ab.path.string(), &result, nullptr);

    const BoundingBoxf shared_reach({25, 0}, {325, 320});
    const auto         audit_ab = audit_offline_commands(gcode_ab.path.string(), spec_ab, placement, shared_reach);
    CHECK(audit_ab.mixed_layers == 20);
    CHECK(audit_ab.tools == std::set<unsigned int>{0, 1});

    Transition spec_ba{face->point, face->normal, 8., {2, 1}};
    object.config.set_key_value(TRANSITION_OPTION, new ConfigOptionFloats(spec_ba.serialize()));
    print.apply(model, config);
    print.process();
    TemporaryGcode gcode_ba;
    print.export_gcode(gcode_ba.path.string(), &result, nullptr);
    const auto audit_ba = audit_offline_commands(gcode_ba.path.string(), spec_ba, placement, shared_reach);
    CHECK(audit_ba.mixed_layers == 20);
    CHECK(audit_ba.tools == std::set<unsigned int>{0, 1});

    GCodeReader reader;
    size_t      tool_changes = 0;
    REQUIRE(reader.parse_file(gcode_ab.path.string(), [&](const GCodeReader&, const GCodeReader::GCodeLine& line) {
        CHECK(line.cmd() != "M620");
        CHECK(line.cmd() != "M621");
        CHECK(line.cmd() != "G150.1");
        CHECK(line.cmd() != "G150.2");
        CHECK(line.cmd() != "G150.3");
        if (line.cmd() == "T0" || line.cmd() == "T1")
            ++tool_changes;
    }));
    CHECK(tool_changes >= 20);
}

TEST_CASE("A gradual material transition routes an intact solid by signed distance", "[planar-transition]")
{
    Model              model;
    ModelObject&       object   = *model.add_object();
    const ModelVolume& volume   = *object.add_volume(make_cube(20, 20, 20));
    ModelInstance&     instance = *object.add_instance();
    instance.set_offset({100, 100, 0});
    const BoundingBoxf3 bounds        = object.instance_bounding_box(0);
    const Transform3d   transform     = instance.get_matrix();
    const Vec3d         center        = transform.inverse() * bounds.center();
    Vec3d               normal        = Vec3d::UnitZ();
    bool                reverse_slots = false;
    SECTION("Horizontal plane") {}
    SECTION("Tilted plane and reversed slots")
    {
        normal        = Vec3d(0.7, 0.35, 1.).normalized();
        reverse_slots = true;
    }
    SECTION("Diagonal XY gradient") { normal = Vec3d(0.8, 0.6, 0.); }

    const char*  benchmark_width = std::getenv("SLIC3R_TEST_PLANAR_WIDTH");
    const double width           = benchmark_width ? std::stod(benchmark_width) : 8.;
    REQUIRE((width == 4. || width == 8. || width == 12.));
    const unsigned int tool_b = reverse_slots ? 0 : 1;
    const Transition   spec{center, normal, width, {2 - tool_b, 1 + tool_b}};
    object.config.set_key_value(TRANSITION_OPTION, new ConfigOptionFloats(spec.serialize()));
    auto config = routing_config();
    config.apply(offline_h2d_config());
    Print print;
    print.set_status_silent();
    print.auto_assign_extruders(&object);
    print.apply(model, config);
    REQUIRE(print.validate().string.empty());
    print.process();
    REQUIRE(print.objects().size() == 1);
    REQUIRE(print.get_object(0)->layers().size() == 100);
    for (const Layer* layer : print.get_object(0)->layers()) {
        REQUIRE(layer->lslices.size() == 1);
        CHECK(area_mm2(layer->lslices) == Approx(400.).margin(0.01));
    }

    TemporaryGcode       gcode;
    GCodeProcessorResult result;
    print.export_gcode(gcode.path.string(), &result, nullptr);
    const auto commands = audit_offline_commands(gcode.path.string(), spec, transform, BoundingBoxf({25, 0}, {325, 320}));
    if (normal.z() == 0.) {
        CHECK(commands.layer_tools.size() == 100);
        CHECK(commands.mixed_layers == 100);
    }
    const Vec3d world_center = transform * center;
    auto        model_move   = [](const auto& move) {
        return move.type == EMoveType::Extrude &&
               (is_perimeter(move.extrusion_role) || is_infill(move.extrusion_role) || move.extrusion_role == erGapFill);
    };
    struct Band
    {
        double volume   = 0.;
        double actual_b = 0.;
        double target_b = 0.;
    };
    const std::array<double, 6> edges{-width / 2., -3. * width / 8., -width / 8., width / 8., 3. * width / 8., width / 2.};
    std::array<Band, 7>         bands;
    std::array<Band, 7>         wall_bands;
    double                      total_extrusion   = 0.;
    double                      outer_extrusion   = 0.;
    size_t                      tool_changes      = 0;
    size_t                      short_model_moves = 0;
    std::set<unsigned>          wall_tools;
    for (size_t i = 1; i < result.moves.size(); ++i) {
        const auto& move = result.moves[i];
        tool_changes += move.type == EMoveType::Tool_change;
        if (!model_move(move))
            continue;
        const double amount   = move.delta_extruder;
        const double sample_z = move.position.z() - 0.5 * LAYER_HEIGHT;
        Vec3d        from     = result.moves[i - 1].position.cast<double>();
        Vec3d        to       = move.position.cast<double>();
        from.z() = to.z()   = sample_z;
        const double length = (to - from).norm();
        short_model_moves += length > 0. && length < 0.2;
        const bool          outer_wall = move.extrusion_role == erExternalPerimeter;
        const double        d0         = normal.dot(from - world_center);
        const double        d1         = normal.dot(to - world_center);
        std::vector<double> cuts{0., 1.};
        for (double edge : edges)
            if (edge > std::min(d0, d1) && edge < std::max(d0, d1))
                cuts.push_back((edge - d0) / (d1 - d0));
        std::sort(cuts.begin(), cuts.end());
        for (size_t j = 1; j < cuts.size(); ++j) {
            const double d          = d0 + (d1 - d0) * 0.5 * (cuts[j - 1] + cuts[j]);
            const double e          = amount * (cuts[j] - cuts[j - 1]);
            const size_t band_id    = std::upper_bound(edges.begin(), edges.end(), d) - edges.begin();
            const double actual_b   = move.extruder_id == tool_b ? e : 0.;
            const double target_b   = std::clamp(0.5 + d / spec.width, 0., 1.) * e;
            auto         accumulate = [&](Band& band) {
                band.volume += e;
                band.actual_b += actual_b;
                band.target_b += target_b;
            };
            accumulate(bands[band_id]);
            if (outer_wall)
                accumulate(wall_bands[band_id]);
        }
        total_extrusion += amount;
        if (outer_wall) {
            outer_extrusion += amount;
            wall_tools.insert(move.extruder_id);
        }
    }
    CAPTURE(tool_changes, short_model_moves);
    CHECK(wall_tools == std::set<unsigned>{0, 1});
    double max_model_error = 0.;
    double max_wall_error  = 0.;
    for (size_t i = 0; i < bands.size(); ++i) {
        const auto& band = bands[i];
        REQUIRE(band.volume > 0.);
        const double actual = band.actual_b / band.volume;
        const double target = band.target_b / band.volume;
        const Band&  wall   = wall_bands[i];
        REQUIRE(wall.volume > 0.);
        const double actual_wall = wall.actual_b / wall.volume;
        const double target_wall = wall.target_b / wall.volume;
        max_model_error          = std::max(max_model_error, std::abs(actual - target));
        max_wall_error           = std::max(max_wall_error, std::abs(actual_wall - target_wall));
        CAPTURE(i, actual, target, actual_wall, target_wall);
        const bool pure = i == 0 || i == bands.size() - 1;
        CHECK(actual == Approx(target).margin(pure ? 0.015 : 0.05));
        CHECK(actual_wall == Approx(target_wall).margin(pure ? 0.015 : 0.15));
    }

    REQUIRE(object.config.erase(TRANSITION_OPTION));
    REQUIRE(object.volumes.size() == 1);
    CHECK(object.volumes.front() == &volume);
    CHECK(object.instance_bounding_box(0).min.isApprox(bounds.min));
    CHECK(object.instance_bounding_box(0).max.isApprox(bounds.max));
    print.apply(model, config);
    print.process();
    TemporaryGcode       plain_gcode;
    GCodeProcessorResult plain;
    print.export_gcode(plain_gcode.path.string(), &plain, nullptr);
    double plain_total = 0.;
    double plain_outer = 0.;
    for (const auto& move : plain.moves)
        if (model_move(move)) {
            CHECK(move.extruder_id == 0);
            plain_total += move.delta_extruder;
            if (move.extrusion_role == erExternalPerimeter)
                plain_outer += move.delta_extruder;
        }
    CHECK(total_extrusion == Approx(plain_total).epsilon(0.025));
    CHECK(outer_extrusion == Approx(plain_outer).epsilon(0.025));
    if (benchmark_width) {
        double flush_mm3 = 0.;
        for (const auto& entry : result.print_statistics.flush_per_filament)
            flush_mm3 += entry.second;
        const size_t mode = static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal);
        WARN("PLANAR_BENCH width_mm=" << width << " normal=" << normal.transpose() << " short_moves=" << short_model_moves
                                      << " tool_changes=" << commands.tool_changes << " mixed_layers=" << commands.mixed_layers
                                      << " max_model_error_pp=" << 100. * max_model_error << " max_wall_error_pp=" << 100. * max_wall_error
                                      << " model_e_mm=" << total_extrusion << " control_e_mm=" << plain_total
                                      << " estimated_s=" << result.print_statistics.modes[mode].time << " control_estimated_s="
                                      << plain.print_statistics.modes[mode].time << " offline_flush_mm3=" << flush_mm3);
    }
}

TEST_CASE("A STEP assembly transitions across its selected shared face", "[planar-transition][assembly-interface]")
{
    const bool separate_files = GENERATE(false, true);
    CAPTURE(separate_files);
    const auto left_body = BRepPrimAPI_MakeBox(gp_Pnt(12, 8, 0), 10, 12, 4).Shape();
    const auto right_body = BRepPrimAPI_MakeBox(gp_Pnt(22, 8, 0), 14, 12, 4).Shape();
    auto read_step = [](STEPControl_Writer& writer, Model& model) {
        TemporaryGcode step_file("planar-interface-%%%%-%%%%-%%%%.step");
        REQUIRE(writer.Write(step_file.path.string().c_str()) == IFSelect_RetDone);
        bool cancelled = false;
        long faces = -1;
        REQUIRE(load_step(step_file.path.string().c_str(), &model, cancelled, 0.003, 0.5, false, nullptr, nullptr, faces));
        REQUIRE_FALSE(cancelled);
    };

    Model model;
    STEPControl_Writer writer;
    REQUIRE(writer.Transfer(left_body, STEPControl_AsIs) == IFSelect_RetDone);
    if (separate_files) {
        auto& assembly = *model.add_object();
        Model left_source;
        read_step(writer, left_source);
        REQUIRE(append_step_components(assembly, left_source, Vec3d::Zero()).size() == 1);

        STEPControl_Writer right_writer;
        REQUIRE(right_writer.Transfer(right_body, STEPControl_AsIs) == IFSelect_RetDone);
        Model right_source;
        read_step(right_writer, right_source);
        REQUIRE(append_step_components(assembly, right_source, Vec3d::Zero()).size() == 1);
    } else {
        REQUIRE(writer.Transfer(right_body, STEPControl_AsIs) == IFSelect_RetDone);
        read_step(writer, model);
    }
    REQUIRE(model.objects.size() == 1);
    auto& object = *model.objects.front();
    REQUIRE(object.volumes.size() == 2);
    model.add_default_instances();
    REQUIRE(object.instances.size() == 1);
    auto& instance = *object.instances.front();
    instance.set_offset({145, 120, 0});
    instance.set_rotation({0, 0, 0.31});
    const Transform3d placement   = instance.get_matrix();
    auto              body_bounds = [&](size_t idx) {
        const auto& volume = *object.volumes[idx];
        return volume.mesh().transformed_bounding_box(volume.get_matrix());
    };
    const auto  body0          = body_bounds(0);
    const auto  body1          = body_bounds(1);
    const bool  first_is_lower = body0.min.x() < body1.min.x();
    const auto& lower          = first_is_lower ? body0 : body1;
    const auto& upper          = first_is_lower ? body1 : body0;
    REQUIRE(lower.min.x() == Approx(12.).margin(0.0001));
    REQUIRE(lower.max.x() == Approx(22.).margin(0.0001));
    REQUIRE(upper.min.x() == Approx(22.).margin(0.0001));
    REQUIRE(upper.max.x() == Approx(36.).margin(0.0001));
    for (auto* volume : object.volumes)
        volume->config.set_key_value("extruder", new ConfigOptionInt(1));

    const auto face = find_shared_planar_interface(object, placement);
    REQUIRE(face);
    const Vec3d expected_normal = (first_is_lower ? 1. : -1.) * Vec3d::UnitX();
    CHECK(face->point.isApprox(Vec3d(22, 14, 2), 1e-6));
    CHECK(face->normal.isApprox(expected_normal, 1e-6));
    Transition spec{face->point, face->normal, 8., first_is_lower ? std::array<unsigned int, 2>{2, 1} : std::array<unsigned int, 2>{1, 2}};
    auto*      upper_volume = object.volumes[first_is_lower ? 1 : 0];
    auto       config       = routing_config();
    config.apply(offline_h2d_config());

    SECTION("Contact face stays in place and both tools print each layer")
    {
        object.config.set_key_value(TRANSITION_OPTION, new ConfigOptionFloats(spec.serialize()));
        Print print;
        print.set_status_silent();
        print.apply(model, config);
        REQUIRE(print.validate().string.empty());
        print.process();
        REQUIRE(print.objects().size() == 1);
        REQUIRE(print.get_object(0)->layers().size() == 20);
        for (const Layer* layer : print.get_object(0)->layers()) {
            CHECK(layer->lslices.size() == 1);
            CHECK(area_mm2(layer->lslices) == Approx(288.).margin(0.02));
        }
        TemporaryGcode       gcode;
        GCodeProcessorResult result;
        print.export_gcode(gcode.path.string(), &result, nullptr);
        const auto audit = audit_offline_commands(gcode.path.string(), spec, placement, BoundingBoxf({25, 0}, {325, 320}));
        CHECK(audit.mixed_layers == 20);
        CHECK(audit.opposite_side_length[0] > 0.);
        CHECK(audit.opposite_side_length[1] > 0.);
        const Transform3d to_local       = placement.inverse();
        size_t            internal_walls = 0;
        size_t            wall_moves     = 0;
        for (size_t i = 1; i < result.moves.size(); ++i) {
            const auto& move = result.moves[i];
            if (move.type != EMoveType::Extrude || !is_perimeter(move.extrusion_role))
                continue;
            ++wall_moves;
            const Vec3d from = to_local * result.moves[i - 1].position.cast<double>();
            const Vec3d to   = to_local * move.position.cast<double>();
            if (std::abs(from.x() - 22.) < 1. && std::abs(to.x() - 22.) < 1. && std::min(from.y(), to.y()) < 19. &&
                std::max(from.y(), to.y()) > 9.)
                ++internal_walls;
        }
        CHECK(wall_moves > 0);
        CHECK(internal_walls == 0);
        CHECK(body_bounds(0).min.isApprox(body0.min));
        CHECK(body_bounds(0).max.isApprox(body0.max));
        CHECK(body_bounds(1).min.isApprox(body1.min));
        CHECK(body_bounds(1).max.isApprox(body1.max));
        CHECK(instance.get_matrix().matrix().isApprox(placement.matrix()));
    }

    SECTION("Alignment follows volume order under transformed placement")
    {
        std::swap(object.volumes[0], object.volumes[1]);
        const Transform3d scaled_placement = placement * Eigen::AngleAxisd(0.47, Vec3d::UnitY()) * Eigen::Scaling(1.3, 0.6, 1.8);
        const auto        reversed         = find_shared_planar_interface(object, scaled_placement);
        REQUIRE(reversed);
        CHECK(reversed->point.isApprox(face->point, 1e-6));
        CHECK(reversed->normal.isApprox(-expected_normal, 1e-6));
        CHECK_FALSE(has_transition(object));
    }

    SECTION("A gap, overlap, partial face, edge contact or shifted plane is rejected")
    {
        const Vec3d                original_offset = upper_volume->get_offset();
        const std::array<Vec3d, 5> displacements{Vec3d(0.1, 0, 0), Vec3d(-0.1, 0, 0), Vec3d(0, 0.1, 0), Vec3d(0, 12, 0), Vec3d::Zero()};
        for (size_t i = 0; i < displacements.size(); ++i) {
            CAPTURE(i);
            upper_volume->set_offset(original_offset + displacements[i]);
            const bool shifted_plane = i == displacements.size() - 1;
            CHECK(find_shared_planar_interface(object, placement).has_value() == shifted_plane);
            spec.point.x() = shifted_plane ? 22.25 : 22.;
            object.config.set_key_value(TRANSITION_OPTION, new ConfigOptionFloats(spec.serialize()));
            Print print;
            print.set_status_silent();
            print.apply(model, config);
            REQUIRE(print.objects().size() == 1);
            CHECK_THROWS_AS(print.get_object(0)->slice(), SlicingError);
        }
    }
}
