#include "OrderedCells.hpp"
#include "TwoCuboidFixture.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <optional>
#include <stdexcept>

using namespace Slic3r::PlanarGradient;
using namespace Slic3r::PlanarGradient::TestFixtures;

namespace {

constexpr Point3    PITCH_MM{1.0, 1.0, 1.0};
constexpr CellIndex PHASE{};
constexpr double    WIDTH_MM = 4.0;
constexpr int       NX       = static_cast<int>((MATERIAL_B.max_mm[0] - MATERIAL_A.min_mm[0]) / PITCH_MM[0]);
constexpr int       NY       = static_cast<int>((MATERIAL_B.max_mm[1] - MATERIAL_A.min_mm[1]) / PITCH_MM[1]);
constexpr int       NZ       = static_cast<int>((MATERIAL_B.max_mm[2] - MATERIAL_A.min_mm[2]) / PITCH_MM[2]);

struct Cell
{
    double                  target_b;
    std::optional<Material> label;
};

using Cells = std::array<std::array<std::array<Cell, NX>, NY>, NZ>;

struct Stats
{
    int    occupied   = 0;
    int    assigned_b = 0;
    double target_sum = 0.0;

    void add(const Cell& cell)
    {
        if (cell.label) {
            ++occupied;
            assigned_b += cell.label == Material::B;
            target_sum += cell.target_b;
        }
    }

    void write(std::ostream& out) const
    {
        const double target = target_sum / occupied;
        const double actual = static_cast<double>(assigned_b) / occupied;
        out << occupied << ',' << assigned_b << ',' << target << ',' << actual << ',' << 100 * (actual - target) << '\n';
    }
};

std::ofstream open_output(const std::filesystem::path& path)
{
    std::ofstream out;
    out.exceptions(std::ios::failbit | std::ios::badbit);
    out.imbue(std::locale::classic());
    out.open(path, std::ios::binary);
    return out;
}

void write_metrics(const std::filesystem::path& path, const Cells& cells)
{
    std::array<Stats, NX> slabs{};
    Stats                 total, transition;
    for (const auto& layer : cells)
        for (const auto& row : layer)
            for (std::size_t x = 0; x < row.size(); ++x) {
                const Cell& cell = row[x];
                slabs[x].add(cell);
                total.add(cell);
                if (cell.target_b > 0.0 && cell.target_b < 1.0)
                    transition.add(cell);
            }

    auto out = open_output(path);
    out << "window,x_min_mm,x_max_mm,occupied_cells,b_cells,target_b_fraction,actual_b_fraction,error_percentage_points\n";
    out << std::fixed << std::setprecision(6) << "overall," << MATERIAL_A.min_mm[0] << ',' << MATERIAL_B.max_mm[0] << ',';
    total.write(out);
    out << "transition," << PLANE_POINT_MM[0] - WIDTH_MM / 2 << ',' << PLANE_POINT_MM[0] + WIDTH_MM / 2 << ',';
    transition.write(out);
    for (std::size_t x = 0; x < slabs.size(); ++x) {
        const double left = MATERIAL_A.min_mm[0] + x * PITCH_MM[0];
        out << "x_slab," << left << ',' << left + PITCH_MM[0] << ',';
        slabs[x].write(out);
    }
    out.close();
}

constexpr int PIXELS = 20;

template<class Sample> void write_slice(std::ostream& out, int left, int top, int rows, const char* title, double coordinate, Sample sample)
{
    out << "<text x='" << left << "' y='" << top - 12 << "' font-size='16' font-weight='600'>" << title << coordinate << " mm</text>\n";
    for (int row = 0; row < rows; ++row) {
        for (int x = 0; x < NX; ++x) {
            const auto  label = sample(x, rows - row - 1).label;
            const char* color = !label ? "#eff2f5" : *label == Material::A ? "#276394" : "#e5a44d";
            out << "<rect x='" << left + x * PIXELS << "' y='" << top + row * PIXELS << "' width='" << PIXELS - 1 << "' height='"
                << PIXELS - 1 << "' fill='" << color << "'/>\n";
        }
    }
    const int interface_x = left + static_cast<int>((PLANE_POINT_MM[0] - MATERIAL_A.min_mm[0]) / PITCH_MM[0]) * PIXELS;
    const int bottom      = top + rows * PIXELS;
    out << "<path d='M" << interface_x << ' ' << top - 3 << "V" << bottom + 3
        << "' stroke='#172635' stroke-width='2' stroke-dasharray='4 3'/>\n";
    out << "<text x='" << left << "' y='" << bottom + 18 << "' font-size='13'>x = 4 mm</text>\n"
        << "<text x='" << left + NX * PIXELS << "' y='" << bottom + 18 << "' text-anchor='end' font-size='13'>22 mm</text>\n";
}

void write_preview(const std::filesystem::path& path, const Cells& cells, const OrderedCells& grid)
{
    auto out = open_output(path);
    out << R"SVG(<svg xmlns='http://www.w3.org/2000/svg' width='832' height='670' viewBox='0 0 832 670' role='img'>
<title>Material assignment through a planar transition</title>
<desc>Four horizontal XY layers and two vertical XZ sections of the two-cuboid fixture. Blue cells are A and amber cells are B.</desc>
<rect width='832' height='670' fill='#fbfaf7'/>
<g font-family='Segoe UI, Arial, sans-serif' fill='#172635'>
<text x='36' y='47' font-size='28' font-weight='700'>Material assignment through a planar transition</text>
<text x='36' y='76' font-size='15'>Two cuboids &#183; 1 mm cells &#183; 4 mm transition &#183; phase (0, 0, 0)</text>
<rect x='36' y='93' width='14' height='14' fill='#276394'/><text x='57' y='105' font-size='14'>Material A</text>
<rect x='159' y='93' width='14' height='14' fill='#e5a44d'/><text x='180' y='105' font-size='14'>Material B</text>
<path d='M282 100h25' stroke='#172635' stroke-width='2' stroke-dasharray='4 3'/>
<text x='315' y='105' font-size='14'>Interface at x = 12 mm</text>
<text x='36' y='139' font-size='13'>XY views read from y = 3 mm at the top to y = &#8722;3 mm at the bottom.</text>
)SVG";
    for (int z = 0; z < NZ; ++z)
        write_slice(out, 36 + (z % 2) * 400, 174 + (z / 2) * 173, NY, "XY at z = ", grid.center_mm({0, 0, z})[2],
                    [&](int x, int y) -> const Cell& { return cells[z][y][x]; });

