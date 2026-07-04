add_library(
  humanoid_core_diagnostics
  src/DiagnosticManager.cpp
  src/DiagnosticRecord.cpp
  src/DiagnosticStatus.cpp)

add_library(humanoid::diagnostics ALIAS humanoid_core_diagnostics)
set_target_properties(humanoid_core_diagnostics PROPERTIES EXPORT_NAME diagnostics)

target_include_directories(
  humanoid_core_diagnostics
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_diagnostics PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_diagnostics)

install(TARGETS humanoid_core_diagnostics EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/diagnostics)
