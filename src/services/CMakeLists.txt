add_library(humanoid_core_telemetry_service TelemetryService.cpp)
add_library(humanoid::telemetry_service ALIAS humanoid_core_telemetry_service)
set_target_properties(humanoid_core_telemetry_service PROPERTIES EXPORT_NAME telemetry_service)

target_include_directories(
  humanoid_core_telemetry_service
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_telemetry_service
  PUBLIC humanoid::core)

humanoid_core_configure_target(humanoid_core_telemetry_service)

install(
  TARGETS humanoid_core_telemetry_service
  EXPORT humanoid_coreTargets)

install(
  FILES TelemetryService.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/services)
