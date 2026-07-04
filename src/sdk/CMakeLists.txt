if(HUMANOID_CORE_UNITREE_ENABLED AND TARGET humanoid::unitree_sdk_abstraction)
  add_library(humanoid_core_unitree_loco_client LocoClientWrapper.cpp)
  add_library(humanoid::unitree_loco_client ALIAS humanoid_core_unitree_loco_client)
  set_target_properties(humanoid_core_unitree_loco_client PROPERTIES EXPORT_NAME unitree_loco_client)

  target_include_directories(
    humanoid_core_unitree_loco_client
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
           $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_link_libraries(
    humanoid_core_unitree_loco_client
    INTERFACE humanoid::adapter_interfaces
    PRIVATE humanoid::unitree_sdk_abstraction)

  set_target_properties(
    humanoid_core_unitree_loco_client
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF)

  humanoid_core_configure_warnings(humanoid_core_unitree_loco_client)

  install(
    TARGETS humanoid_core_unitree_loco_client
    EXPORT humanoid_coreTargets)

  install(
    FILES LocoClientWrapper.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sdk)

  install(
    FILES LocoClientWrapper.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/humanoid/sdk)
endif()
