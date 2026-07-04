add_executable(humanoid_core_basic_initialization basic_initialization/main.cpp)

target_link_libraries(humanoid_core_basic_initialization PRIVATE humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_basic_initialization)

install(TARGETS humanoid_core_basic_initialization RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(humanoid_core_basic_robot_connection basic_robot_connection/main.cpp)

target_link_libraries(
  humanoid_core_basic_robot_connection
  PRIVATE humanoid::adapter_interfaces
          humanoid::robot_factory
          humanoid::logging)

if(TARGET humanoid::unitree_adapter)
  target_link_libraries(humanoid_core_basic_robot_connection PRIVATE humanoid::unitree_adapter)
  target_compile_definitions(humanoid_core_basic_robot_connection PRIVATE HUMANOID_CORE_HAS_UNITREE=1)
else()
  target_compile_definitions(humanoid_core_basic_robot_connection PRIVATE HUMANOID_CORE_HAS_UNITREE=0)
endif()

humanoid_core_configure_target(humanoid_core_basic_robot_connection)

install(TARGETS humanoid_core_basic_robot_connection RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
