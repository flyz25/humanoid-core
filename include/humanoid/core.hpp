#pragma once

/**
 * @file core.hpp
 * @brief Public umbrella header for humanoid-core.
 */

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/adapters/IRobotFactory.h>
#include <humanoid/adapters/Result.h>
#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/common/Version.hpp>
#include <humanoid/configuration/ConfigManager.hpp>
#include <humanoid/configuration/ConfigValue.hpp>
#include <humanoid/configuration/Configuration.hpp>
#include <humanoid/core/Command.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandExecutionPipeline.h>
#include <humanoid/core/CommandPriority.h>
#include <humanoid/core/CommandQueue.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/CommandStatus.h>
#include <humanoid/core/CommandType.h>
#include <humanoid/core/CoreContext.hpp>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/core/RobotState.hpp>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/core/RuntimeMetadata.hpp>
#include <humanoid/core/SafetyValidator.h>
#include <humanoid/diagnostics/DiagnosticController.hpp>
#include <humanoid/diagnostics/DiagnosticManager.hpp>
#include <humanoid/diagnostics/DiagnosticRecord.hpp>
#include <humanoid/diagnostics/DiagnosticStatus.hpp>
#include <humanoid/gesture/GestureController.hpp>
#include <humanoid/gesture/GestureManager.hpp>
#include <humanoid/logging/LogLevel.hpp>
#include <humanoid/logging/LogMessage.hpp>
#include <humanoid/logging/LogSink.hpp>
#include <humanoid/logging/Logger.hpp>
#include <humanoid/logging/LoggerManager.hpp>
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/MissionResult.h>
#include <humanoid/mission/MissionStatus.h>
#include <humanoid/mission/MissionStep.h>
#include <humanoid/motion/MotionController.hpp>
#include <humanoid/motion/MotionManager.hpp>
#include <humanoid/motion/MotionMode.hpp>
#include <humanoid/network/Endpoint.hpp>
#include <humanoid/network/NetworkManager.hpp>
#include <humanoid/network/TransportProtocol.hpp>
#include <humanoid/robot/Robot.hpp>
#include <humanoid/robot/RobotAdapter.hpp>
#include <humanoid/robot/RobotManager.hpp>
#include <humanoid/safety/SafetyController.hpp>
#include <humanoid/safety/SafetyManager.hpp>
#include <humanoid/safety/SafetyState.hpp>
#include <humanoid/services/TelemetryService.h>
#include <humanoid/utilities/Filesystem.hpp>
#include <humanoid/utilities/ScopeExit.hpp>
#include <humanoid/utilities/Time.hpp>
