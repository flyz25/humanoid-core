add_library(
  humanoid_core_utilities
  src/Filesystem.cpp
  src/Time.cpp)

add_library(humanoid::utilities ALIAS humanoid_core_utilities)
set_target_properties(humanoid_core_utilities PROPERTIES EXPORT_NAME utilities)

target_include_directories(
  humanoid_core_utilities
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_utilities PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_utilities)

install(TARGETS humanoid_core_utilities EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/utilities)
