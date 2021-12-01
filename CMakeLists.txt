cmake_minimum_required(VERSION 3.22)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
project(mynet)

# Create the executable
set(CMAKE_CXX_STANDARD 20)



add_custom_command(
    OUTPUT game_generated.h
    COMMAND flatc --cpp -o ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/schema/game.fbs 
    )
add_executable(${PROJECT_NAME} src/main.cc game_generated.h)


target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)


find_package(Flatbuffers CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE flatbuffers::flatbuffers)
# Link the static library from subproject1 using its alias sub::lib1
# Link the header only library from subproject2 using its alias sub::lib2
# This will cause the include directories for that target to be added to this project
# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#     sub::lib1
#     sub::lib2
# )

# target_include_directories(${PROJECT_NAME}
#     INTERFACE
#     ${PROJECT_SOURCE_DIR}
# )
