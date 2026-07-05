#include <humanoid/bt/BehaviorTreeFactory.h>

#include <algorithm>
#include <mutex>
#include <utility>

namespace humanoid::bt {

bool BehaviorTreeFactory::RegisterNode(std::string node_type, BTNodeCreator creator) {
  if (node_type.empty() || !creator) {
    return false;
  }

  std::unique_lock<std::shared_mutex> lock{mutex_};
  return creators_.emplace(std::move(node_type), std::move(creator)).second;
}

bool BehaviorTreeFactory::UnregisterNode(std::string_view node_type) {
  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = creators_.find(node_type);
  if (iterator == creators_.end()) {
    return false;
  }
  creators_.erase(iterator);
  return true;
}

bool BehaviorTreeFactory::Contains(std::string_view node_type) const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return creators_.find(node_type) != creators_.end();
}

std::unique_ptr<BTNode> BehaviorTreeFactory::CreateNode(std::string_view node_type) const {
  BTNodeCreator creator;
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = creators_.find(node_type);
    if (iterator == creators_.end()) {
      return {};
    }
    creator = iterator->second;
  }

  try {
    return creator();
  } catch (...) {
    return {};
  }
}

std::unique_ptr<BehaviorTree> BehaviorTreeFactory::CreateTree(std::string_view root_node_type,
                                                              BTContext context) const {
  std::unique_ptr<BTNode> root = CreateNode(root_node_type);
  if (!root) {
    return {};
  }
  return std::make_unique<BehaviorTree>(std::move(root), std::move(context));
}

std::vector<std::string> BehaviorTreeFactory::RegisteredNodeTypes() const {
  std::vector<std::string> node_types;
  std::shared_lock<std::shared_mutex> lock{mutex_};
  node_types.reserve(creators_.size());
  for (const auto& [node_type, creator] : creators_) {
    (void)creator;
    node_types.push_back(node_type);
  }
  return node_types;
}

std::size_t BehaviorTreeFactory::Size() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return creators_.size();
}

} // namespace humanoid::bt
