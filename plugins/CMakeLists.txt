add_library(
  humanoid_core_plugins
  src/PluginFactory.cpp
  src/PluginLifecycleState.cpp
  src/PluginMetadata.cpp
  src/PluginRegistry.cpp)

add_library(humanoid::plugins ALIAS humanoid_core_plugins)
set_target_properties(humanoid_core_plugins PROPERTIES EXPORT_NAME plugins)

target_include_directories(
  humanoid_core_plugins
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_plugins PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_plugins)

install(TARGETS humanoid_core_plugins EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR}/plugins)
