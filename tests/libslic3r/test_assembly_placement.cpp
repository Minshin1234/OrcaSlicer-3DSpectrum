#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Model.hpp"

#include "libslic3r/Format/STEP.hpp"
#include "libslic3r/Format/STL.hpp"
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PlanarGradient/PlanarRegions.hpp"
#include "libslic3r/PlanarGradient/PlanarTransition.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include <stdexcept>
#include <cmath>
#include <algorithm>

using namespace Slic3r;

namespace {

struct ScratchDirectory
{
    const fs::path path = fs::temp_directory_path() / fs::unique_path("fs-placement-%%%%-%%%%");

    ScratchDirectory() { REQUIRE(fs::create_directory(path)); }
    ~ScratchDirectory()
    {
        boost::system::error_code error;
        fs::remove_all(path, error);
    }
};

void write_assembly(const std::string& path)
{
    const auto app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument(TCollection_ExtendedString("BinXCAF"), doc);
    const auto tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    const auto a    = tool->AddShape(BRepPrimAPI_MakeBox(2, 3, 4).Shape(), false);
    const auto b    = tool->AddShape(BRepPrimAPI_MakeBox(4, 3, 2).Shape(), false);
    TDataStd_Name::Set(a, TCollection_ExtendedString("Placement_A"));
    TDataStd_Name::Set(b, TCollection_ExtendedString("Placement_B"));

    gp_Trsf a_pose, b_pose, parent_pose;
    a_pose.SetTranslation(gp_Vec(2, 1, 0));
    b_pose.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), std::acos(-1.0) / 2);
    b_pose.SetTranslationPart(gp_Vec(9, 4, 1));
    parent_pose.SetTranslation(gp_Vec(10, 20, 3));
    const auto nested = tool->NewShape();
    tool->AddComponent(nested, a, TopLoc_Location(a_pose));
    tool->AddComponent(nested, b, TopLoc_Location(b_pose));
    const auto root = tool->NewShape();
    tool->AddComponent(root, nested, TopLoc_Location(parent_pose));
    tool->UpdateAssemblies();

    STEPCAFControl_Writer writer;
    const bool            transferred = writer.Transfer(doc);
    const auto            status      = transferred ? writer.Write(path.c_str()) : IFSelect_RetError;
    app->Close(doc);
    REQUIRE(transferred);
    REQUIRE(status == IFSelect_RetDone);
}

void write_step_component(const std::string& path, const char* name, double x, double width)
{
    const auto app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument(TCollection_ExtendedString("BinXCAF"), doc);
    const auto tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    const auto solid = tool->AddShape(BRepPrimAPI_MakeBox(gp_Pnt(x, 8, 3), width, 12, 4).Shape(), false);
    TDataStd_Name::Set(solid, TCollection_ExtendedString(name));
    STEPCAFControl_Writer writer;
    writer.SetNameMode(true);
    const bool transferred = writer.Transfer(doc, STEPControl_AsIs);
    const auto status = transferred ? writer.Write(path.c_str()) : IFSelect_RetError;
    app->Close(doc);
    REQUIRE(transferred);
    REQUIRE(status == IFSelect_RetDone);
}

BoundingBoxf3 world_bounds(const ModelVolume& volume, const ModelInstance& instance)
{
    const Transform3d world = instance.get_matrix() * volume.get_matrix();
    BoundingBoxf3     box;
    for (const auto& vertex : volume.mesh().its.vertices)
        box.merge(world * vertex.cast<double>());
    return box;
}

ModelVolume& named_body(const ModelObject& object, const char* name)
{
    for (ModelVolume* volume : object.volumes)
        if (volume->name == name)
            return *volume;
    FAIL("Missing assembly body " << name);
    throw std::logic_error("Unreachable after Catch2 failure");
}

void check_bounds(const BoundingBoxf3& box, const Vec3d& min, const Vec3d& max)
{
    CHECK((box.min - min).cwiseAbs().maxCoeff() < 1e-4);
    CHECK((box.max - max).cwiseAbs().maxCoeff() < 1e-4);
}

struct LoadedProject
{
    Model                model;
    PlateDataPtrs        plates;
    std::vector<Preset*> presets;

    ~LoadedProject()
    {
        release_PlateData_list(plates);
        for (Preset* preset : presets)
            delete preset;
    }
};

} // namespace

