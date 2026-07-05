#include <humanoid/runtime/Blackboard.h>

#include <mutex>
#include <utility>

namespace humanoid::runtime {
namespace {

[[nodiscard]] bool IsValidAddress(std::string_view namespace_name, std::string_view key) noexcept {
  return !namespace_name.empty() && !key.empty();
}

} // namespace

bool Blackboard::StoreValue(std::string_view namespace_name, std::string_view key,
                            StoredValue value) {
  if (!IsValidAddress(namespace_name, key) || !value) {
    return false;
  }

  BlackboardNamespace owned_namespace{namespace_name};
  BlackboardKey owned_key{key};
  std::unique_lock<std::shared_mutex> lock{mutex_};
  auto [namespace_iterator, inserted_namespace] = values_.try_emplace(std::move(owned_namespace));
  try {
    namespace_iterator->second.insert_or_assign(std::move(owned_key), std::move(value));
  } catch (...) {
    if (inserted_namespace && namespace_iterator->second.empty()) {
      values_.erase(namespace_iterator);
    }
    throw;
  }
  return true;
}

Blackboard::StoredValue Blackboard::GetValue(std::string_view namespace_name,
                                             std::string_view key) const {
  if (!IsValidAddress(namespace_name, key)) {
    return {};
  }

  std::shared_lock<std::shared_mutex> lock{mutex_};
  const auto namespace_iterator = values_.find(namespace_name);
  if (namespace_iterator == values_.end()) {
    return {};
  }
  const auto value_iterator = namespace_iterator->second.find(key);
  if (value_iterator == namespace_iterator->second.end()) {
    return {};
  }
  return value_iterator->second;
}

bool Blackboard::Contains(std::string_view namespace_name, std::string_view key) const {
  return static_cast<bool>(GetValue(namespace_name, key));
}

bool Blackboard::Remove(std::string_view namespace_name, std::string_view key) {
  if (!IsValidAddress(namespace_name, key)) {
    return false;
  }

  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto namespace_iterator = values_.find(namespace_name);
  if (namespace_iterator == values_.end()) {
    return false;
  }
  const auto value_iterator = namespace_iterator->second.find(key);
  if (value_iterator == namespace_iterator->second.end()) {
    return false;
  }
  namespace_iterator->second.erase(value_iterator);
  if (namespace_iterator->second.empty()) {
    values_.erase(namespace_iterator);
  }
  return true;
}

std::size_t Blackboard::Clear(std::string_view namespace_name) {
  if (namespace_name.empty()) {
    return 0U;
  }

  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto namespace_iterator = values_.find(namespace_name);
  if (namespace_iterator == values_.end()) {
    return 0U;
  }
  const std::size_t removed = namespace_iterator->second.size();
  values_.erase(namespace_iterator);
  return removed;
}

std::size_t Blackboard::Clear() {
  std::unique_lock<std::shared_mutex> lock{mutex_};
  std::size_t removed = 0U;
  for (const auto& entry : values_) {
    removed += entry.second.size();
  }
  values_.clear();
  return removed;
}

} // namespace humanoid::runtime
