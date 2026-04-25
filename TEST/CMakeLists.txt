cmake_minimum_required(VERSION 3.13)
project(TEST LANGUAGES CXX)

add_executable(TEST
    main.cpp
)

set_release_static_link(TEST)

target_link_libraries(TEST
    PUBLIC
        ${COMMON_LIBS}
)