TEST_CASE("Nested STEP assembly placement and per-body extruders survive a project 3MF round trip", "[placement-audit]")
{
    const ScratchDirectory scratch;
    const std::string      step_path = (scratch.path / "assembly.step").string();
    write_assembly(step_path);

    Model step_model;
    bool  cancelled     = false;
    long  mesh_face_num = -1;
    REQUIRE(load_step(step_path.c_str(), &step_model, cancelled, 0.003, 0.5, false, nullptr, nullptr, mesh_face_num));
    REQUIRE_FALSE(cancelled);
    step_model.add_default_instances();
    REQUIRE(step_model.objects.size() == 1);
    ModelObject& object = *step_model.objects.front();
    REQUIRE(object.volumes.size() == 2);
    REQUIRE(object.instances.size() == 1);
    ModelInstance& instance = *object.instances.front();
    ModelVolume&   a        = named_body(object, "Placement_A");
    ModelVolume&   b        = named_body(object, "Placement_B");

    check_bounds(world_bounds(a, instance), {12, 21, 3}, {14, 24, 7});
    check_bounds(world_bounds(b, instance), {16, 24, 4}, {19, 28, 6});

    a.config.set_key_value("extruder", new ConfigOptionInt(1));
    b.config.set_key_value("extruder", new ConfigOptionInt(2));
    instance.set_offset({30, -10, 5});
    const auto before_a = world_bounds(a, instance);
    const auto before_b = world_bounds(b, instance);
    check_bounds(before_a, {42, 11, 8}, {44, 14, 12});
    check_bounds(before_b, {46, 14, 9}, {49, 18, 11});

    PresetBundle       bundle;
    DynamicPrintConfig config                                     = bundle.project_config;
    config.option<ConfigOptionStrings>("filament_colour")->values = {"#226699", "#EEAA44"};
    const std::string project_path                                = (scratch.path / "assembly.3mf").string();
    StoreParams       store;
    store.path     = project_path.c_str();
    store.model    = &step_model;
    store.config   = &config;
    store.strategy = SaveStrategy::Zip64 | SaveStrategy::Silence | SaveStrategy::SkipStatic;
    REQUIRE(store_bbs_3mf(store));

    LoadedProject loaded;
    loaded.model.set_backup_path((scratch.path / "import-backup").string());
    DynamicPrintConfig        imported_config;
    ConfigSubstitutionContext substitutions{ForwardCompatibilitySubstitutionRule::Disable};
    bool                      is_bbs = false;
    Semver                    version;
    REQUIRE(load_bbs_3mf(project_path.c_str(), &imported_config, &substitutions, &loaded.model, &loaded.plates, &loaded.presets, &is_bbs,
                         &version, nullptr, LoadStrategy::LoadModel | LoadStrategy::LoadConfig));
    REQUIRE(loaded.model.objects.size() == 1);
    const ModelObject& result = *loaded.model.objects.front();
    REQUIRE(result.volumes.size() == 2);
    REQUIRE(result.instances.size() == 1);
    const auto& imported_a = named_body(result, "Placement_A");
    const auto& imported_b = named_body(result, "Placement_B");
    CHECK(imported_a.config.opt_int("extruder") == 1);
    CHECK(imported_b.config.opt_int("extruder") == 2);
    check_bounds(world_bounds(imported_a, *result.instances.front()), before_a.min, before_a.max);
    check_bounds(world_bounds(imported_b, *result.instances.front()), before_b.min, before_b.max);
}

