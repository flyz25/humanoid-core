if(ENABLE_CLOUD)
  message(STATUS "ENABLE_CLOUD=ON; building optional cloud platform targets")
else()
  message(STATUS "ENABLE_CLOUD=OFF; cloud platform targets are disabled")
  return()
endif()

add_library(
  humanoid_core_cloud_platform
  CloudPlatform.cpp
  api/RestApiCatalog.cpp
  auth/AuthManager.cpp
  fleet/FleetManager.cpp
  grpc/GrpcCatalog.cpp
  monitoring/ObservabilityRegistry.cpp
  ota/OtaManager.cpp
  websocket/WebSocketCatalog.cpp)

add_library(humanoid::cloud_platform ALIAS humanoid_core_cloud_platform)
set_target_properties(humanoid_core_cloud_platform PROPERTIES EXPORT_NAME cloud_platform)

target_include_directories(
  humanoid_core_cloud_platform
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/core/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_cloud_platform PUBLIC humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_cloud_platform)

install(TARGETS humanoid_core_cloud_platform EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR}/cloud)
install(FILES api/openapi.yaml DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cloud/api)
install(FILES grpc/humanoid_cloud.proto
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cloud/grpc)
install(FILES websocket/streams.yaml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cloud/websocket)
install(FILES dashboard/dashboard_api.yaml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cloud/dashboard)
install(DIRECTORY deployment/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cloud/deployment)

function(humanoid_core_add_cloud_example target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE humanoid::cloud_platform)
  humanoid_core_configure_target(${target})
  install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

if(HUMANOID_CORE_BUILD_EXAMPLES)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_fleet_manager_example
    examples/fleet_manager/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_rest_api_example
    examples/rest_api/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_websocket_example
    examples/websocket/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_mission_upload_example
    examples/mission_upload/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_remote_robot_control_example
    examples/remote_robot_control/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_telemetry_example
    examples/cloud_telemetry/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_robot_registration_example
    examples/robot_registration/main.cpp)
  humanoid_core_add_cloud_example(
    humanoid_core_cloud_ota_update_example
    examples/ota_update/main.cpp)
endif()

if(BUILD_TESTING AND HUMANOID_CORE_BUILD_TESTS)
  add_executable(humanoid_core_cloud_platform_unit_test tests/cloud_platform_unit_test.cpp)
  target_link_libraries(humanoid_core_cloud_platform_unit_test PRIVATE humanoid::cloud_platform)
  humanoid_core_configure_target(humanoid_core_cloud_platform_unit_test)
  add_test(NAME humanoid_core_cloud_platform_unit_test
           COMMAND humanoid_core_cloud_platform_unit_test)
endif()
