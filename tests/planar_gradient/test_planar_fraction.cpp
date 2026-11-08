#include "PlanarFraction.hpp"
#include "TwoCuboidFixture.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>

using Catch::Approx;
using namespace Slic3r::PlanarGradient;
using namespace Slic3r::PlanarGradient::TestFixtures;

TEST_CASE("Cuboids retain distinct bounds and share the selected interface", "[PlanarGradient]")
{
    const auto& a = MATERIAL_A;
    const auto& b = MATERIAL_B;
    REQUIRE(a.min_mm == Point3{4.0, -3.0, 1.0});
    REQUIRE(b.max_mm == Point3{22.0, 3.0, 5.0});
    REQUIRE(a.min_mm[0] < a.max_mm[0]);
    REQUIRE(b.min_mm[0] < b.max_mm[0]);
    REQUIRE(a.max_mm[0] == b.min_mm[0]);
    REQUIRE(a.max_mm[0] == PLANE_POINT_MM[0]);
    for (std::size_t axis = 1; axis < PLANE_POINT_MM.size(); ++axis) {
        REQUIRE(a.min_mm[axis] == b.min_mm[axis]);
        REQUIRE(a.max_mm[axis] == b.max_mm[axis]);
        REQUIRE(PLANE_POINT_MM[axis] > a.min_mm[axis]);
        REQUIRE(PLANE_POINT_MM[axis] < a.max_mm[axis]);
    }
}

TEST_CASE("Planar fractions span a symmetric transition of the requested width", "[PlanarGradient]")
{
    const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, 4.0);
    REQUIRE(field.fraction_b({9.0, 0.0, 3.0}) == 0.0);
    REQUIRE(field.fraction_b({10.0, 0.0, 3.0}) == 0.0);
    REQUIRE(field.fraction_b({11.0, 0.0, 3.0}) == 0.25);
    REQUIRE(field.fraction_b(PLANE_POINT_MM) == 0.5);
    REQUIRE(field.fraction_b({13.0, 0.0, 3.0}) == 0.75);
    REQUIRE(field.fraction_b({14.0, 0.0, 3.0}) == 1.0);
    REQUIRE(field.fraction_b({15.0, 0.0, 3.0}) == 1.0);

    double previous = 0.0;
    for (int step = 0; step <= 64; ++step) {
        const double fraction = field.fraction_b({8.0 + step / 8.0, 2.0, 4.0});
        REQUIRE(fraction >= previous);
        previous = fraction;
    }
    const PlanarFraction wider(PLANE_POINT_MM, NORMAL_A_TO_B, 8.0);
    REQUIRE(wider.fraction_b({10.0, 0.0, 3.0}) == 0.25);
}

TEST_CASE("A scaled oblique normal still measures physical distance", "[PlanarGradient]")
{
    const PlanarFraction field({1.0, 2.0, 3.0}, {3e307, 4e307, 0.0}, 10.0);
    REQUIRE(field.fraction_b({5.0, -1.0, 8.0}) == Approx(0.5));
    REQUIRE(field.fraction_b({2.5, 4.0, 3.0}) == Approx(0.75));

    const PlanarFraction reversed({1.0, 2.0, 3.0}, {-3.0, -4.0, 0.0}, 10.0);
    REQUIRE(reversed.fraction_b({2.5, 4.0, 3.0}) == Approx(0.25));

    const PlanarFraction small_normal(PLANE_POINT_MM, {1e-300, 0.0, 0.0}, 4.0);
    REQUIRE(small_normal.fraction_b({13.0, 0.0, 3.0}) == 0.75);
}

TEST_CASE("Zero width selects B on the plane", "[PlanarGradient]")
{
    const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, 0.0);
    REQUIRE(field.fraction_b({11.999, 0.0, 3.0}) == 0.0);
    REQUIRE(field.fraction_b(PLANE_POINT_MM) == 1.0);
    REQUIRE(field.fraction_b({12.001, 0.0, 3.0}) == 1.0);
}

TEST_CASE("Invalid parameters and non-finite samples are rejected", "[PlanarGradient]")
{
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan      = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_THROWS_AS(PlanarFraction(PLANE_POINT_MM, NORMAL_A_TO_B, -1.0), std::invalid_argument);
    REQUIRE_THROWS_AS(PlanarFraction(PLANE_POINT_MM, NORMAL_A_TO_B, infinity), std::invalid_argument);
    REQUIRE_THROWS_AS(PlanarFraction(PLANE_POINT_MM, NORMAL_A_TO_B, nan), std::invalid_argument);
    REQUIRE_THROWS_AS(PlanarFraction(PLANE_POINT_MM, Point3{}, 1.0), std::invalid_argument);
    REQUIRE_THROWS_AS(PlanarFraction(PLANE_POINT_MM, {0.0, infinity, 0.0}, 1.0), std::invalid_argument);
    REQUIRE_THROWS_AS(PlanarFraction({nan, 0.0, 0.0}, NORMAL_A_TO_B, 1.0), std::invalid_argument);

    const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, 4.0);
    REQUIRE_THROWS_AS(field.fraction_b({0.0, nan, 0.0}), std::invalid_argument);
    REQUIRE_THROWS_AS(field.fraction_b({infinity, 0.0, 0.0}), std::invalid_argument);

    const double         largest = std::numeric_limits<double>::max();
    const PlanarFraction far_field({-largest, 0.0, 0.0}, NORMAL_A_TO_B, 4.0);
    REQUIRE_THROWS_AS(far_field.fraction_b({largest, 0.0, 0.0}), std::overflow_error);
}
