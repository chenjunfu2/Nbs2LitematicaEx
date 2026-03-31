cmake_minimum_required(VERSION 3.13)
project(Nbs2LitematicEx LANGUAGES CXX)

add_executable(Nbs2LitematicEx
    main.cpp
)

target_link_libraries(Nbs2LitematicEx
    PUBLIC
        ${COMMON_LIBS}
)
