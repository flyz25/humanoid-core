#include <humanoid/bt/TreeValidator.h>

#include <string>
#include <utility>

#include <humanoid/bt/BehaviorTreeFactory.h>

namespace humanoid::bt {
namespace {

[[nodiscard]] TreeValidationResult Result(bool success, std::string message) {
  TreeValidationResult result;
  result.success = success;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] TreeValidationResult
ValidateNode(const TreeNodeDefinition& node, const BehaviorTreeFactory& factory, std::string path) {
  if (node.type.empty()) {
    return Result(false, "Missing behavior tree node type at " + path);
  }
  if (!factory.Contains(node.type)) {
    return Result(false, "Unknown behavior tree node type at " + path + ": " + node.type);
  }

  for (std::size_t index = 0U; index < node.children.size(); ++index) {
    TreeValidationResult child_result = ValidateNode(
        node.children[index], factory, path + ".children[" + std::to_string(index) + "]");
    if (!child_result.Succeeded()) {
      return child_result;
    }
  }

  return Result(true, "Behavior tree node is valid");
}

} // namespace

TreeValidationResult TreeValidator::Validate(const TreeDocument& document,
                                             const BehaviorTreeFactory& factory) const {
  return ValidateNode(document.root, factory, "root");
}

} // namespace humanoid::bt
