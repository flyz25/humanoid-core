/**
 * @file bt_loader_unit_test.cpp
 * @brief Validates behavior tree loading from JSON and YAML documents.
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <humanoid/bt/ActionNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/BehaviorTreeFactory.h>
#include <humanoid/bt/ConditionNode.h>
#include <humanoid/bt/InverterNode.h>
#include <humanoid/bt/SequenceNode.h>
#include <humanoid/bt/TreeLoader.h>

namespace {

using humanoid::bt::ActionNode;
using humanoid::bt::BehaviorTreeFactory;
using humanoid::bt::BTContext;
using humanoid::bt::BTStatus;
using humanoid::bt::ConditionNode;
using humanoid::bt::InverterNode;
using humanoid::bt::SequenceNode;
using humanoid::bt::TreeLoader;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

[[nodiscard]] std::shared_ptr<BehaviorTreeFactory> MakeFactory() {
  auto factory = std::make_shared<BehaviorTreeFactory>();
  Check(factory->RegisterNode("Sequence", []() { return std::make_unique<SequenceNode>(); }),
        "failed to register Sequence");
  Check(factory->RegisterNode("Inverter", []() { return std::make_unique<InverterNode>(); }),
        "failed to register Inverter");
  Check(factory->RegisterNode("SuccessAction",
                              []() {
                                return std::make_unique<ActionNode>(
                                    "SuccessAction", [](BTContext&) { return BTStatus::Success; });
                              }),
        "failed to register SuccessAction");
  Check(factory->RegisterNode("TrueCondition",
                              []() {
                                return std::make_unique<ConditionNode>(
                                    "TrueCondition", [](const BTContext&) { return true; });
                              }),
        "failed to register TrueCondition");
  Check(factory->RegisterNode("FalseCondition",
                              []() {
                                return std::make_unique<ConditionNode>(
                                    "FalseCondition", [](const BTContext&) { return false; });
                              }),
        "failed to register FalseCondition");
  return factory;
}

void TestLoadJsonTree() {
  const std::shared_ptr<BehaviorTreeFactory> factory = MakeFactory();
  const TreeLoader loader{factory};
  constexpr std::string_view document = R"json(
{
  "root": {
    "type": "Sequence",
    "children": [
      { "type": "SuccessAction" },
      { "type": "TrueCondition" }
    ]
  }
}
)json";

  auto result = loader.LoadJson(document);
  Check(result.Succeeded(), "JSON tree did not load");
  Check(result.tree != nullptr, "JSON tree result did not contain a tree");
  Check(result.tree->Initialize() == BTStatus::Idle, "JSON tree initialization failed");
  Check(result.tree->Tick() == BTStatus::Success, "JSON tree did not tick successfully");
}

void TestLoadYamlTree() {
  const std::shared_ptr<BehaviorTreeFactory> factory = MakeFactory();
  const TreeLoader loader{factory};
  constexpr std::string_view document = R"yaml(
root:
  type: Inverter
  child:
    type: FalseCondition
)yaml";

  auto result = loader.LoadYaml(document);
  Check(result.Succeeded(), "YAML tree did not load");
  Check(result.tree != nullptr, "YAML tree result did not contain a tree");
  Check(result.tree->Initialize() == BTStatus::Idle, "YAML tree initialization failed");
  Check(result.tree->Tick() == BTStatus::Success, "YAML tree did not tick successfully");
}

void TestInvalidTreeDocuments() {
  const std::shared_ptr<BehaviorTreeFactory> factory = MakeFactory();
  const TreeLoader loader{factory};

  Check(!loader.LoadJson("{").Succeeded(), "invalid JSON tree was accepted");
  Check(!loader.LoadJson(R"json({ "root": { "children": [] } })json").Succeeded(),
        "missing node type was accepted");
  Check(!loader.LoadJson(R"json({ "root": { "type": "UnknownNode" } })json").Succeeded(),
        "unknown node type was accepted");
  Check(!loader
             .LoadJson(R"json({
    "root": {
      "type": "SuccessAction",
      "child": { "type": "TrueCondition" }
    }
  })json")
             .Succeeded(),
        "leaf node with a child was accepted");
  Check(!loader.LoadYaml("root:\n  type: UnknownNode\n").Succeeded(),
        "unknown YAML node type was accepted");
}

void TestLoadTreeFromFile() {
  const std::shared_ptr<BehaviorTreeFactory> factory = MakeFactory();
  const TreeLoader loader{factory};
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("humanoid_core_bt_loader_unit_test_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".yaml");

  {
    std::ofstream file{path, std::ios::out | std::ios::trunc};
    Check(file.is_open(), "failed to open YAML tree test file");
    file << "root:\n"
            "  type: Sequence\n"
            "  children:\n"
            "    - type: SuccessAction\n"
            "    - type: TrueCondition\n";
    file.flush();
    Check(file.good(), "failed to write YAML tree test file");
  }

  const std::string expected_content = "root:\n"
                                       "  type: Sequence\n"
                                       "  children:\n"
                                       "    - type: SuccessAction\n"
                                       "    - type: TrueCondition\n";
  {
    std::ifstream file{path};
    const std::string content{std::istreambuf_iterator<char>{file},
                              std::istreambuf_iterator<char>{}};
    Check(content == expected_content, "YAML tree test file content mismatch");
    auto direct_result = loader.LoadYaml(content);
    if (!direct_result.Succeeded()) {
      throw std::runtime_error{"direct YAML tree content did not load: " + direct_result.message};
    }
  }

  auto result = loader.LoadFile(path);
  std::filesystem::remove(path);

  if (!result.Succeeded()) {
    throw std::runtime_error{"YAML tree file did not load: " + result.message};
  }
  Check(result.tree != nullptr, "YAML tree file result did not contain a tree");
  Check(result.tree->Initialize() == BTStatus::Idle, "YAML tree file initialization failed");
  Check(result.tree->Tick() == BTStatus::Success, "YAML tree file did not tick successfully");
}

} // namespace

int main() {
  try {
    TestLoadJsonTree();
    TestLoadYamlTree();
    TestInvalidTreeDocuments();
    TestLoadTreeFromFile();
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
