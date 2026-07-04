#include "AudioAdapter.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "SdkClientSupport.h"

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/audio/g1_audio_client.hpp>

namespace humanoid::plugins::unitree::sdk {
namespace {

constexpr std::uint8_t kMaxVolumePercent = 100U;
constexpr std::uint8_t kMutedVolumePercent = 0U;

[[nodiscard]] SdkResult ValidatePlayback(const SdkAudioPlayback& playback) {
  if (playback.app_name.empty()) {
    return internal::Failure(SdkErrorCode::kUnknown, "audio playback app_name is empty");
  }

  if (playback.stream_id.empty()) {
    return internal::Failure(SdkErrorCode::kUnknown, "audio playback stream_id is empty");
  }

  if (playback.pcm_data.empty()) {
    return internal::Failure(SdkErrorCode::kUnknown, "audio playback pcm_data is empty");
  }

  return internal::Success("audio playback payload is valid");
}

[[nodiscard]] SdkResult ValidateAppName(const std::string& app_name) {
  if (app_name.empty()) {
    return internal::Failure(SdkErrorCode::kUnknown, "audio app_name is empty");
  }

  return internal::Success("audio app_name is valid");
}

[[nodiscard]] SdkResult ValidateVolume(std::uint8_t volume) {
  if (volume > kMaxVolumePercent) {
    return internal::Failure(SdkErrorCode::kUnknown, "audio volume must be in range [0, 100]");
  }

  return internal::Success("audio volume is valid");
}

} // namespace

class AudioAdapter::Impl final {
public:
  Impl() = default;
  ~Impl() noexcept = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] SdkResult RequireInitialized(const char* command) const {
    if (!initialized_ || !client_) {
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               std::string{command} + " requires initialized audio client");
    }

    return internal::Success(std::string{command} + " precondition satisfied");
  }

  [[nodiscard]] SdkResult RequireConnected(const char* command) const {
    if (!initialized_ || !client_ || !connected_) {
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               std::string{command} + " requires connected audio client");
    }

    return internal::Success(std::string{command} + " precondition satisfied");
  }

  mutable std::mutex mutex_;
  std::unique_ptr<::unitree::robot::g1::AudioClient> client_;
  SdkConfiguration configuration_{};
  bool initialized_{false};
  bool connected_{false};
};

AudioAdapter::AudioAdapter() : impl_(std::make_unique<Impl>()) {}

AudioAdapter::~AudioAdapter() noexcept {
  try {
    static_cast<void>(Shutdown());
  } catch (...) {
  }
}

SdkResult AudioAdapter::Initialize(const SdkConfiguration& configuration) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (impl_->initialized_) {
    return internal::Success("Unitree audio adapter already initialized");
  }

  SdkResult validation = internal::ValidateSdkConfiguration(configuration);
  if (!validation.Succeeded()) {
    return validation;
  }

  try {
    ::unitree::robot::ChannelFactory::Instance()->Init(static_cast<int>(configuration.domain_id),
                                                       configuration.network_interface);
    impl_->client_ = std::make_unique<::unitree::robot::g1::AudioClient>();
    impl_->client_->Init();
    impl_->client_->SetTimeout(internal::TimeoutSeconds(configuration.timeout));
  } catch (const std::exception& exception) {
    impl_->client_.reset();
    impl_->initialized_ = false;
    impl_->connected_ = false;
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             std::string{"Unitree audio adapter initialization failed: "} +
                                 exception.what());
  } catch (...) {
    impl_->client_.reset();
    impl_->initialized_ = false;
    impl_->connected_ = false;
    return internal::Failure(
        SdkErrorCode::kConnectionFailed,
        "Unitree audio adapter initialization failed with an unknown exception");
  }

  impl_->configuration_ = configuration;
  impl_->initialized_ = true;
  impl_->connected_ = false;
  return internal::Success("Unitree audio adapter initialized");
}

SdkResult AudioAdapter::Shutdown() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  impl_->client_.reset();
  impl_->initialized_ = false;
  impl_->connected_ = false;
  return internal::Success("Unitree audio adapter shut down");
}

SdkResult AudioAdapter::Connect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireInitialized("Connect");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  std::uint8_t volume = 0U;
  SdkResult result =
      internal::InvokeSdkCommand("Audio.Connect", SdkErrorCode::kConnectionFailed,
                                 [this, &volume]() { return impl_->client_->GetVolume(volume); });
  impl_->connected_ = result.Succeeded();
  return result;
}

SdkResult AudioAdapter::Play(const SdkAudioPlayback& playback) {
  SdkResult validation = ValidatePlayback(playback);
  if (!validation.Succeeded()) {
    return validation;
  }

  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Play");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Audio.Play", SdkErrorCode::kRobotFault, [this, &playback]() {
    return impl_->client_->PlayStream(playback.app_name, playback.stream_id, playback.pcm_data);
  });
}

SdkResult AudioAdapter::Stop(const std::string& app_name) {
  SdkResult validation = ValidateAppName(app_name);
  if (!validation.Succeeded()) {
    return validation;
  }

  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Stop");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Audio.Stop", SdkErrorCode::kRobotFault, [this, &app_name]() {
    return impl_->client_->PlayStop(app_name);
  });
}

SdkResult AudioAdapter::SetVolume(std::uint8_t volume) {
  SdkResult validation = ValidateVolume(volume);
  if (!validation.Succeeded()) {
    return validation;
  }

  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("SetVolume");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Audio.SetVolume", SdkErrorCode::kRobotFault,
                                    [this, volume]() { return impl_->client_->SetVolume(volume); });
}

SdkResult AudioAdapter::Mute() { return SetVolume(kMutedVolumePercent); }

bool AudioAdapter::IsInitialized() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->initialized_;
}

bool AudioAdapter::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->connected_;
}

} // namespace humanoid::plugins::unitree::sdk
