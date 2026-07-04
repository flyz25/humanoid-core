#include <humanoid/core/RobotAdapter.h>

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

class TestRobotAdapter final : public humanoid::core::RobotAdapter {
public:
  [[nodiscard]] humanoid::common::Status Initialize() override {
    initialized_ = true;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Shutdown() override {
    initialized_ = false;
    connected_ = false;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Connect() override {
    if (!initialized_) {
      return humanoid::common::Status::error(
          humanoid::common::StatusCode::kFailedPrecondition,
          "adapter is not initialized");
    }

    connected_ = true;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Disconnect() override {
    connected_ = false;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] bool IsConnected() const noexcept override { return connected_; }

  [[nodiscard]] humanoid::core::RobotState GetRobotState() const override {
    humanoid::core::RobotState state;
    state.connection.connected = connected_;
    return state;
  }

  [[nodiscard]] humanoid::core::RobotInformation GetRobotInformation() const override {
    humanoid::core::RobotInformation information;
    information.vendor = "Test";
    information.model = "Interface";
    information.adapterName = "TestRobotAdapter";
    return information;
  }

  [[nodiscard]] humanoid::core::RobotCapabilities GetCapabilities() const override {
    humanoid::core::RobotCapabilities capabilities;
    capabilities.supportsLifecycle = true;
    capabilities.supportsConnectionManagement = true;
    capabilities.supportsStateFeedback = true;
    capabilities.supportsRobotInformation = true;
    capabilities.supportsPeriodicUpdate = true;
    return capabilities;
  }

  [[nodiscard]] humanoid::common::Status Update() override {
    ++update_count_;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] int updateCount() const noexcept { return update_count_; }

private:
  bool initialized_{false};
  bool connected_{false};
  int update_count_{0};
};

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] bool TestRobotAdapterIsAbstract() {
  constexpr std::string_view kTestName{"RobotAdapter abstract interface"};

  static_assert(std::has_virtual_destructor_v<humanoid::core::RobotAdapter>);
  static_assert(std::is_abstract_v<humanoid::core::RobotAdapter>);

  if constexpr (!std::is_abstract_v<humanoid::core::RobotAdapter>) {
    return Fail(kTestName, "RobotAdapter must remain abstract");
  }

  return true;
}

[[nodiscard]] bool TestRobotAdapterLifecycleContract() {
  constexpr std::string_view kTestName{"RobotAdapter lifecycle contract"};

  TestRobotAdapter adapter;
  if (adapter.IsConnected()) {
    return Fail(kTestName, "new adapter reported connected");
  }

  if (adapter.Connect().isOk()) {
    return Fail(kTestName, "connect succeeded before initialization");
  }

  if (!adapter.Initialize().isOk() || !adapter.Connect().isOk()) {
    return Fail(kTestName, "initialize/connect failed");
  }

  if (!adapter.IsConnected() || !adapter.GetRobotState().connection.connected) {
    return Fail(kTestName, "connected state was not reflected");
  }

  if (!adapter.Disconnect().isOk() || adapter.IsConnected()) {
    return Fail(kTestName, "disconnect failed");
  }

  if (!adapter.Shutdown().isOk()) {
    return Fail(kTestName, "shutdown failed");
  }

  return true;
}

[[nodiscard]] bool TestRobotInformationAndCapabilities() {
  constexpr std::string_view kTestName{"RobotAdapter information and capabilities"};

  TestRobotAdapter adapter;
  const humanoid::core::RobotInformation information = adapter.GetRobotInformation();
  if (information.vendor != "Test" || information.model != "Interface" ||
      information.adapterName != "TestRobotAdapter") {
    return Fail(kTestName, "robot information values were not preserved");
  }

  if (!information.adapterApiVersion.toString().starts_with("0.3.0")) {
    return Fail(kTestName, "adapter API version did not match framework version");
  }

  const humanoid::core::RobotCapabilities capabilities = adapter.GetCapabilities();
  if (!capabilities.supportsLifecycle ||
      !capabilities.supportsConnectionManagement ||
      !capabilities.supportsStateFeedback ||
      !capabilities.supportsRobotInformation ||
      !capabilities.supportsPeriodicUpdate) {
    return Fail(kTestName, "expected capabilities were not declared");
  }

  if (!adapter.Update().isOk() || adapter.updateCount() != 1) {
    return Fail(kTestName, "update did not execute");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestRobotAdapterIsAbstract,
      TestRobotAdapterLifecycleContract,
      TestRobotInformationAndCapabilities,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
