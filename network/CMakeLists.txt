add_library(
  humanoid_core_network
  src/Endpoint.cpp
  src/TransportProtocol.cpp)

add_library(humanoid::network ALIAS humanoid_core_network)
set_target_properties(humanoid_core_network PROPERTIES EXPORT_NAME network)

target_include_directories(
  humanoid_core_network
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_network PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_network)

install(TARGETS humanoid_core_network EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/network)
