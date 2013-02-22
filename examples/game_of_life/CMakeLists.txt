# Copyright (c) 2013 Jeroen Habraken
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(game_of_life_HEADERS)

set(game_of_life_SOURCES
    game_of_life.cpp)

# add_definitions(-DHPX_ACTION_ARGUMENT_LIMIT=10)
# add_definitions(-DHPX_COMPONENT_CREATE_ARGUMENT_LIMIT=10)
# add_definitions(-DHPX_FUNCTION_ARGUMENT_LIMIT=15)

add_hpx_executable(game_of_life
                   SOURCES ${game_of_life_SOURCES}
                   HEADERS ${game_of_life_HEADERS}
                   FOLDER "Examples/GameOfLife"
                   DEPENDENCIES dataflow_component iostreams_component)

# add pseudo-target dependencies
add_hpx_pseudo_dependencies(examples.game_of_life game_of_life_exe)
