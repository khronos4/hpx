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
    if(live_neighbours < 2 || live_neighbours > 3){
        return false;
    }

    if(live_neighbours == 3){
        return true;
    }

    if (o && live_neighbours == 2)
    {
        return true;
    }

    return false;
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

int hpx_main(boost::program_options::variables_map & vm)
{
    std::size_t evolutions = vm["evolutions"].as<std::size_t>();

    hpx::naming::id_type here = hpx::find_here();

    std::array<hpx::lcos::dataflow_base<bool>, 81> grid[2];

    for (std::size_t x = 0; x < 9; ++x)
    {
        for (std::size_t y = 0; y < 9; ++y)
        {
            grid[0][(x * 9) + y] = hpx::lcos::dataflow<flyer_action>(here, x, y);
        }
    }

    for (std::size_t e = 0; e < evolutions; ++e)
    {
        for (std::size_t x = 0; x < 9; ++x)
        {
            for (std::size_t y = 0; y < 9; ++y)
            {
                grid[(e + 1) % 2][(x * 9) + y] = hpx::lcos::dataflow<evolve_action>(here,
                    grid[e % 2][((((x - 1) % 9) * 9) + ((y - 1) % 9))], grid[e % 2][((((x - 1) % 9) * 9) + y)], grid[e % 2][((((x - 1) % 9) * 9) + ((y + 1) % 9))],
                    grid[e % 2][((x * 9)             + ((y - 1) % 9))], grid[e % 2][(x * 9)              + y],       grid[e % 2][((x * 9)             + ((y + 1) % 9))],
                    grid[e % 2][((((x + 1) % 9) * 9) + ((y - 1) % 9))], grid[e % 2][((((x + 1) % 9) * 9) + y)], grid[e % 2][((((x + 1) % 9) * 9) + ((y + 1) % 9))]
                );
            }
        }
    }

    for (std::size_t x = 0; x < 9; ++x)
    {
        for (std::size_t y = 0; y < 9; ++y)
        {
            std::cout << grid[evolutions % 2][(x * 9) + y].get_future().get();
        }
        std::cout << std::endl;
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    boost::program_options::options_description
        desc_commandline("usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        (
            "evolutions"
          , boost::program_options::value<std::size_t>()->default_value(4)
          , "Number of evolutions"
        )
        ;

    return hpx::init(desc_commandline, argc, argv);
}
