add_library(
  humanoid_core_unitree_g1_plugin
  src/UnitreeG1Adapter.cpp
  src/UnitreeG1Plugin.cpp)

add_library(humanoid::unitree_g1_plugin ALIAS humanoid_core_unitree_g1_plugin)
set_target_properties(humanoid_core_unitree_g1_plugin PROPERTIES EXPORT_NAME unitree_g1_plugin)

target_include_directories(
  humanoid_core_unitree_g1_plugin
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_unitree_g1_plugin
  PUBLIC humanoid::plugins
         humanoid::humanoid_core)

humanoid_core_configure_target(humanoid_core_unitree_g1_plugin)

install(
  TARGETS humanoid_core_unitree_g1_plugin
  EXPORT humanoid_coreTargets)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  FILES plugin_manifest.json
  DESTINATION ${CMAKE_INSTALL_DATADIR}/humanoid_core/plugins/unitree/g1)
