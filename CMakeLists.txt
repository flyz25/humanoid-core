cmake_minimum_required(VERSION 3.22)

project(
  humanoid-core
  VERSION 0.11.0
  DESCRIPTION "Clean architecture C++20 framework foundation for humanoid robotics"
  LANGUAGES CXX)

set(HUMANOID_CORE_VERSION_PRERELEASE "alpha")
set(HUMANOID_CORE_SEMANTIC_VERSION "${PROJECT_VERSION}-${HUMANOID_CORE_VERSION_PRERELEASE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HUMANOID_CORE_BUILD_EXAMPLES "Build humanoid-core examples" ON)
option(HUMANOID_CORE_BUILD_TESTS "Build humanoid-core tests when GoogleTest is available" ON)
option(ENABLE_UNITREE "Enable Unitree SDK2 adapter targets when Unitree SDK2 is available" ON)
option(ENABLE_ROS2 "Enable optional ROS2 integration targets when ROS2 is available" ON)
option(HUMANOID_CORE_REQUIRE_GTEST "Fail configuration if GoogleTest is unavailable" OFF)
option(HUMANOID_CORE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(HumanoidCoreCompilerOptions)

add_subdirectory(third_party)
add_subdirectory(common)
add_subdirectory(plugins)
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
add_subdirectory(plugins/unitree/sdk)
add_subdirectory(src)
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
            humanoid::adapter_interfaces
            humanoid::robot_factory
            humanoid::telemetry_service
            humanoid::logging
            humanoid::configuration
            humanoid::robot
            humanoid::motion
            humanoid::gesture
            humanoid::safety
            humanoid::diagnostics
            humanoid::network)

add_subdirectory(plugins/unitree)
add_subdirectory(ros2)

if(HUMANOID_CORE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTING AND HUMANOID_CORE_BUILD_TESTS)
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(TARGETS humanoid_core humanoid_core_options EXPORT humanoid_coreTargets)
install(FILES cmake/FindUnitreeSDK2.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/humanoid_core)

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

set(CPACK_PACKAGE_NAME "humanoid-core")
set(CPACK_PACKAGE_VENDOR "humanoid-core")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${HUMANOID_CORE_SEMANTIC_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ")
include(CPack)
