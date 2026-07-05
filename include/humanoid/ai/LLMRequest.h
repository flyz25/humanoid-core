#pragma once

/**
 * @file LLMRequest.h
 * @brief Defines vendor-neutral request data for LLM providers.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace humanoid::ai {

/**
 * @brief Stable LLM request identifier.
 *
 * A value of zero is unassigned. Applications or future planner components own
 * identifier generation.
 */
using LLMRequestId = std::uint64_t;

/**
 * @brief Duration type used for provider request timeouts.
 */
using LLMRequestTimeout = std::chrono::milliseconds;

/**
 * @brief Message role within a provider-neutral LLM request.
 */
enum class LLMMessageRole {
  System,    ///< System-level instruction.
  User,      ///< User or operator message.
  Assistant, ///< Assistant/model message.
  Tool       ///< Tool result or tool-facing message.
};

/**
 * @brief One provider-neutral LLM message.
 */
struct LLMMessage final {
  /**
   * @brief Role associated with the message.
   */
  LLMMessageRole role{LLMMessageRole::User};

  /**
   * @brief UTF-8 text content for the message.
   */
  std::string content;

  /**
   * @brief Optional provider-neutral message name or tool identifier.
   */
  std::string name;

  /**
   * @brief Reports whether this message can be sent to a provider boundary.
   *
   * @return True when message content is not empty.
   */
  [[nodiscard]] bool isValid() const noexcept { return !content.empty(); }
};

/**
 * @brief Supported scalar request parameter values.
 *
 * Parameters must remain provider-neutral. Provider SDK objects, transport
 * handles, credentials, and pointers are forbidden.
 */
using LLMParameterValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered provider-neutral request parameters.
 */
using LLMParameters = std::map<std::string, LLMParameterValue, std::less<>>;

/**
 * @brief Ordered non-operational request annotations.
 */
using LLMRequestMetadata = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Provider-neutral request for an abstract LLM provider.
 *
 * The request contains no SDK objects, HTTP handles, credentials, provider
 * enums, or transport-specific configuration. Concrete provider adapters may
 * translate this value into OpenAI, Anthropic, Gemini, Ollama, local-model, or
 * custom provider requests outside the core abstraction boundary.
 */
struct LLMRequest final {
  /**
   * @brief Producer-assigned request identifier; zero is unassigned.
   */
  LLMRequestId id{0U};

  /**
   * @brief Optional model identifier. Empty means provider default.
   */
  std::string model;

  /**
   * @brief Ordered prompt or chat messages.
   */
  std::vector<LLMMessage> messages;

  /**
   * @brief Provider-neutral generation parameters.
   */
  LLMParameters parameters;

  /**
   * @brief Non-operational metadata for tracing and correlation.
   */
  LLMRequestMetadata metadata;

  /**
   * @brief Maximum provider response duration; zero disables timeout policy.
   */
  LLMRequestTimeout timeout{LLMRequestTimeout::zero()};

  /**
   * @brief Constructs an empty request.
   */
  LLMRequest() = default;

  /**
   * @brief Reports whether this request is structurally valid.
   *
   * @return True when all messages are valid and timeout is nonnegative.
   */
  [[nodiscard]] bool isValid() const noexcept {
    if (messages.empty() || timeout < LLMRequestTimeout::zero()) {
      return false;
    }

    for (const LLMMessage& message : messages) {
      if (!message.isValid()) {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Reports whether timeout enforcement is requested.
   *
   * @return True when timeout is greater than zero.
   */
  [[nodiscard]] constexpr bool hasTimeout() const noexcept {
    return timeout > LLMRequestTimeout::zero();
  }
};

} // namespace humanoid::ai
