#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "libslic3r/CutUtils.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelArrange.hpp"

#include <boost/nowide/cstdio.hpp>
#include <boost/filesystem.hpp>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("Model construction", "[Model]") {
    GIVEN("A Slic3r Model") {
		Slic3r::Model model;
        Slic3r::TriangleMesh sample_mesh = Slic3r::make_cube(20,20,20);
        Slic3r::DynamicPrintConfig config = Slic3r::Test::default_print_config();
        Slic3r::Print print;

        WHEN("Model object is added") {
            Slic3r::ModelObject *model_object = model.add_object();
            THEN("Model object list == 1") {
                REQUIRE(model.objects.size() == 1);
            }
            model_object->add_volume(sample_mesh);
            THEN("Model volume list == 1") {
                REQUIRE(model_object->volumes.size() == 1);
            }
            THEN("Model volume is a part") {
                REQUIRE(model_object->volumes.front()->is_model_part());
            }
            THEN("Mesh is equivalent to input mesh.") {
                REQUIRE(! sample_mesh.its.vertices.empty());
				const std::vector<Vec3f>& mesh_vertices = model_object->volumes.front()->mesh().its.vertices;
				Vec3f mesh_offset = model_object->volumes.front()->source.mesh_offset.cast<float>();
				for (size_t i = 0; i < sample_mesh.its.vertices.size(); ++ i) {
					const Vec3f &p1 = sample_mesh.its.vertices[i];
					const Vec3f  p2 = mesh_vertices[i] + mesh_offset;
					REQUIRE((p2 - p1).norm() < EPSILON);
				}
            }
            model_object->add_instance();
            arrange_objects(model, get_bed_shape(config), ArrangeParams{scaled(min_object_distance(config))}, [](arrangement::ArrangePolygon&) {});
			model_object->ensure_on_bed();
			print.auto_assign_extruders(model_object);
			THEN("Print works?") {
				print.set_status_silent();
				print.apply(model, config);
				print.process();
				boost::filesystem::path temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
                GCodeProcessorResult result;
                const std::string output_path = print.export_gcode(temp.string(), &result, nullptr);
                REQUIRE(boost::filesystem::exists(output_path));
				REQUIRE(boost::filesystem::is_regular_file(output_path));
				REQUIRE(boost::filesystem::file_size(output_path) > 0);
				boost::nowide::remove(output_path.c_str());
			}
        }
    }
}

TEST_CASE("A planar material split keeps both parts in place and assigns their filament slots", "[Model][cut]")
{
    Model model;
    ModelObject* object = model.add_object();
    ModelVolume* source = object->add_volume(make_cube(20, 20, 20));
    source->config.set_key_value("extruder", new ConfigOptionInt(1));
    ModelInstance* instance = object->add_instance();
    instance->set_offset(Vec3d(45., 60., 0.));
    const BoundingBoxf3 original_bounds = object->instance_bounding_box(0);

    SECTION("The two solids share the selected boundary")
    {
        const double plane_z = original_bounds.min.z() + 7.;
        Cut cut(object, 0, Geometry::translation_transform(Vec3d(0., 0., plane_z)));
        const auto& parts = cut.perform_with_plane();
        REQUIRE(parts.size() == 1);
        REQUIRE(parts.front()->volumes.size() == 2);
        REQUIRE_FALSE(assign_planar_cut_filaments(*parts.front(), 2, 2));
        for (const ModelVolume* volume : parts.front()->volumes)
            CHECK(volume->extruder_id() == 1);
        REQUIRE(assign_planar_cut_filaments(*parts.front(), 2, 1));

        const auto bounds = parts.front()->instance_bounding_box(0);
        CHECK(bounds.min.isApprox(original_bounds.min));
        CHECK(bounds.max.isApprox(original_bounds.max));
        for (const ModelVolume* volume : parts.front()->volumes) {
            const auto matrix = parts.front()->instances.front()->get_matrix() * volume->get_matrix();
            const auto part_bounds = volume->mesh().transformed_bounding_box(matrix);
            if (volume->is_from_upper()) {
                CHECK(volume->extruder_id() == 2);
                CHECK(part_bounds.min.z() == Catch::Approx(plane_z));
            } else {
                CHECK(volume->extruder_id() == 1);
                CHECK(part_bounds.max.z() == Catch::Approx(plane_z));
            }
        }
    }

    SECTION("The split follows a tilted plane")
    {
        Transform3d plane = Geometry::translation_transform(original_bounds.center() - instance->get_offset());
        plane.rotate(Eigen::AngleAxisd(0.35, Vec3d::UnitY()));
        Cut cut(object, 0, plane);
        const auto& parts = cut.perform_with_plane();
        REQUIRE(parts.size() == 1);
        REQUIRE(parts.front()->volumes.size() == 2);
        REQUIRE(assign_planar_cut_filaments(*parts.front(), 1, 2));

        const auto bounds = parts.front()->instance_bounding_box(0);
        CHECK((bounds.min - original_bounds.min).norm() < 1e-5);
        CHECK((bounds.max - original_bounds.max).norm() < 1e-5);
        for (const ModelVolume* volume : parts.front()->volumes) {
            const Transform3d to_plane = plane.inverse() * volume->get_matrix();
            const auto part_bounds = volume->mesh().transformed_bounding_box(to_plane);
            if (volume->is_from_upper())
                CHECK(part_bounds.min.z() == Catch::Approx(0.).margin(1e-5));
            else
                CHECK(part_bounds.max.z() == Catch::Approx(0.).margin(1e-5));
        }
    }

    SECTION("A plane that misses the solid cannot replace the source")
    {
        Cut cut(object, 0, Geometry::translation_transform(Vec3d(0., 0., original_bounds.max.z() + 5.)));
        const auto& parts = cut.perform_with_plane();
        REQUIRE(parts.size() == 1);
        REQUIRE_FALSE(assign_planar_cut_filaments(*parts.front(), 2, 1));
        REQUIRE(parts.front()->volumes.size() == 1);
        CHECK(parts.front()->volumes.front()->extruder_id() == 1);
    }

    REQUIRE(object->volumes.size() == 1);
    CHECK(source->extruder_id() == 1);
}
