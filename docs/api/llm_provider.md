# LLM Provider Abstraction

Milestone 9.4 defines a provider-neutral LLM boundary for future AI-assisted
planning components. It does not add any provider implementation, provider SDK,
HTTP client, authentication flow, network transport, prompt orchestration, or
planner integration.

## Public Headers

```cpp
#include <humanoid/ai/ILLMProvider.h>
#include <humanoid/ai/LLMCapabilities.h>
#include <humanoid/ai/LLMRequest.h>
#include <humanoid/ai/LLMResponse.h>
```

All types are in `humanoid::ai` and are available through the
`<humanoid/core.hpp>` umbrella header.

## Provider Families

`LLMProviderKind` identifies the provider family for selection and diagnostics:

- `OpenAI`
- `Anthropic`
- `Gemini`
- `Ollama`
- `Local`
- `Custom`

The enum is metadata only. It does not expose SDK objects, authentication
configuration, HTTP transports, provider-specific request types, or provider
enums.

## Request

`LLMRequest` is a framework-owned value type containing:

- A producer-assigned request ID.
- Optional model identifier.
- Ordered messages with provider-neutral roles.
- Ordered scalar generation parameters.
- String metadata for tracing and correlation.
- A `std::chrono::milliseconds` timeout.

`LLMMessageRole` supports system, user, assistant, and tool messages. Request
parameters may contain only `bool`, `std::int64_t`, `double`, and `std::string`
values. Provider SDK objects, transport handles, pointers, credentials, and
HTTP objects are forbidden.

`LLMRequest::isValid()` requires at least one non-empty message and a
nonnegative timeout.

## Response

`LLMResponse` contains:

- Response ID.
- Model identifier when known.
- Generated text content.
- `humanoid::common::Status`.
- Provider-neutral `LLMFinishReason`.
- Token usage when reported by the provider.
- Response metadata for diagnostics.

`LLMResponse::isSuccess()` is true when status is OK and the finish reason is
not error or cancelled.

## Capabilities

`LLMCapabilities` declares:

- Provider ID, name, and description.
- Provider family.
- Supported or configured model names.
- Streaming, tool-call, embedding, and cancellation support.
- Context window and output token limits when known.

`LLMCapabilities::isValid()` requires only provider ID and name. Model lists and
token limits may be dynamic for local or custom providers.

## Provider Interface

`ILLMProvider` is pure abstract:

- `Generate()`
- `Cancel()`
- `GetCapabilities()`

Concrete provider adapters will be injected through this interface in future
milestones. Implementations must contain provider exceptions and translate
failures into `LLMResponse::status`.

## Dependency Boundary

```text
Future AI planner or application composition root
  -> ILLMProvider
    -> LLMRequest
    -> LLMResponse
    -> LLMCapabilities
```

The abstraction contains no OpenAI SDK, Anthropic SDK, Gemini SDK, Ollama HTTP
implementation, local model runtime, REST client, socket, credential store, or
provider-specific transport code.
