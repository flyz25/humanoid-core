#pragma once

/**
 * @file AudioAdapter.h
 * @brief Defines the Unitree G1 audio SDK adapter.
 */

#include <cstdint>
#include <memory>
#include <string>

#include "SdkTypes.h"

namespace humanoid::plugins::unitree::sdk {

/**
 * @brief Thread-safe adapter that translates audio commands into Unitree SDK2 calls.
 *
 * AudioAdapter owns the Unitree G1 audio client internally and exposes only
 * humanoid-core SDK abstraction types.
 */
class AudioAdapter final {
public:
  /**
   * @brief Constructs an uninitialized audio adapter.
   */
  AudioAdapter();

  /**
   * @brief Releases the owned SDK client.
   */
  ~AudioAdapter() noexcept;

  AudioAdapter(const AudioAdapter&) = delete;
  AudioAdapter& operator=(const AudioAdapter&) = delete;
  AudioAdapter(AudioAdapter&&) = delete;
  AudioAdapter& operator=(AudioAdapter&&) = delete;

  /**
   * @brief Initializes the Unitree SDK2 audio client.
   *
   * @param configuration SDK configuration.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Initialize(const SdkConfiguration& configuration);

  /**
   * @brief Releases the Unitree SDK2 audio client.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Shutdown();

  /**
   * @brief Verifies communication with the audio service.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Connect();

  /**
   * @brief Starts audio playback from PCM bytes.
   *
   * @param playback Playback payload.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Play(const SdkAudioPlayback& playback);

  /**
   * @brief Stops playback for an application name.
   *
   * @param app_name Application name.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Stop(const std::string& app_name);

  /**
   * @brief Sets audio volume.
   *
   * @param volume Volume in percent, in the range [0, 100].
   * @return Operation result.
   */
  [[nodiscard]] SdkResult SetVolume(std::uint8_t volume);

  /**
   * @brief Mutes audio by setting volume to zero.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Mute();

  /**
   * @brief Reports whether the SDK client is initialized.
   *
   * @return True when initialized.
   */
  [[nodiscard]] bool IsInitialized() const noexcept;

  /**
   * @brief Reports whether communication has been verified.
   *
   * @return True when connected.
   */
  [[nodiscard]] bool IsConnected() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::plugins::unitree::sdk
