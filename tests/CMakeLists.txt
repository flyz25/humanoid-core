find_package(GTest QUIET)

if(NOT GTest_FOUND)
  if(HUMANOID_CORE_REQUIRE_GTEST)
    message(FATAL_ERROR "GoogleTest was requested but was not found")
  endif()

  message(STATUS "GoogleTest not found; humanoid-core unit tests are not built")
  return()
endif()

add_executable(humanoid_core_sample_unit_test sample_unit_test.cpp)

target_link_libraries(
  humanoid_core_sample_unit_test
  PRIVATE humanoid::humanoid_core
          GTest::gtest_main)

humanoid_core_configure_target(humanoid_core_sample_unit_test)

include(GoogleTest)
gtest_discover_tests(humanoid_core_sample_unit_test)
