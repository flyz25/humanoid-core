#include <humanoid/bt/TreeLoader.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include <humanoid/bt/CompositeNode.h>
#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {
namespace {

[[nodiscard]] TreeLoadResult Result(std::unique_ptr<BehaviorTree> tree, bool success,
                                    std::string message) {
  TreeLoadResult result;
  result.tree = std::move(tree);
  result.success = success;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] TreeLoadResult Failure(std::string message) {
  return Result({}, false, std::move(message));
}

[[nodiscard]] std::string LowercaseExtension(const std::filesystem::path& path) {
  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
  return extension;
}

[[nodiscard]] std::unique_ptr<BTNode> BuildNode(const TreeNodeDefinition& definition,
                                                const BehaviorTreeFactory& factory,
                                                std::string& error) {
  std::unique_ptr<BTNode> node = factory.CreateNode(definition.type);
  if (!node) {
    error = "Unable to create behavior tree node: " + definition.type;
    return {};
  }

  if (definition.children.empty()) {
    return node;
  }

  if (CompositeNode* composite = dynamic_cast<CompositeNode*>(node.get())) {
    for (const TreeNodeDefinition& child_definition : definition.children) {
      std::unique_ptr<BTNode> child = BuildNode(child_definition, factory, error);
      if (!child) {
        return {};
      }
      if (!composite->AddChild(std::move(child))) {
        error = "Unable to add child to composite node: " + definition.type;
        return {};
      }
    }
    return node;
  }

  if (DecoratorNode* decorator = dynamic_cast<DecoratorNode*>(node.get())) {
    if (definition.children.size() != 1U) {
      error = "Decorator node requires exactly one child: " + definition.type;
      return {};
    }
    std::unique_ptr<BTNode> child = BuildNode(definition.children.front(), factory, error);
    if (!child) {
      return {};
    }
    if (!decorator->SetChild(std::move(child))) {
      error = "Unable to set child on decorator node: " + definition.type;
      return {};
    }
    return node;
  }

  error = "Behavior tree leaf node cannot have children: " + definition.type;
  return {};
}

} // namespace

TreeLoader::TreeLoader(std::shared_ptr<const BehaviorTreeFactory> factory)
    : factory_(std::move(factory)) {}

TreeLoader::TreeLoader(std::shared_ptr<const BehaviorTreeFactory> factory, TreeParser parser,
                       TreeValidator validator)
    : factory_(std::move(factory)), parser_(std::move(parser)), validator_(std::move(validator)) {}

TreeLoadResult TreeLoader::LoadFile(const std::filesystem::path& path, BTContext context) const {
  try {
    std::ifstream file{path, std::ios::in | std::ios::binary};
    if (!file.is_open()) {
      return Failure("Unable to open behavior tree file: " + path.string());
    }

    const std::string content{std::istreambuf_iterator<char>{file},
                              std::istreambuf_iterator<char>{}};
    if (file.bad()) {
      return Failure("Unable to read behavior tree file: " + path.string());
    }

    const std::string extension = LowercaseExtension(path);
    if (extension == ".json") {
      return LoadJson(content, std::move(context));
    }
    if (extension == ".yaml" || extension == ".yml") {
      return LoadYaml(content, std::move(context));
    }
    if (extension == ".xml") {
      return Failure("XML behavior tree loading is not enabled");
    }

    return Failure("Unsupported behavior tree file extension: " + extension);
  } catch (const std::exception& exception) {
    return Failure(std::string{"Behavior tree file load failed: "} + exception.what());
  } catch (...) {
    return Failure("Behavior tree file load failed with an unknown error");
  }
}

TreeLoadResult TreeLoader::LoadJson(std::string_view document, BTContext context) const {
  return Build(parser_.ParseJson(document), std::move(context));
}

TreeLoadResult TreeLoader::LoadYaml(std::string_view document, BTContext context) const {
  return Build(parser_.ParseYaml(document), std::move(context));
}

TreeLoadResult TreeLoader::Build(TreeParseResult parse_result, BTContext context) const {
  if (!parse_result.Succeeded()) {
    return Failure(parse_result.message);
  }
  if (!factory_) {
    return Failure("Behavior tree factory dependency is unavailable");
  }

  const TreeValidationResult validation = validator_.Validate(parse_result.document, *factory_);
  if (!validation.Succeeded()) {
    return Failure(validation.message);
  }

  std::string error;
  std::unique_ptr<BTNode> root = BuildNode(parse_result.document.root, *factory_, error);
  if (!root) {
    return Failure(error.empty() ? "Unable to construct behavior tree" : error);
  }
  return Result(std::make_unique<BehaviorTree>(std::move(root), std::move(context)), true,
                "Behavior tree loaded successfully");
}

} // namespace humanoid::bt