TEST_CASE("Separate STEP components retain a shared CAD frame and filament assignments", "[placement-audit][step-components]")
{
    const ScratchDirectory scratch;
    Model assembly;
    auto* object = assembly.add_object();
    object->name = "CAD assembly";
    auto read_component = [&](const char* file, const char* name, double x, double width) {
        const auto path = (scratch.path / file).string();
        write_step_component(path, name, x, width);
        Model model;
        bool cancelled = false;
        long faces = -1;
        REQUIRE(load_step(path.c_str(), &model, cancelled, 0.003, 0.5, false, nullptr, nullptr, faces));
        REQUIRE_FALSE(cancelled);
        REQUIRE(model.objects.size() == 1);
        REQUIRE(model.objects.front()->instances.empty());
        return model;
    };
    const auto base = read_component("base.step", "Rigid base", 12, 10);
    const auto joint = read_component("joint.STP", "Flexible joint", 22, 6);
    const auto finger = read_component("finger.step", "Rigid finger", 28, 8);
    REQUIRE(append_step_components(*object, base, Vec3d::Zero()).size() == 1);
    REQUIRE(append_step_components(*object, joint, Vec3d::Zero()).size() == 1);
    const auto cad_frame = step_component_frame(*object);
    REQUIRE(cad_frame);
    CHECK(cad_frame->isZero(1e-6));
    REQUIRE(object->volumes.size() == 2);
    auto& a = named_body(*object, "Rigid base");
    auto& b = named_body(*object, "Flexible joint");
    a.config.set_key_value("extruder", new ConfigOptionInt(1));
    b.config.set_key_value("extruder", new ConfigOptionInt(2));
    auto* instance = object->add_instance();
    check_bounds(world_bounds(a, *instance), {12, 8, 3}, {22, 20, 7});
    check_bounds(world_bounds(b, *instance), {22, 8, 3}, {28, 20, 7});

    object->center_around_origin();
    instance->set_offset({135, 120, 8});
    instance->set_rotation({0, 0, 0.37});
    const auto before_a = world_bounds(a, *instance);
    const auto before_b = world_bounds(b, *instance);
    const auto frame = step_component_frame(*object);
    REQUIRE(frame);
    CHECK(frame->isApprox(object->origin_translation, 1e-6));

    PresetBundle bundle;
    auto config = bundle.project_config;
    config.option<ConfigOptionStrings>("filament_colour")->values = {"#226699", "#EEAA44"};
    const std::string project_path = (scratch.path / "components.3mf").string();
    StoreParams store;
    store.path = project_path.c_str();
    store.model = &assembly;
    store.config = &config;
    store.strategy = SaveStrategy::Zip64 | SaveStrategy::Silence | SaveStrategy::SkipStatic;
    REQUIRE(store_bbs_3mf(store));

    LoadedProject loaded;
    loaded.model.set_backup_path((scratch.path / "import-backup").string());
    DynamicPrintConfig imported_config;
    ConfigSubstitutionContext substitutions{ForwardCompatibilitySubstitutionRule::Disable};
    bool is_bbs = false;
    Semver version;
    REQUIRE(load_bbs_3mf(project_path.c_str(), &imported_config, &substitutions, &loaded.model, &loaded.plates, &loaded.presets, &is_bbs,
                         &version, nullptr, LoadStrategy::LoadModel | LoadStrategy::LoadConfig));
    REQUIRE(loaded.model.objects.size() == 1);
    auto& reopened = *loaded.model.objects.front();
    REQUIRE(reopened.instances.size() == 1);
    auto& reopened_instance = *reopened.instances.front();
    auto& reopened_a = named_body(reopened, "Rigid base");
    auto& reopened_b = named_body(reopened, "Flexible joint");
    CHECK(reopened_a.config.extruder() == 1);
    CHECK(reopened_b.config.extruder() == 2);
    const auto reopened_frame = step_component_frame(reopened);
    REQUIRE(reopened_frame);
    CHECK(reopened_frame->isApprox(*frame, 1e-6));
    const auto added = append_step_components(reopened, finger, *reopened_frame);
    REQUIRE(added.size() == 1);
    REQUIRE(reopened.volumes.size() == 3);
    CHECK(added.front()->name == "Rigid finger");
    check_bounds(world_bounds(reopened_a, reopened_instance), before_a.min, before_a.max);
    check_bounds(world_bounds(reopened_b, reopened_instance), before_b.min, before_b.max);
    const Vec3d cad_delta = reopened_instance.get_matrix().linear() * Vec3d(7, 0, 0);
    CHECK(world_bounds(*added.front(), reopened_instance).center().isApprox(before_b.center() + cad_delta, 1e-6));
    CHECK(step_component_frame(reopened));
    added.front()->translate(Vec3d(0.2, 0, 0));
    CHECK_FALSE(step_component_frame(reopened));
}

