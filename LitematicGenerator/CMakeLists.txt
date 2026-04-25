cmake_minimum_required(VERSION 3.13)
project(LitematicGenerator LANGUAGES CXX)

add_executable(LitematicGenerator
    main.cpp
)

set_release_static_link(LitematicGenerator)

target_link_libraries(LitematicGenerator
    PUBLIC
        ${COMMON_LIBS}
)
