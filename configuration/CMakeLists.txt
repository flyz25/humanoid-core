add_library(humanoid_core_configuration src/ConfigManager.cpp)

add_library(humanoid::configuration ALIAS humanoid_core_configuration)
set_target_properties(humanoid_core_configuration PROPERTIES EXPORT_NAME configuration)

target_include_directories(
  humanoid_core_configuration
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_configuration PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_configuration)

install(TARGETS humanoid_core_configuration EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/configuration)
