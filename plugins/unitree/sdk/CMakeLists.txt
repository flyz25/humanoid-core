if(NOT ENABLE_UNITREE)
  return()
endif()

find_package(UnitreeSDK2 QUIET MODULE)

if(NOT TARGET unitree_sdk2)
  message(STATUS "Unitree SDK abstraction target is disabled; Unitree SDK2 was not found")
  return()
endif()

add_library(
  humanoid_core_unitree_sdk_abstraction
  AudioAdapter.cpp
  HandAdapter.cpp
  LocoAdapter.cpp
  SdkConverter.cpp
  SdkWrapper.cpp)

add_library(humanoid::unitree_sdk_abstraction ALIAS humanoid_core_unitree_sdk_abstraction)
set_target_properties(humanoid_core_unitree_sdk_abstraction
                      PROPERTIES EXPORT_NAME unitree_sdk_abstraction)

target_include_directories(
  humanoid_core_unitree_sdk_abstraction
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/core/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/humanoid/plugins/unitree/sdk>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_unitree_sdk_abstraction
  PRIVATE unitree_sdk2)

set_target_properties(
  humanoid_core_unitree_sdk_abstraction
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)

humanoid_core_configure_warnings(humanoid_core_unitree_sdk_abstraction)

install(
  TARGETS humanoid_core_unitree_sdk_abstraction
  EXPORT humanoid_coreTargets)

install(
  FILES AudioAdapter.h
        HandAdapter.h
        LocoAdapter.h
        SdkConverter.h
        SdkTypes.h
        SdkWrapper.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/humanoid/plugins/unitree/sdk)

install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR}/plugins/unitree/sdk)
