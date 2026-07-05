#pragma once

/**
 * @file ILLMProvider.h
 * @brief Defines the abstract LLM provider boundary.
 */

#include <humanoid/ai/LLMCapabilities.h>
#include <humanoid/ai/LLMRequest.h>
#include <humanoid/ai/LLMResponse.h>
#include <humanoid/common/Status.hpp>

namespace humanoid::ai {

/**
 * @brief Pure abstract provider interface for future LLM adapters.
 *
 * `ILLMProvider` is a dependency-injection boundary. Applications, planners,
 * or future AI services may depend on this interface without depending on
 * OpenAI, Anthropic, Gemini, Ollama, local model runtimes, HTTP clients,
 * credentials, or provider SDKs.
 */
class ILLMProvider {
public:
  /**
   * @brief Destroys the provider interface.
   */
  virtual ~ILLMProvider() = default;

  /**
   * @brief Generates a provider-neutral response for a provider-neutral request.
   *
   * Implementations must contain provider exceptions and translate failures
   * into `LLMResponse::status`.
   *
   * @param request Provider-neutral LLM request.
   * @return Provider-neutral LLM response.
   */
  [[nodiscard]] virtual LLMResponse Generate(const LLMRequest& request) = 0;

  /**
   * @brief Requests cooperative cancellation of in-progress provider work.
   *
   * The interface does not define preemptive interruption or transport
   * behavior. Concrete providers that support cancellation must observe this
   * request at safe internal boundaries.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Cancel() = 0;

  /**
   * @brief Returns provider capabilities.
   *
   * @return Provider-neutral capability declaration.
   */
  [[nodiscard]] virtual LLMCapabilities GetCapabilities() const = 0;
};

} // namespace humanoid::ai
