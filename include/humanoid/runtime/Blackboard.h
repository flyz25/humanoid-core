#pragma once

/**
 * @file Blackboard.h
 * @brief Defines the generic thread-safe runtime blackboard.
 */

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <utility>

namespace humanoid::runtime {

/**
 * @brief Logical namespace used to isolate blackboard keys.
 */
using BlackboardNamespace = std::string;

/**
 * @brief Key identifying one value inside a blackboard namespace.
 */
using BlackboardKey = std::string;

namespace detail {

template <typename T> struct IsSharedPointer final : std::false_type {};

template <typename T> struct IsSharedPointer<std::shared_ptr<T>> final : std::true_type {};

template <typename T>
inline constexpr bool kIsSharedPointer = IsSharedPointer<std::remove_cv_t<T>>::value;

} // namespace detail

/**
 * @brief Thread-safe namespaced store for shared, strongly typed runtime data.
 *
 * Values are immutable through the blackboard API and are returned as
 * `std::shared_ptr<const T>`. This preserves value lifetime after replacement,
 * removal, or clear operations without exposing the blackboard's internal
 * synchronization. A stored type must be requested using the exact same type.
 *
 * The blackboard owns no execution policy and contains no vendor, mission, or
 * behavior-tree logic.
 */
class Blackboard final {
public:
  /** @brief Constructs an empty blackboard. */
  Blackboard() = default;

  /** @brief Destroys the blackboard and releases stored ownership. */
  ~Blackboard() = default;

  Blackboard(const Blackboard&) = delete;
  Blackboard& operator=(const Blackboard&) = delete;
  Blackboard(Blackboard&&) = delete;
  Blackboard& operator=(Blackboard&&) = delete;

  /**
   * @brief Stores a value using blackboard-managed shared ownership.
   *
   * Existing values at the same namespace and key are atomically replaced.
   * Raw pointers are rejected to avoid ambiguous lifetime ownership.
   *
   * @tparam T Stored value type.
   * @param namespace_name Non-empty logical namespace.
   * @param key Non-empty key inside the namespace.
   * @param value Value moved into shared immutable storage.
   * @return True when the value was stored; false for an invalid address.
   */
  template <typename T>
  requires(!detail::kIsSharedPointer<std::remove_cvref_t<T>> &&
           !std::is_pointer_v<std::remove_cvref_t<T>>) bool Store(std::string_view namespace_name,
                                                                  std::string_view key, T value) {
    using Value = std::remove_cv_t<T>;
    return StoreShared<Value>(namespace_name, key, std::make_shared<const Value>(std::move(value)));
  }

  /**
   * @brief Stores an existing shared value.
   *
   * Existing values at the same namespace and key are atomically replaced.
   * The blackboard and caller share ownership of an immutable view.
   *
   * @tparam T Stored value type.
   * @param namespace_name Non-empty logical namespace.
   * @param key Non-empty key inside the namespace.
   * @param value Shared value; null values are rejected.
   * @return True when the value was stored; false for invalid input.
   */
  template <typename T>
  requires(std::is_object_v<std::remove_cv_t<T>> &&
           !std::is_pointer_v<std::remove_cv_t<T>>) bool Store(std::string_view namespace_name,
                                                               std::string_view key,
                                                               std::shared_ptr<T> value) {
    using Value = std::remove_cv_t<T>;
    std::shared_ptr<const Value> immutable_value = std::move(value);
    return StoreShared<Value>(namespace_name, key, std::move(immutable_value));
  }

  /**
   * @brief Returns shared ownership of a typed value.
   *
   * @tparam T Exact stored value type.
   * @param namespace_name Logical namespace.
   * @param key Key inside the namespace.
   * @return Immutable shared value, or empty for a missing key or type mismatch.
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<const std::remove_cv_t<T>> Get(std::string_view namespace_name,
                                                               std::string_view key) const {
    static_assert(!std::is_reference_v<T>, "Blackboard values cannot be reference types");
    static_assert(!std::is_pointer_v<T>, "Blackboard values cannot be raw pointer types");
    using Value = std::remove_cv_t<T>;
    static_assert(std::is_object_v<Value>, "Blackboard values must be object types");

    const std::shared_ptr<const ValueBase> stored = GetValue(namespace_name, key);
    if (!stored || stored->type != std::type_index{typeid(Value)}) {
      return {};
    }
    return std::static_pointer_cast<const ValueHolder<Value>>(stored)->value;
  }

  /**
   * @brief Reports whether any value exists at an address.
   *
   * @param namespace_name Logical namespace.
   * @param key Key inside the namespace.
   * @return True when the address contains a value of any type.
   */
  [[nodiscard]] bool Contains(std::string_view namespace_name, std::string_view key) const;

  /**
   * @brief Removes one value from the blackboard.
   *
   * Existing shared handles to the value remain valid.
   *
   * @param namespace_name Logical namespace.
   * @param key Key inside the namespace.
   * @return True when a value was removed.
   */
  bool Remove(std::string_view namespace_name, std::string_view key);

  /**
   * @brief Clears all values from one namespace.
   *
   * @param namespace_name Logical namespace to clear.
   * @return Number of values removed.
   */
  std::size_t Clear(std::string_view namespace_name);

  /**
   * @brief Clears all namespaces and values.
   *
   * @return Number of values removed.
   */
  std::size_t Clear();

private:
  struct ValueBase {
    explicit ValueBase(std::type_index value_type) noexcept : type(value_type) {}
    virtual ~ValueBase() = default;

    const std::type_index type;
  };

  template <typename T> struct ValueHolder final : ValueBase {
    explicit ValueHolder(std::shared_ptr<const T> stored_value)
        : ValueBase(std::type_index{typeid(T)}), value(std::move(stored_value)) {}

    std::shared_ptr<const T> value;
  };

  using StoredValue = std::shared_ptr<const ValueBase>;
  using NamespaceValues = std::map<BlackboardKey, StoredValue, std::less<>>;
  using Values = std::map<BlackboardNamespace, NamespaceValues, std::less<>>;

  template <typename T>
  bool StoreShared(std::string_view namespace_name, std::string_view key,
                   std::shared_ptr<const T> value) {
    if (!value) {
      return false;
    }
    return StoreValue(namespace_name, key,
                      std::make_shared<const ValueHolder<T>>(std::move(value)));
  }

  bool StoreValue(std::string_view namespace_name, std::string_view key, StoredValue value);

  [[nodiscard]] StoredValue GetValue(std::string_view namespace_name, std::string_view key) const;

  mutable std::shared_mutex mutex_;
  Values values_;
};

} // namespace humanoid::runtime
