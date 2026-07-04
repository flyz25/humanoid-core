function(humanoid_core_add_example target source)
  add_executable(${target} ${source})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${target} PRIVATE ${ARGN})
  humanoid_core_configure_target(${target})
  install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

humanoid_core_add_example(
  humanoid_core_basic_initialization
  basic_initialization/main.cpp
  humanoid::humanoid_core)

humanoid_core_add_example(
  humanoid_core_basic_robot_connection
  basic_robot_connection/main.cpp
  humanoid::adapter_interfaces
  humanoid::core
  humanoid::robot_factory
  humanoid::logging)

if(TARGET humanoid::unitree_adapter)
  target_link_libraries(humanoid_core_basic_robot_connection PRIVATE humanoid::unitree_adapter)
  target_compile_definitions(humanoid_core_basic_robot_connection PRIVATE HUMANOID_CORE_HAS_UNITREE=1)
else()
  target_compile_definitions(
    humanoid_core_basic_robot_connection
    PRIVATE HUMANOID_CORE_HAS_UNITREE=0)
endif()

humanoid_core_add_example(
  humanoid_core_plugin_loading_example
  plugin_loading/main.cpp
  humanoid::unitree_g1_plugin)

humanoid_core_add_example(
  humanoid_core_framework_integration_example
  framework_integration/main.cpp
  humanoid::unitree_g1_plugin
  humanoid::telemetry_service)

humanoid_core_add_example(
  humanoid_core_capability_query_example
  capability_query/main.cpp
  humanoid::unitree_g1_plugin)

humanoid_core_add_example(
  humanoid_core_robot_connection_example
  robot_connection/main.cpp
  humanoid::adapter_interfaces
  humanoid::core
  humanoid::robot_factory
  humanoid::logging)

if(TARGET humanoid::unitree_adapter)
  target_link_libraries(humanoid_core_robot_connection_example PRIVATE humanoid::unitree_adapter)
  target_compile_definitions(
    humanoid_core_robot_connection_example
    PRIVATE HUMANOID_CORE_HAS_UNITREE=1)
else()
  target_compile_definitions(
    humanoid_core_robot_connection_example
    PRIVATE HUMANOID_CORE_HAS_UNITREE=0)
endif()

humanoid_core_add_example(
  humanoid_core_telemetry_example
  telemetry/main.cpp
  humanoid::core
  humanoid::telemetry_service)

humanoid_core_add_example(
  humanoid_core_command_execution_example
  command_execution/main.cpp
  humanoid::core)

humanoid_core_add_example(
  humanoid_core_command_queue_example
  command_queue/main.cpp
  humanoid::core)

humanoid_core_add_example(
  humanoid_core_command_cancellation_example
  command_cancellation/main.cpp
  humanoid::core)

humanoid_core_add_example(
  humanoid_core_capability_validation_example
  capability_validation/main.cpp
  humanoid::core)

if(TARGET humanoid::unitree_sdk_abstraction)
  humanoid_core_add_example(
    humanoid_core_unitree_sdk_boundary_example
    unitree_sdk_boundary/main.cpp
    humanoid::unitree_sdk_abstraction)
endif()
