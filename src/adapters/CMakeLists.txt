add_library(humanoid_core_adapter_interfaces INTERFACE)
add_library(humanoid::adapter_interfaces ALIAS humanoid_core_adapter_interfaces)
set_target_properties(humanoid_core_adapter_interfaces PROPERTIES EXPORT_NAME adapter_interfaces)

target_include_directories(
  humanoid_core_adapter_interfaces
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_adapter_interfaces INTERFACE humanoid::compiler_options)

install(
  TARGETS humanoid_core_adapter_interfaces
  EXPORT humanoid_coreTargets)

install(
  FILES ${PROJECT_SOURCE_DIR}/include/humanoid/adapters/Result.h
        ${PROJECT_SOURCE_DIR}/include/humanoid/adapters/IRobotAdapter.h
        ${PROJECT_SOURCE_DIR}/include/humanoid/adapters/IRobotFactory.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/humanoid/adapters)
