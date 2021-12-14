add_executable(main main.cc)
# target_link_libraries(unit_test PRIVATE PRIVATE Catch2::Catch2 Catch2::Catch2WithMain mynet)
target_link_libraries(main PRIVATE fmt::fmt mynet)