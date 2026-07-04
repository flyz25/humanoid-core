add_executable(humanoid_core_smoke_test smoke_test.cpp)

target_link_libraries(
  humanoid_core_smoke_test
  PRIVATE humanoid::adapter_interfaces
          humanoid::robot_factory
          humanoid::core
          humanoid::telemetry_service
          humanoid::logging)

humanoid_core_configure_target(humanoid_core_smoke_test)

add_test(NAME humanoid_core_smoke_test COMMAND humanoid_core_smoke_test)

add_executable(humanoid_core_robot_state_unit_test robot_state_unit_test.cpp)

target_link_libraries(
  humanoid_core_robot_state_unit_test
  PRIVATE humanoid::core
          humanoid::telemetry_service)

humanoid_core_configure_target(humanoid_core_robot_state_unit_test)

add_test(NAME humanoid_core_robot_state_unit_test COMMAND humanoid_core_robot_state_unit_test)

add_executable(humanoid_core_plugin_registry_unit_test plugin_registry_unit_test.cpp)

target_link_libraries(
  humanoid_core_plugin_registry_unit_test
  PRIVATE humanoid::plugins)

humanoid_core_configure_target(humanoid_core_plugin_registry_unit_test)

add_test(NAME humanoid_core_plugin_registry_unit_test
         COMMAND humanoid_core_plugin_registry_unit_test)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
  if(HUMANOID_CORE_REQUIRE_GTEST)
    message(FATAL_ERROR "GoogleTest was requested but was not found")
  endif()

  message(STATUS "GoogleTest not found; optional GoogleTest unit tests are not built")
  return()
endif()

add_executable(
  humanoid_core_sample_unit_test
  sample_unit_test.cpp
  mock_robot_adapter_test.cpp)

target_link_libraries(
  humanoid_core_sample_unit_test
  PRIVATE humanoid::humanoid_core
          humanoid::adapter_interfaces
          humanoid::robot_factory
          GTest::gtest_main)

humanoid_core_configure_target(humanoid_core_sample_unit_test)

include(GoogleTest)
gtest_discover_tests(humanoid_core_sample_unit_test)
