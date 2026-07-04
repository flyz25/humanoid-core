add_library(humanoid_core_gesture src/GestureManager.cpp)

add_library(humanoid::gesture ALIAS humanoid_core_gesture)
set_target_properties(humanoid_core_gesture PROPERTIES EXPORT_NAME gesture)

target_include_directories(
  humanoid_core_gesture
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(humanoid_core_gesture PUBLIC humanoid::common)

humanoid_core_configure_target(humanoid_core_gesture)

install(TARGETS humanoid_core_gesture EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/gesture)
