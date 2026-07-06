find_package(rclcpp QUIET)

if(ENABLE_ROS2 AND rclcpp_FOUND)
  set(HUMANOID_CORE_HAS_ROS2_RUNTIME ON)
  message(STATUS "ROS2 rclcpp found; ROS2 runtime integration metadata is enabled")
elseif(ENABLE_ROS2)
  set(HUMANOID_CORE_HAS_ROS2_RUNTIME OFF)
  message(STATUS "ROS2 rclcpp not found; building ROS2 bridge contracts without runtime endpoints")
else()
  set(HUMANOID_CORE_HAS_ROS2_RUNTIME OFF)
  message(STATUS "ENABLE_ROS2=OFF; ROS2 runtime endpoints are disabled")
endif()

add_library(humanoid_core_ros2_bridge bridge/ROS2Bridge.cpp)
add_library(humanoid::ros2_bridge ALIAS humanoid_core_ros2_bridge)
set_target_properties(humanoid_core_ros2_bridge PROPERTIES EXPORT_NAME ros2_bridge)

target_include_directories(
  humanoid_core_ros2_bridge
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/core/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(
  humanoid_core_ros2_bridge
  PUBLIC HUMANOID_CORE_HAS_ROS2_RUNTIME=$<BOOL:${HUMANOID_CORE_HAS_ROS2_RUNTIME}>)

target_link_libraries(humanoid_core_ros2_bridge PUBLIC humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_ros2_bridge)

install(TARGETS humanoid_core_ros2_bridge EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY messages/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2/messages)
install(DIRECTORY launch/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2/launch)
install(DIRECTORY rviz/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2/rviz)
install(DIRECTORY topics/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2/topics)
install(DIRECTORY services/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2/services)
install(DIRECTORY actions/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2/actions)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR}/ros2)

function(humanoid_core_add_ros2_example target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE humanoid::ros2_bridge)
  humanoid_core_configure_target(${target})
  install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

if(HUMANOID_CORE_BUILD_EXAMPLES)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_robot_bridge_example
    examples/robot_bridge/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_mission_execution_example
    examples/mission_execution/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_behavior_tree_example
    examples/behavior_tree/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_planner_example
    examples/planner/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_perception_example
    examples/perception/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_runtime_example
    examples/runtime/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_plugin_discovery_example
    examples/plugin_discovery/main.cpp)
  humanoid_core_add_ros2_example(
    humanoid_core_ros2_sdk_isolation_example
    examples/sdk_isolation/main.cpp)
endif()

if(BUILD_TESTING AND HUMANOID_CORE_BUILD_TESTS)
  add_executable(humanoid_core_ros2_bridge_unit_test tests/ros2_bridge_unit_test.cpp)
  target_link_libraries(humanoid_core_ros2_bridge_unit_test PRIVATE humanoid::ros2_bridge)
  humanoid_core_configure_target(humanoid_core_ros2_bridge_unit_test)
  add_test(NAME humanoid_core_ros2_bridge_unit_test COMMAND humanoid_core_ros2_bridge_unit_test)
endif()
