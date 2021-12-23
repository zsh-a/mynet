add_executable(main main.cc)


set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g3")
# target_link_libraries(unit_test PRIVATE PRIVATE Catch2::Catch2 Catch2::Catch2WithMain mynet)
target_link_libraries(main PRIVATE fmt::fmt  pthread mynet )

add_subdirectory(pingpong)
add_subdirectory(chat)
add_subdirectory(roundtrip)
add_subdirectory(relay)
add_subdirectory(tinyhttp)