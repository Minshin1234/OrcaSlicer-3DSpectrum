cmake_minimum_required(VERSION 3.13)
project(PlanarGradientTests LANGUAGES CXX)

enable_testing()
include(../../cmake/catch2.cmake)

add_library(planar_fraction STATIC
    ../../src/libslic3r/PlanarGradient/PlanarFraction.cpp
    ../../src/libslic3r/PlanarGradient/OrderedCells.cpp
)
target_include_directories(planar_fraction PUBLIC ../../src/libslic3r/PlanarGradient)
target_compile_features(planar_fraction PUBLIC cxx_std_17)

add_executable(planar_gradient_tests test_planar_fraction.cpp test_ordered_cells.cpp)
target_include_directories(planar_gradient_tests PRIVATE ../data/planar_gradient)
target_link_libraries(planar_gradient_tests PRIVATE planar_fraction Catch2::Catch2WithMain)
catch_discover_tests(planar_gradient_tests)

add_executable(planar_gradient_preview planar_gradient_preview.cpp)
target_include_directories(planar_gradient_preview PRIVATE ../data/planar_gradient)
target_link_libraries(planar_gradient_preview PRIVATE planar_fraction)
