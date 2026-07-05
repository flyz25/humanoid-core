add_executable(humanoid_core_smoke_test smoke_test.cpp)

target_link_libraries(
  humanoid_core_smoke_test
  PRIVATE humanoid::adapter_interfaces
          humanoid::robot_factory
          humanoid::core
          humanoid::telemetry_service
          humanoid::logging)

humanoid_core_configure_target(humanoid_core_smoke_test)

add_test(NAME humanoid_core_smoke_test COMMAND humanoid_core_smoke_test)

add_executable(humanoid_core_robot_state_unit_test robot_state_unit_test.cpp)

target_link_libraries(
  humanoid_core_robot_state_unit_test
  PRIVATE humanoid::core
          humanoid::telemetry_service)

humanoid_core_configure_target(humanoid_core_robot_state_unit_test)

add_test(NAME humanoid_core_robot_state_unit_test COMMAND humanoid_core_robot_state_unit_test)

add_executable(humanoid_core_command_model_unit_test command_model_unit_test.cpp)

target_link_libraries(humanoid_core_command_model_unit_test PRIVATE humanoid::core)

humanoid_core_configure_target(humanoid_core_command_model_unit_test)

add_test(NAME humanoid_core_command_model_unit_test COMMAND humanoid_core_command_model_unit_test)

add_executable(humanoid_core_execution_context_unit_test execution_context_unit_test.cpp)

target_link_libraries(humanoid_core_execution_context_unit_test PRIVATE humanoid::core)

humanoid_core_configure_target(humanoid_core_execution_context_unit_test)

add_test(NAME humanoid_core_execution_context_unit_test
         COMMAND humanoid_core_execution_context_unit_test)

add_executable(humanoid_core_mission_model_unit_test mission_model_unit_test.cpp)

target_link_libraries(humanoid_core_mission_model_unit_test PRIVATE humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_mission_model_unit_test)

add_test(NAME humanoid_core_mission_model_unit_test COMMAND humanoid_core_mission_model_unit_test)

add_executable(humanoid_core_mission_executor_unit_test mission_executor_unit_test.cpp)

target_link_libraries(
  humanoid_core_mission_executor_unit_test
  PRIVATE humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_mission_executor_unit_test)

add_test(NAME humanoid_core_mission_executor_unit_test
         COMMAND humanoid_core_mission_executor_unit_test)

add_executable(humanoid_core_mission_loader_unit_test mission_loader_unit_test.cpp)

target_link_libraries(
  humanoid_core_mission_loader_unit_test
  PRIVATE humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_mission_loader_unit_test)

add_test(NAME humanoid_core_mission_loader_unit_test COMMAND humanoid_core_mission_loader_unit_test)

add_executable(humanoid_core_mission_condition_unit_test mission_condition_unit_test.cpp)

target_link_libraries(
  humanoid_core_mission_condition_unit_test
  PRIVATE humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_mission_condition_unit_test)

add_test(NAME humanoid_core_mission_condition_unit_test
         COMMAND humanoid_core_mission_condition_unit_test)

add_executable(humanoid_core_command_dispatcher_unit_test command_dispatcher_unit_test.cpp)

target_link_libraries(
  humanoid_core_command_dispatcher_unit_test
  PRIVATE humanoid::core
          humanoid::adapter_interfaces)

humanoid_core_configure_target(humanoid_core_command_dispatcher_unit_test)

add_test(NAME humanoid_core_command_dispatcher_unit_test
         COMMAND humanoid_core_command_dispatcher_unit_test)

add_executable(humanoid_core_command_execution_pipeline_unit_test
               command_execution_pipeline_unit_test.cpp)

target_link_libraries(humanoid_core_command_execution_pipeline_unit_test PRIVATE humanoid::core)

humanoid_core_configure_target(humanoid_core_command_execution_pipeline_unit_test)

add_test(NAME humanoid_core_command_execution_pipeline_unit_test
         COMMAND humanoid_core_command_execution_pipeline_unit_test)

add_executable(humanoid_core_command_queue_unit_test command_queue_unit_test.cpp)

target_link_libraries(humanoid_core_command_queue_unit_test PRIVATE humanoid::core)

humanoid_core_configure_target(humanoid_core_command_queue_unit_test)