    out << "<text x='36' y='511' font-size='13'>XZ views read from z = 5 mm at the top to z = 1 mm at the bottom.</text>\n";
    for (int y : {2, 3})
        write_slice(out, 36 + (y - 2) * 400, 547, NZ, "XZ at y = ", grid.center_mm({0, y, 0})[1],
                    [&](int x, int z) -> const Cell& { return cells[z][y][x]; });
    out << "</g>\n</svg>\n";
    out.close();
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: planar_gradient_preview OUTPUT_DIRECTORY\n";
        return 2;
    }

    try {
        const OrderedCells   grid(MATERIAL_A.min_mm, PITCH_MM, PHASE);
        const PlanarFraction field(PLANE_POINT_MM, NORMAL_A_TO_B, WIDTH_MM);
        Cells                cells{};
        const auto           start = std::chrono::steady_clock::now();
        for (int z = 0; z < NZ; ++z)
            for (int y = 0; y < NY; ++y)
                for (int x = 0; x < NX; ++x) {
                    const CellIndex index{x, y, z};
                    const Point3    center = grid.center_mm(index);
                    if (contains(MATERIAL_A, center) || contains(MATERIAL_B, center)) {
                        const double fraction = field.fraction_b(center);
                        cells[z][y][x]        = {fraction, grid.assign(index, fraction)};
                    }
                }
        const auto elapsed = std::chrono::steady_clock::now() - start;

        const std::filesystem::path output(argv[1]);
        std::filesystem::create_directories(output);
        write_metrics(output / "material-error.csv", cells);
        write_preview(output / "planar-preview.svg", cells, grid);
        std::cout << "Cell sampling and assignment: " << std::chrono::duration<double, std::micro>(elapsed).count()
                  << " us (excludes file output)\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
