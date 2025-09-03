cmake_minimum_required(VERSION 3.15)
project(MPSO)

add_definitions(-w)
SET(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

include_directories(common)
include_directories(offlineGen)
include_directories(shuffle)
include_directories(mpso)

file(GLOB_RECURSE SRCS
    common/*.cpp
    offlineGen/*.cpp
    shuffle/*.cpp
    mpso/*.cpp
)

find_package(volePSI REQUIRED HINTS "./libvolepsi")
set(CMAKE_BUILD_TYPE Release)


# for test
add_executable(test_mpsi test/test_mpsi.cpp  ${SRCS})
target_compile_options(test_mpsi PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(test_mpsi visa::volePSI)

add_executable(test_mpsic test/test_mpsic.cpp  ${SRCS})
target_compile_options(test_mpsic PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(test_mpsic visa::volePSI)

add_executable(test_mpsics test/test_mpsics.cpp  ${SRCS})
target_compile_options(test_mpsics PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(test_mpsics visa::volePSI)

add_executable(test_mpsu test/test_mpsu.cpp  ${SRCS})
target_compile_options(test_mpsu PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(test_mpsu visa::volePSI)














