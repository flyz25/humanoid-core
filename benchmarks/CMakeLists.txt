if(NOT TARGET humanoid::cloud_platform)
  message(STATUS "Benchmarks require ENABLE_CLOUD=ON for full v1.0.0 suite; skipping")
  return()
endif()

add_executable(humanoid_core_benchmark_suite benchmark_suite.cpp)

target_link_libraries(
  humanoid_core_benchmark_suite
  PRIVATE humanoid::humanoid_core
          humanoid::plugins
          humanoid::cloud_platform)

humanoid_core_configure_target(humanoid_core_benchmark_suite)

install(TARGETS humanoid_core_benchmark_suite RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
