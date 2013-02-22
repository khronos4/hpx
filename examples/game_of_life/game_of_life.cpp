// When using the dataflow component we have to define the following constant
// as this component uses up to 6 arguments for one of its components.
#define FUSION_MAX_VECTOR_SIZE 20
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define HPX_LIMIT 12

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include <hpx/components/dataflow/dataflow.hpp>

#include <array>
#include <iostream>

bool evolve(bool nw, bool n, bool ne,
            bool w,  bool o, bool e,
            bool sw, bool s, bool se)
{
    std::size_t live_neighbours = nw + n + ne + w + e + sw + s + se;
    if (o)
    {
        switch (live_neighbours)
        {
            case 2:
            case 3:
                return true;
            default:
                return false;
        }
    }
    else
    {
        return live_neighbours == 3;
    }
}
HPX_PLAIN_ACTION(evolve, evolve_action)

bool flyer(std::size_t x, std::size_t y)
{
    if ((x == 3 && y == 4) ||
         (x == 4 && y == 5) ||
         (x == 5 && (y == 3 || y == 4 || y == 5)))
    {
        return true;
    }

    return false;
}
HPX_PLAIN_ACTION(flyer, flyer_action);

int hpx_main(int argc, char* argv[])
{
    hpx::naming::id_type here = hpx::find_here();

    std::array<hpx::lcos::dataflow_base<bool>, 81> grid, grid_2;

    for (std::size_t x = 0; x < 9; ++x)
    {
        for (std::size_t y = 0; y < 9; ++y)
        {
            grid[x * 9 + y] = hpx::lcos::dataflow<flyer_action>(here, x, y);
        }
    }

    std::array<hpx::lcos::dataflow_base<bool>, 81> & grid_input = grid;
    std::array<hpx::lcos::dataflow_base<bool>, 81> & grid_output = grid_2;

    for (std::size_t evolution = 0; evolution < 4; ++evolution)
    {
        for (std::size_t x = 0; x < 9; ++x)
        {
            for (std::size_t y = 0; y < 9; ++y)
            {
                grid_output[x * 9 + y] = hpx::lcos::dataflow<evolve_action>(here,
                    grid_input[((x - 1) % 9) * 9 + ((y - 1) % 9)], grid_input[((x - 1) % 9) * 9 + y], grid_input[((x - 1) % 9) * 9 + ((y + 1) % 9)],
                    grid_input[x * 9 + ((y - 1) % 9)],             grid_input[x * 9 + y],             grid_input[x * 9 + ((y + 1) % 9)],
                    grid_input[((x + 1) % 9) * 9 + ((y - 1) % 9)], grid_input[((x + 1) % 9) * 9 + y], grid_input[((x + 1) % 9) * 9 + ((y + 1) % 9)]
                );
            }
        }

        std::swap(grid_input, grid_output);
    }

    for (std::size_t x = 0; x < 9; ++x)
    {
        for (std::size_t y = 0; y < 9; ++y)
        {
            std::cout << grid_input[x * 9 + y].get_future().get();
        }
        std::cout << std::endl;
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
     return hpx::init(argc, argv);
}
