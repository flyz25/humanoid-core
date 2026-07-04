add_library(humanoid_core_robot_factory RobotFactoryRegistry.cpp)
add_library(humanoid::robot_factory ALIAS humanoid_core_robot_factory)
set_target_properties(humanoid_core_robot_factory PROPERTIES EXPORT_NAME robot_factory)

target_include_directories(
  humanoid_core_robot_factory
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_robot_factory
  PUBLIC humanoid::adapter_interfaces)

humanoid_core_configure_target(humanoid_core_robot_factory)

install(
  TARGETS humanoid_core_robot_factory
  EXPORT humanoid_coreTargets)

install(
  FILES RobotFactoryRegistry.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/factory)

install(
  FILES RobotFactoryRegistry.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/humanoid/factory)
