#include "OrderedCells.hpp"
#include "TwoCuboidFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

using namespace Slic3r::PlanarGradient;
using namespace Slic3r::PlanarGradient::TestFixtures;

TEST_CASE("Ordered thresholds repeat across signed indices and phase shifts", "[PlanarGradient]")
{
    const OrderedCells base({4.0, -3.0, 1.0}, {1.0, 2.0, 0.5}, {});
    const OrderedCells shifted({4.0, -3.0, 1.0}, {1.0, 2.0, 0.5}, {1, -2, 3});
    REQUIRE(base.center_mm({0, -1, 2}) == Point3{4.5, -4.0, 2.25});
    REQUIRE(shifted.center_mm({0, -1, 2}) == base.center_mm({0, -1, 2}));
    for (double fraction : {0.0, 0.125, 0.5, 0.625, 1.0}) {
        for (std::int32_t z = -4; z < 4; ++z) {
            REQUIRE(base.assign({-1, 2, z}, fraction) == base.assign({3, -2, z + 4}, fraction));
            REQUIRE(shifted.assign({-1, 2, z}, fraction) == base.assign({0, 0, z + 3}, fraction));
        }
    }

    constexpr auto     max = std::numeric_limits<std::int32_t>::max();
    constexpr auto     min = std::numeric_limits<std::int32_t>::min();
    const OrderedCells extremes({}, {1, 1, 1}, {max, min, min});
    REQUIRE(extremes.assign({max, min, max}, 0.25) == base.assign({2, 0, 3}, 0.25));
}

TEST_CASE("Every complete period rounds uniform fractions to the nearest cell", "[PlanarGradient]")
{
    const OrderedCells grid({}, {1, 1, 1}, {-1, 2, -3});
    for (int step = 0; step <= 256; ++step) {
        const double fraction = step / 256.0;
        int          b_count  = 0;
        for (int z = -3; z < 1; ++z)
            for (int y = 7; y < 11; ++y)
                for (int x = -6; x < -2; ++x)
                    b_count += grid.assign({x, y, z}, fraction) == Material::B;

        REQUIRE(b_count == static_cast<int>(std::floor(64 * fraction + 0.5)));
        REQUIRE(std::abs(b_count / 64.0 - fraction) <= 1.0 / 128.0);
    }
}

TEST_CASE("The two cuboids receive labels only at occupied cell centers", "[PlanarGradient]")
{
    const OrderedCells   grid(MATERIAL_A.min_mm, {1, 1, 1}, {});
    const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, 4.0);
    int                  occupied = 0;
    int                  mixed_b  = 0;
    for (int z = -1; z <= 4; ++z) {
        for (int y = -1; y <= 6; ++y) {
            for (int x = -1; x <= 18; ++x) {
                const CellIndex       cell{x, y, z};
                const Point3          center = grid.center_mm(cell);
                std::optional<double> fraction;
                if (contains(MATERIAL_A, center) || contains(MATERIAL_B, center))
                    fraction = field.fraction_b(center);
                const auto label = grid.assign(cell, fraction);
                REQUIRE(label.has_value() == fraction.has_value());
                if (label) {
                    ++occupied;
                    REQUIRE(label == grid.assign(cell, fraction));
                    if (*fraction == 0.0)
                        REQUIRE(label == Material::A);
                    else if (*fraction == 1.0)
                        REQUIRE(label == Material::B);
                    else if (label == Material::B)
                        ++mixed_b;
                }
            }
        }
    }
    REQUIRE(occupied == 18 * 6 * 4);
    REQUIRE(mixed_b == 4 * 6 * 4 / 2);
}

TEST_CASE("Ordered cells reject invalid coordinates and fractions", "[PlanarGradient]")
{
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan      = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_THROWS_AS(OrderedCells({nan, 0, 0}, {1, 1, 1}, {}), std::invalid_argument);
    REQUIRE_THROWS_AS(OrderedCells({}, {1, infinity, 1}, {}), std::invalid_argument);
    REQUIRE_THROWS_AS(OrderedCells({}, {1, 0, 1}, {}), std::invalid_argument);
    REQUIRE_THROWS_AS(OrderedCells({}, {1, -1, 1}, {}), std::invalid_argument);

    const OrderedCells grid({}, {std::numeric_limits<double>::max(), 1, 1}, {});
    REQUIRE_THROWS_AS(grid.center_mm({1, 0, 0}), std::overflow_error);
    for (double fraction : {-0.01, 1.01, nan, infinity})
        REQUIRE_THROWS_AS(grid.assign({}, fraction), std::invalid_argument);
    REQUIRE_FALSE(grid.assign({}, std::nullopt));
}
