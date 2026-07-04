add_library(humanoid_core_robot src/RobotManager.cpp)

add_library(humanoid::robot ALIAS humanoid_core_robot)
set_target_properties(humanoid_core_robot PROPERTIES EXPORT_NAME robot)

target_include_directories(
  humanoid_core_robot
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_robot PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_robot)

install(TARGETS humanoid_core_robot EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/robot)
