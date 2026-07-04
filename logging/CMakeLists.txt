add_library(
  humanoid_core_logging
  src/LogLevel.cpp
  src/LogMessage.cpp
  src/LoggerManager.cpp)

add_library(humanoid::logging ALIAS humanoid_core_logging)
set_target_properties(humanoid_core_logging PROPERTIES EXPORT_NAME logging)

target_include_directories(
  humanoid_core_logging
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_logging PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_logging)

install(TARGETS humanoid_core_logging EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/logging)
