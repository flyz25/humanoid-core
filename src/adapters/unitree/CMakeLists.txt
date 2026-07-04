add_library(
  humanoid_core_unitree_adapter
  UnitreeG1Adapter.cpp
  UnitreeRobotFactory.cpp)

add_library(humanoid::unitree_adapter ALIAS humanoid_core_unitree_adapter)
set_target_properties(humanoid_core_unitree_adapter PROPERTIES EXPORT_NAME unitree_adapter)

target_include_directories(
  humanoid_core_unitree_adapter
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_unitree_adapter
  PUBLIC humanoid::adapter_interfaces
  PRIVATE humanoid::logging
          humanoid::unitree_loco_client)

humanoid_core_configure_target(humanoid_core_unitree_adapter)

install(
  TARGETS humanoid_core_unitree_adapter
  EXPORT humanoid_coreTargets)

install(
  FILES UnitreeG1Adapter.h
        UnitreeRobotFactory.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adapters/unitree)

install(
  FILES UnitreeG1Adapter.h
        UnitreeRobotFactory.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/humanoid/adapters/unitree)
