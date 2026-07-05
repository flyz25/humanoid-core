#pragma once

/**
 * @file LLMCapabilities.h
 * @brief Defines provider-neutral LLM provider capabilities.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace humanoid::ai {

/**
 * @brief Identifies the family of an abstract LLM provider.
 *
 * The enum is metadata for selection only. It does not expose SDK types,
 * authentication types, HTTP clients, or provider-specific configuration.
 */
enum class LLMProviderKind {
  OpenAI,    ///< Future OpenAI-compatible provider adapter.
  Anthropic, ///< Future Anthropic-compatible provider adapter.
  Gemini,    ///< Future Gemini-compatible provider adapter.
  Ollama,    ///< Future Ollama-compatible provider adapter.
  Local,     ///< Future local model provider adapter.
  Custom     ///< Application-defined provider adapter.
};

/**
 * @brief Provider-neutral capability declaration for an LLM provider.
 */
struct LLMCapabilities final {
  /**
   * @brief Stable provider identifier within a process or plugin host.
   */
  std::string providerId;

  /**
   * @brief Human-readable provider name.
   */
  std::string name;

  /**
   * @brief Human-readable provider description.
   */
  std::string description;

  /**
   * @brief Provider family.
   */
  LLMProviderKind providerKind{LLMProviderKind::Custom};

  /**
   * @brief Model identifiers supported or configured by the provider.
   */
  std::vector<std::string> supportedModels;

  /**
   * @brief True when provider adapter supports streaming responses.
   */
  bool supportsStreaming{false};

  /**
   * @brief True when provider adapter supports tool call output.
   */
  bool supportsToolCalls{false};

  /**
   * @brief True when provider adapter supports embeddings.
   */
  bool supportsEmbeddings{false};

  /**
   * @brief True when provider adapter supports cooperative cancellation.
   */
  bool supportsCancellation{false};

  /**
   * @brief Maximum supported input context tokens when known; zero means unknown.
   */
  std::uint64_t contextWindowTokens{0U};

  /**
   * @brief Maximum supported output tokens when known; zero means unknown.
   */
  std::uint64_t maxOutputTokens{0U};

  /**
   * @brief Reports whether this capability declaration is usable.
   *
   * @return True when provider identity fields are assigned.
   */
  [[nodiscard]] bool isValid() const noexcept { return !providerId.empty() && !name.empty(); }
};

} // namespace humanoid::ai
