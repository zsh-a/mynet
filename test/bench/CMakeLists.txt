add_executable(bench main.cc)
find_path(NANOBENCH_INCLUDE_DIRS "nanobench.h")
target_include_directories(bench PRIVATE ${NANOBENCH_INCLUDE_DIRS})
target_link_libraries(bench PRIVATE mynet fmt::fmt)