#pragma once

/**
 * @file LLMResponse.h
 * @brief Defines provider-neutral LLM response data.
 */

#include <cstdint>
#include <map>
#include <string>

#include <humanoid/common/Status.hpp>

namespace humanoid::ai {

/**
 * @brief Stable LLM response identifier.
 */
using LLMResponseId = std::uint64_t;

/**
 * @brief Provider-neutral completion finish reason.
 */
enum class LLMFinishReason {
  Complete,      ///< Provider reported a complete response.
  LengthLimit,   ///< Provider stopped because of an output length limit.
  ToolCall,      ///< Provider stopped to request a tool call.
  ContentFilter, ///< Provider stopped due to safety or content filtering.
  Cancelled,     ///< Request was cancelled cooperatively.
  Error,         ///< Provider failed before producing a normal response.
  Unknown        ///< Provider did not expose a normalized finish reason.
};

/**
 * @brief Token usage reported by a provider when available.
 */
struct LLMTokenUsage final {
  /**
   * @brief Prompt or input token count.
   */
  std::uint64_t promptTokens{0U};

  /**
   * @brief Completion or output token count.
   */
  std::uint64_t completionTokens{0U};

  /**
   * @brief Total token count.
   */
  std::uint64_t totalTokens{0U};
};

/**
 * @brief Ordered non-operational response annotations.
 */
using LLMResponseMetadata = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Provider-neutral response returned by an `ILLMProvider`.
 *
 * The response contains normalized text, status, finish reason, usage, and
 * metadata only. It does not expose provider SDK objects, HTTP responses,
 * sockets, credentials, or provider-specific enum values.
 */
struct LLMResponse final {
  /**
   * @brief Provider- or adapter-assigned response identifier; zero is unassigned.
   */
  LLMResponseId id{0U};

  /**
   * @brief Model that produced the response when known.
   */
  std::string model;

  /**
   * @brief Generated response text.
   */
  std::string content;

  /**
   * @brief Operation status for the provider request.
   */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /**
   * @brief Normalized completion finish reason.
   */
  LLMFinishReason finishReason{LLMFinishReason::Unknown};

  /**
   * @brief Token usage reported by the provider when available.
   */
  LLMTokenUsage usage;

  /**
   * @brief Non-operational response metadata for tracing and diagnostics.
   */
  LLMResponseMetadata metadata;

  /**
   * @brief Constructs an empty successful response value.
   */
  LLMResponse() = default;

  /**
   * @brief Reports whether provider execution succeeded.
   *
   * @return True when the status is OK and finish reason is not error or cancelled.
   */
  [[nodiscard]] bool isSuccess() const noexcept {
    return status.isOk() && finishReason != LLMFinishReason::Error &&
           finishReason != LLMFinishReason::Cancelled;
  }
};

} // namespace humanoid::ai
