add_library(
  humanoid_core_core
  src/CoreContext.cpp
  src/RuntimeMetadata.cpp
  ${PROJECT_SOURCE_DIR}/src/core/CommandDispatcher.cpp
  ${PROJECT_SOURCE_DIR}/src/core/CommandQueue.cpp
  ${PROJECT_SOURCE_DIR}/src/core/RobotStateManager.cpp
  ${PROJECT_SOURCE_DIR}/src/core/SafetyValidator.cpp)

add_library(humanoid::core ALIAS humanoid_core_core)
set_target_properties(humanoid_core_core PROPERTIES EXPORT_NAME core)

target_include_directories(
  humanoid_core_core
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_core
  PUBLIC humanoid::common
         humanoid::configuration
         humanoid::logging
  PRIVATE humanoid::adapter_interfaces)

humanoid_core_configure_target(humanoid_core_core)

install(TARGETS humanoid_core_core EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/core)