add_test(NAME humanoid_core_command_queue_unit_test
         COMMAND humanoid_core_command_queue_unit_test)

add_executable(humanoid_core_safety_validator_unit_test safety_validator_unit_test.cpp)

target_link_libraries(
  humanoid_core_safety_validator_unit_test
  PRIVATE humanoid::core
          humanoid::adapter_interfaces)

humanoid_core_configure_target(humanoid_core_safety_validator_unit_test)

add_test(NAME humanoid_core_safety_validator_unit_test
         COMMAND humanoid_core_safety_validator_unit_test)

add_executable(humanoid_core_plugin_registry_unit_test plugin_registry_unit_test.cpp)

target_link_libraries(
  humanoid_core_plugin_registry_unit_test
  PRIVATE humanoid::plugins)

humanoid_core_configure_target(humanoid_core_plugin_registry_unit_test)

add_test(NAME humanoid_core_plugin_registry_unit_test
         COMMAND humanoid_core_plugin_registry_unit_test)

add_executable(humanoid_core_plugin_factory_unit_test plugin_factory_unit_test.cpp)

target_link_libraries(
  humanoid_core_plugin_factory_unit_test
  PRIVATE humanoid::plugins)

humanoid_core_configure_target(humanoid_core_plugin_factory_unit_test)

add_test(NAME humanoid_core_plugin_factory_unit_test
         COMMAND humanoid_core_plugin_factory_unit_test)

add_executable(humanoid_core_unitree_g1_plugin_skeleton_test
               unitree_g1_plugin_skeleton_test.cpp)

target_link_libraries(
  humanoid_core_unitree_g1_plugin_skeleton_test
  PRIVATE humanoid::unitree_g1_plugin)

humanoid_core_configure_target(humanoid_core_unitree_g1_plugin_skeleton_test)

add_test(NAME humanoid_core_unitree_g1_plugin_skeleton_test
         COMMAND humanoid_core_unitree_g1_plugin_skeleton_test)

if(TARGET humanoid::unitree_sdk_abstraction)
  add_executable(humanoid_core_unitree_sdk_converter_test
                 unitree_sdk_converter_test.cpp)

  target_link_libraries(
    humanoid_core_unitree_sdk_converter_test
    PRIVATE humanoid::unitree_sdk_abstraction)

  humanoid_core_configure_target(humanoid_core_unitree_sdk_converter_test)

  add_test(NAME humanoid_core_unitree_sdk_converter_test
           COMMAND humanoid_core_unitree_sdk_converter_test)

  add_executable(humanoid_core_unitree_sdk_adapter_test
                 unitree_sdk_adapter_test.cpp)

  target_link_libraries(
    humanoid_core_unitree_sdk_adapter_test
    PRIVATE humanoid::unitree_sdk_abstraction)

  humanoid_core_configure_target(humanoid_core_unitree_sdk_adapter_test)

  add_test(NAME humanoid_core_unitree_sdk_adapter_test
           COMMAND humanoid_core_unitree_sdk_adapter_test)
endif()

add_executable(humanoid_core_robot_adapter_interface_unit_test
               robot_adapter_interface_unit_test.cpp)

target_link_libraries(
  humanoid_core_robot_adapter_interface_unit_test
  PRIVATE humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_robot_adapter_interface_unit_test)

add_test(NAME humanoid_core_robot_adapter_interface_unit_test
         COMMAND humanoid_core_robot_adapter_interface_unit_test)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
  if(HUMANOID_CORE_REQUIRE_GTEST)
    message(FATAL_ERROR "GoogleTest was requested but was not found")
  endif()

  message(STATUS "GoogleTest not found; optional GoogleTest unit tests are not built")
  return()
endif()

add_executable(
  humanoid_core_sample_unit_test
  sample_unit_test.cpp
  mock_robot_adapter_test.cpp)

target_link_libraries(
  humanoid_core_sample_unit_test
  PRIVATE humanoid::humanoid_core
          humanoid::adapter_interfaces
          humanoid::robot_factory
          GTest::gtest_main)

humanoid_core_configure_target(humanoid_core_sample_unit_test)

include(GoogleTest)
gtest_discover_tests(humanoid_core_sample_unit_test)
