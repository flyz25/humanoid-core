add_library(
  humanoid_core_unitree_g1_plugin
  src/UnitreeG1Adapter.cpp
  src/UnitreeG1Plugin.cpp)

add_library(humanoid::unitree_g1_plugin ALIAS humanoid_core_unitree_g1_plugin)
set_target_properties(humanoid_core_unitree_g1_plugin PROPERTIES EXPORT_NAME unitree_g1_plugin)

target_include_directories(
  humanoid_core_unitree_g1_plugin
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/plugins/unitree/sdk>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/humanoid/plugins/unitree/sdk>)

target_link_libraries(
  humanoid_core_unitree_g1_plugin
  PUBLIC humanoid::plugins
         humanoid::humanoid_core)

if(TARGET humanoid::unitree_sdk_abstraction)
  target_link_libraries(humanoid_core_unitree_g1_plugin PRIVATE humanoid::unitree_sdk_abstraction)
  target_compile_definitions(humanoid_core_unitree_g1_plugin
                             PRIVATE HUMANOID_CORE_HAS_UNITREE_SDK=1)
else()
  target_compile_definitions(humanoid_core_unitree_g1_plugin
                             PRIVATE HUMANOID_CORE_HAS_UNITREE_SDK=0)
endif()

humanoid_core_configure_target(humanoid_core_unitree_g1_plugin)

install(
  TARGETS humanoid_core_unitree_g1_plugin
  EXPORT humanoid_coreTargets)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  FILES plugin_manifest.json
  DESTINATION ${CMAKE_INSTALL_DATADIR}/humanoid_core/plugins/unitree/g1)
