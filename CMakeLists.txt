cmake_minimum_required(VERSION 3.16)

project(iterated_circle_inversions VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.80 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
endif()

add_executable(iterated_circle_inversions
    src/iterated_inversion.cpp
    src/main.cpp
    src/util.cpp
    src/circle_set.cpp
    src/circle_tree.cpp
    src/input.cpp
    src/geometry.cpp
    src/image.cpp
)

target_link_libraries(iterated_circle_inversions ${OpenCV_LIBS})