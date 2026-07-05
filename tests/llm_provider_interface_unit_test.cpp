#include <humanoid/ai/ILLMProvider.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

namespace {

void Require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

[[nodiscard]] humanoid::ai::LLMCapabilities MakeCapabilities() {
  humanoid::ai::LLMCapabilities capabilities;
  capabilities.providerId = "test.provider";
  capabilities.name = "Test Provider";
  capabilities.description = "LLM provider abstraction unit test";
  capabilities.providerKind = humanoid::ai::LLMProviderKind::Local;
  capabilities.supportedModels = {"local-test-model"};
  capabilities.supportsStreaming = true;
  capabilities.supportsToolCalls = true;
  capabilities.supportsCancellation = true;
  capabilities.contextWindowTokens = 4096U;
  capabilities.maxOutputTokens = 1024U;
  return capabilities;
}

class MockLLMProvider final : public humanoid::ai::ILLMProvider {
public:
  explicit MockLLMProvider(humanoid::ai::LLMCapabilities capabilities)
      : capabilities_(std::move(capabilities)) {}

  [[nodiscard]] humanoid::ai::LLMResponse
  Generate(const humanoid::ai::LLMRequest& request) override {
    humanoid::ai::LLMResponse response;
    if (!request.isValid()) {
      response.status = humanoid::common::Status::error(
          humanoid::common::StatusCode::kInvalidArgument, "invalid LLM request");
      response.finishReason = humanoid::ai::LLMFinishReason::Error;
      return response;
    }

    response.id = request.id;
    response.model = request.model.empty() ? capabilities_.supportedModels.front() : request.model;
    response.content = "deterministic mock response";
    response.finishReason = humanoid::ai::LLMFinishReason::Complete;
    response.usage.promptTokens = 4U;
    response.usage.completionTokens = 3U;
    response.usage.totalTokens = 7U;
    response.metadata.emplace("provider_id", capabilities_.providerId);
    return response;
  }

  [[nodiscard]] humanoid::common::Status Cancel() override {
    cancelled_ = true;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::ai::LLMCapabilities GetCapabilities() const override {
    return capabilities_;
  }

  [[nodiscard]] bool Cancelled() const noexcept { return cancelled_; }

private:
  humanoid::ai::LLMCapabilities capabilities_;
  bool cancelled_{false};
};

void VerifyRequestValidation() {
  humanoid::ai::LLMRequest request;
  Require(!request.isValid());

  request.id = 42U;
  request.model = "local-test-model";
  request.messages.push_back(
      humanoid::ai::LLMMessage{humanoid::ai::LLMMessageRole::System, "stay deterministic", {}});
  request.messages.push_back(
      humanoid::ai::LLMMessage{humanoid::ai::LLMMessageRole::User, "plan inspection", {}});
  request.parameters.emplace("temperature", 0.0);
  request.parameters.emplace("max_tokens", std::int64_t{128});
  request.metadata.emplace("source", "unit-test");
  request.timeout = std::chrono::milliseconds{250};

  Require(request.isValid());
  Require(request.hasTimeout());
}

void VerifyCapabilitiesAndResponse() {
  const humanoid::ai::LLMCapabilities capabilities = MakeCapabilities();
  Require(capabilities.isValid());
  Require(capabilities.providerKind == humanoid::ai::LLMProviderKind::Local);
  Require(capabilities.supportsStreaming);
  Require(capabilities.supportsToolCalls);
  Require(capabilities.supportsCancellation);

  humanoid::ai::LLMResponse response;
  response.id = 7U;
  response.model = "local-test-model";
  response.content = "ok";
  response.finishReason = humanoid::ai::LLMFinishReason::Complete;
  Require(response.isSuccess());

  response.finishReason = humanoid::ai::LLMFinishReason::Error;
  Require(!response.isSuccess());
}

void VerifyProviderDependencyInjection() {
  std::unique_ptr<humanoid::ai::ILLMProvider> provider =
      std::make_unique<MockLLMProvider>(MakeCapabilities());

  humanoid::ai::LLMRequest request;
  request.id = 99U;
  request.messages.push_back(
      humanoid::ai::LLMMessage{humanoid::ai::LLMMessageRole::User, "hello", {}});

  humanoid::ai::LLMResponse response = provider->Generate(request);
  Require(response.isSuccess());
  Require(response.id == request.id);
  Require(response.model == "local-test-model");
  Require(response.metadata.at("provider_id") == "test.provider");

  Require(provider->Cancel().isOk());
  Require(provider->GetCapabilities().providerId == "test.provider");
}

void VerifyInvalidProviderRequest() {
  MockLLMProvider provider{MakeCapabilities()};
  const humanoid::ai::LLMResponse response = provider.Generate(humanoid::ai::LLMRequest{});
  Require(!response.isSuccess());
  Require(response.status.code() == humanoid::common::StatusCode::kInvalidArgument);
  Require(response.finishReason == humanoid::ai::LLMFinishReason::Error);
}

} // namespace

int main() {
  VerifyRequestValidation();
  VerifyCapabilitiesAndResponse();
  VerifyProviderDependencyInjection();
  VerifyInvalidProviderRequest();
  return 0;
}
