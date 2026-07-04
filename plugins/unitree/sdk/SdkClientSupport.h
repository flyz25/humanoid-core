#pragma once

/**
 * @file SdkClientSupport.h
 * @brief Defines internal helpers shared by Unitree SDK adapter implementations.
 */

#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include "SdkTypes.h"

#if defined(__linux__)
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace humanoid::plugins::unitree::sdk::internal {

/**
 * @brief Milliseconds in one second.
 */
constexpr float kMillisecondsPerSecond = 1000.0F;

#if defined(__linux__)
/**
 * @brief RAII file descriptor used by SDK runtime preflight checks.
 */
class FileDescriptor final {
public:
  /**
   * @brief Takes ownership of a POSIX file descriptor.
   *
   * @param descriptor Descriptor to close on destruction.
   */
  explicit FileDescriptor(int descriptor) noexcept : descriptor_(descriptor) {}

  /**
   * @brief Closes the owned descriptor when valid.
   */
  ~FileDescriptor() noexcept {
    if (descriptor_ >= 0) {
      static_cast<void>(::close(descriptor_));
    }
  }

  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;
  FileDescriptor(FileDescriptor&&) = delete;
  FileDescriptor& operator=(FileDescriptor&&) = delete;

  /**
   * @brief Reports whether the descriptor is valid.
   *
   * @return True when descriptor is non-negative.
   */
  [[nodiscard]] bool IsValid() const noexcept { return descriptor_ >= 0; }

private:
  int descriptor_{-1};
};
#endif

/**
 * @brief Creates a successful SDK result.
 *
 * @param message Diagnostic message.
 * @return Successful result.
 */
[[nodiscard]] inline SdkResult Success(std::string message) {
  return SdkResult{SdkErrorCode::kSuccess, std::move(message)};
}

/**
 * @brief Creates a failed SDK result.
 *
 * @param code Error category.
 * @param message Diagnostic message.
 * @return Failed result.
 */
[[nodiscard]] inline SdkResult Failure(SdkErrorCode code, std::string message) {
  return SdkResult{code, std::move(message)};
}

/**
 * @brief Converts a Unitree SDK2 integer return value into SdkResult.
 *
 * @param sdk_return Unitree SDK2 return code.
 * @param command Command name.
 * @return Normalized result.
 */
[[nodiscard]] inline SdkResult FromSdkReturn(std::int32_t sdk_return, const char* command) {
  if (sdk_return == 0) {
    return Success(std::string{command} + " succeeded");
  }

  std::ostringstream message;
  message << command << " failed with Unitree SDK2 return code " << sdk_return;
  return Failure(SdkErrorCode::kUnknown, message.str());
}

/**
 * @brief Invokes a Unitree SDK2 command and translates exceptions.
 *
 * @tparam Operation Callable returning a Unitree SDK2 integer return code.
 * @param command Command name.
 * @param exception_code Error category used when an exception is thrown.
 * @param operation Unitree SDK operation.
 * @return Normalized result.
 */
template <typename Operation>
[[nodiscard]] SdkResult InvokeSdkCommand(const char* command, SdkErrorCode exception_code,
                                         Operation operation) {
  try {
    return FromSdkReturn(static_cast<std::int32_t>(operation()), command);
  } catch (const std::exception& exception) {
    return Failure(exception_code, std::string{command} +
                                       " failed with Unitree SDK2 exception: " + exception.what());
  } catch (...) {
    return Failure(exception_code,
                   std::string{command} + " failed with an unknown Unitree SDK2 exception");
  }
}

/**
 * @brief Converts framework timeout to Unitree SDK2 seconds.
 *
 * @param timeout Timeout duration.
 * @return Timeout in seconds.
 */
[[nodiscard]] inline float TimeoutSeconds(std::chrono::milliseconds timeout) {
  return static_cast<float>(timeout.count()) / kMillisecondsPerSecond;
}

/**
 * @brief Reports whether a network interface exists.
 *
 * @param network_interface Network interface name.
 * @return True when the interface exists, or when not running on Linux.
 */
[[nodiscard]] inline bool NetworkInterfaceExists(const std::string& network_interface) {
#if defined(__linux__)
  std::error_code error;
  const std::filesystem::path interface_path =
      std::filesystem::path{"/sys/class/net"} / network_interface;
  return std::filesystem::exists(interface_path, error);
#else
  static_cast<void>(network_interface);
  return true;
#endif
}

/**
 * @brief Reports whether the process can open a route netlink socket.
 *
 * @return True when route netlink is available, or when not running on Linux.
 */
[[nodiscard]] inline bool CanOpenRouteNetlinkSocket() noexcept {
#if defined(__linux__)
  const FileDescriptor descriptor{::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE)};
  return descriptor.IsValid();
#else
  return true;
#endif
}

/**
 * @brief Validates SDK configuration before constructing Unitree SDK clients.
 *
 * @param configuration SDK configuration.
 * @return Validation result.
 */
[[nodiscard]] inline SdkResult ValidateSdkConfiguration(const SdkConfiguration& configuration) {
  if (configuration.network_interface.empty()) {
    return Failure(SdkErrorCode::kConnectionFailed, "network interface is empty");
  }

  if (configuration.timeout.count() <= 0) {
    return Failure(SdkErrorCode::kTimeout, "timeout must be greater than zero");
  }

  if (configuration.domain_id > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    return Failure(SdkErrorCode::kConnectionFailed, "DDS domain id exceeds supported range");
  }

  if (!NetworkInterfaceExists(configuration.network_interface)) {
    return Failure(SdkErrorCode::kConnectionFailed,
                   "network interface does not exist: " + configuration.network_interface);
  }

  if (!CanOpenRouteNetlinkSocket()) {
    return Failure(SdkErrorCode::kConnectionFailed,
                   "route netlink socket is unavailable; Unitree SDK2 transport cannot be "
                   "initialized in this environment");
  }

  return Success("Unitree SDK2 configuration is valid");
}

} // namespace humanoid::plugins::unitree::sdk::internal
