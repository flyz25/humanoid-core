add_library(
  humanoid_core_safety
  src/SafetyManager.cpp
  src/SafetyState.cpp)

add_library(humanoid::safety ALIAS humanoid_core_safety)
set_target_properties(humanoid_core_safety PROPERTIES EXPORT_NAME safety)

target_include_directories(
  humanoid_core_safety
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_safety PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_safety)

install(TARGETS humanoid_core_safety EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/safety)
