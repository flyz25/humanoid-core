add_library(
  humanoid_core_motion
  src/MotionManager.cpp
  src/MotionMode.cpp)

add_library(humanoid::motion ALIAS humanoid_core_motion)
set_target_properties(humanoid_core_motion PROPERTIES EXPORT_NAME motion)

target_include_directories(
  humanoid_core_motion
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_motion PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_motion)

install(TARGETS humanoid_core_motion EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/motion)
