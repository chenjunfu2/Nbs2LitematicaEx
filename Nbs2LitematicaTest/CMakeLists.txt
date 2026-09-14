cmake_minimum_required(VERSION 3.13)
project(Nbs2LitematicaTest LANGUAGES CXX)

add_executable(Nbs2LitematicaTest
    main.cpp
)

set_release_static_link(Nbs2LitematicaTest)

target_link_libraries(Nbs2LitematicaTest
    PUBLIC
        ${COMMON_LIBS}
)
