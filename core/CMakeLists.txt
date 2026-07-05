add_library(
  humanoid_core_core
  src/CoreContext.cpp
  src/RuntimeMetadata.cpp
  ${PROJECT_SOURCE_DIR}/src/core/CommandDispatcher.cpp
  ${PROJECT_SOURCE_DIR}/src/core/CommandExecutionPipeline.cpp
  ${PROJECT_SOURCE_DIR}/src/core/CommandQueue.cpp
  ${PROJECT_SOURCE_DIR}/src/core/RobotStateManager.cpp
  ${PROJECT_SOURCE_DIR}/src/core/SafetyValidator.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/ActionNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/BTContext.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/BehaviorTree.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/BehaviorTreeFactory.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/BehaviorTreeRuntime.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/CommandNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/CompositeNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/ConditionNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/DecoratorNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/DelayNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/FailerNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/InverterNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/LimiterNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/MissionNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/ParallelNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/RepeatNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/RetryNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/SelectorNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/SequenceNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/SucceederNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/TimeoutNode.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/TreeLoader.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/TreeParser.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/TreeValidator.cpp
  ${PROJECT_SOURCE_DIR}/src/bt/WaitNode.cpp
  ${PROJECT_SOURCE_DIR}/src/mission/ConditionEvaluator.cpp
  ${PROJECT_SOURCE_DIR}/src/mission/MissionExecutor.cpp
  ${PROJECT_SOURCE_DIR}/src/mission/MissionLoader.cpp
  ${PROJECT_SOURCE_DIR}/src/mission/MissionParser.cpp
  ${PROJECT_SOURCE_DIR}/src/mission/MissionValidator.cpp
  ${PROJECT_SOURCE_DIR}/src/planner/PlannerFactory.cpp
  ${PROJECT_SOURCE_DIR}/src/planner/PlannerRegistry.cpp
  ${PROJECT_SOURCE_DIR}/src/planner/PlanningPipeline.cpp
  ${PROJECT_SOURCE_DIR}/src/planner/RuleBasedPlanner.cpp
  ${PROJECT_SOURCE_DIR}/src/runtime/Blackboard.cpp
  ${PROJECT_SOURCE_DIR}/src/runtime/Cancellation.cpp
  ${PROJECT_SOURCE_DIR}/src/runtime/ExecutionContext.cpp
  ${PROJECT_SOURCE_DIR}/src/runtime/ResourceManager.cpp
  ${PROJECT_SOURCE_DIR}/src/runtime/RuntimeScheduler.cpp)

add_library(humanoid::core ALIAS humanoid_core_core)
set_target_properties(humanoid_core_core PROPERTIES EXPORT_NAME core)

target_include_directories(
  humanoid_core_core
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  humanoid_core_core
  PUBLIC humanoid::common
         humanoid::configuration
         humanoid::logging
  PRIVATE humanoid::adapter_interfaces)

humanoid_core_configure_target(humanoid_core_core)

install(TARGETS humanoid_core_core EXPORT humanoid_coreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/core)
