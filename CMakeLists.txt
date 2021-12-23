cmake_minimum_required(VERSION 3.22)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
project(mynet)

# Create the executable
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} src/event_loop.cc src/epoller.cc src/connection.cc src/log.cc)
target_include_directories(${PROJECT_NAME}
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt-header-only pthread)

# find_package(Catch2 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
# find_path(NANOBENCH_INCLUDE_DIRS "nanobench.h")

add_subdirectory(test)

enable_testing()


