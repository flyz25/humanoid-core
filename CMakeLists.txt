cmake_minimum_required(VERSION 3.22)

project(
  humanoid-core
  VERSION 0.1.0
  DESCRIPTION "Clean architecture C++17 framework foundation for humanoid robotics"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HUMANOID_CORE_BUILD_EXAMPLES "Build humanoid-core examples" ON)
option(HUMANOID_CORE_BUILD_TESTS "Build humanoid-core tests when GoogleTest is available" ON)
option(HUMANOID_CORE_REQUIRE_GTEST "Fail configuration if GoogleTest is unavailable" OFF)
option(HUMANOID_CORE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(HumanoidCoreCompilerOptions)

add_subdirectory(third_party)
add_subdirectory(common)
add_subdirectory(utilities)
add_subdirectory(logging)
add_subdirectory(configuration)
add_subdirectory(robot)
add_subdirectory(motion)
add_subdirectory(gesture)
add_subdirectory(safety)
add_subdirectory(diagnostics)
add_subdirectory(network)
add_subdirectory(core)
add_subdirectory(apps)

add_library(humanoid_core INTERFACE)
add_library(humanoid::humanoid_core ALIAS humanoid_core)
set_target_properties(humanoid_core PROPERTIES EXPORT_NAME humanoid_core)

target_include_directories(
  humanoid_core
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core
  INTERFACE humanoid::core
            humanoid::common
            humanoid::utilities
            humanoid::logging
            humanoid::configuration
            humanoid::robot
            humanoid::motion
            humanoid::gesture
            humanoid::safety
            humanoid::diagnostics
            humanoid::network)

if(HUMANOID_CORE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTING AND HUMANOID_CORE_BUILD_TESTS)
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(TARGETS humanoid_core humanoid_core_options EXPORT humanoid_coreTargets)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/humanoid_coreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/humanoid_coreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/humanoid_coreConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/humanoid_core)

install(
  EXPORT humanoid_coreTargets
  NAMESPACE humanoid::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/humanoid_core)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/humanoid_coreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/humanoid_coreConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/humanoid_core)
