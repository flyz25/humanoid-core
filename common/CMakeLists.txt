add_library(
  humanoid_core_common
  src/LifecycleState.cpp
  src/Status.cpp
  src/Version.cpp)

add_library(humanoid::common ALIAS humanoid_core_common)
set_target_properties(humanoid_core_common PROPERTIES EXPORT_NAME common)

target_include_directories(
  humanoid_core_common
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

humanoid_core_configure_target(humanoid_core_common)

install(TARGETS humanoid_core_common EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/common)
