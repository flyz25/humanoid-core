#include <adapters/unitree/UnitreeRobotFactory.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <adapters/unitree/UnitreeG1Adapter.h>

namespace humanoid::adapters::unitree {

std::string_view UnitreeRobotFactory::Vendor() const noexcept { return "Unitree"; }

std::vector<std::string> UnitreeRobotFactory::SupportedModels() const { return {"G1"}; }

bool UnitreeRobotFactory::Supports(std::string_view vendor, std::string_view model) const {
  return vendor == Vendor() && model == "G1";
}

std::unique_ptr<IRobotAdapter>
UnitreeRobotFactory::CreateAdapter(const RobotConfig& config,
                                   std::shared_ptr<logging::ILogger> logger) const {
  if (!Supports(config.vendor, config.model)) {
    return nullptr;
  }

  return std::make_unique<UnitreeG1Adapter>(config, std::move(logger));
}

} // namespace humanoid::adapters::unitree