TEST_CASE("Separate STL files preserve vertex coordinates but cannot restore omitted placement", "[placement-audit]")
{
    const ScratchDirectory scratch;
    auto                   verify_stl = [&](const char* filename, const Vec3d& translation, const Vec3d& size) {
        const std::string path = (scratch.path / filename).string();
        TriangleMesh      mesh = make_cube(size.x(), size.y(), size.z());
        mesh.translate(translation.cast<float>());
        REQUIRE(store_stl(path.c_str(), &mesh, true));
        Model imported;
        REQUIRE(load_stl(path.c_str(), &imported));
        imported.add_default_instances();
        REQUIRE(imported.objects.size() == 1);
        const ModelObject& object = *imported.objects.front();
        REQUIRE(object.volumes.size() == 1);
        REQUIRE(object.instances.size() == 1);
        check_bounds(world_bounds(*object.volumes.front(), *object.instances.front()), translation, translation + size);
    };
    verify_stl("a-world.stl", {12, 21, 3}, {2, 3, 4});
    verify_stl("b-world.stl", {16, 24, 4}, {3, 4, 2});
    verify_stl("a-local.stl", {0, 0, 0}, {2, 3, 4});
    verify_stl("b-local.stl", {0, 0, 0}, {3, 4, 2});
}

TEST_CASE("Native 3MF preserves planar material settings only on the opted-in object", "[placement-audit][planar-routing][planar-transition]")
{
    using Slic3r::PlanarGradient::REGION_OPTION;
    using Slic3r::PlanarGradient::TRANSITION_OPTION;
    const ScratchDirectory    scratch;
    const std::string         path = (scratch.path / "planar-opt-in.3mf").string();
    std::string option;
    std::vector<double> settings;
    SECTION("Experimental region adapter") {
        option = REGION_OPTION;
        settings = {8.0, 4.0, 1.0, 1.0};
    }
    SECTION("Tilted gradual transition") {
        option = TRANSITION_OPTION;
        settings = {8.0, 3.0, 2.0, 0.6, 0.0, 0.8, 5.5, 2.0, 1.0};
    }

    Model        source;
    ModelObject* opted = source.add_object();
    opted->name        = "Planar opt-in";
    opted->add_volume(make_cube(18, 6, 4));
    opted->config.set_key_value(option, new ConfigOptionFloats(settings));
    opted->add_instance()->set_offset({30, 30, 0});
    const auto original_bounds = world_bounds(*opted->volumes.front(), *opted->instances.front());

    ModelObject* ordinary = source.add_object();
    ordinary->name        = "Ordinary solid";
    ordinary->add_volume(make_cube(18, 6, 4));
    ordinary->add_instance()->set_offset({60, 30, 0});

    PresetBundle       bundle;
    DynamicPrintConfig config                                     = bundle.project_config;
    config.option<ConfigOptionStrings>("filament_colour")->values = {"#226699", "#EEAA44"};
    StoreParams store;
    store.path     = path.c_str();
    store.model    = &source;
    store.config   = &config;
    store.strategy = SaveStrategy::Zip64 | SaveStrategy::Silence | SaveStrategy::SkipStatic;
    REQUIRE(store_bbs_3mf(store));

    LoadedProject loaded;
    loaded.model.set_backup_path((scratch.path / "import-backup").string());
    DynamicPrintConfig        imported_config;
    ConfigSubstitutionContext substitutions{ForwardCompatibilitySubstitutionRule::Disable};
    bool                      is_bbs = false;
    Semver                    version;
    REQUIRE(load_bbs_3mf(path.c_str(), &imported_config, &substitutions, &loaded.model, &loaded.plates, &loaded.presets, &is_bbs, &version,
                         nullptr, LoadStrategy::LoadModel | LoadStrategy::LoadConfig));
    REQUIRE(loaded.model.objects.size() == 2);
    const auto opted_in = std::find_if(loaded.model.objects.begin(), loaded.model.objects.end(),
                                       [](const auto* object) { return object->name == "Planar opt-in"; });
    const auto control  = std::find_if(loaded.model.objects.begin(), loaded.model.objects.end(),
                                       [](const auto* object) { return object->name == "Ordinary solid"; });
    REQUIRE(opted_in != loaded.model.objects.end());
    REQUIRE(control != loaded.model.objects.end());
    const auto* stored = (*opted_in)->config.get().option<ConfigOptionFloats>(option);
    REQUIRE(stored != nullptr);
    CHECK(stored->values == settings);
    CHECK_FALSE((*control)->config.has(option));
    REQUIRE((*opted_in)->volumes.size() == 1);
    REQUIRE((*opted_in)->instances.size() == 1);
    check_bounds(world_bounds(*(*opted_in)->volumes.front(), *(*opted_in)->instances.front()), original_bounds.min, original_bounds.max);
}
