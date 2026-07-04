#include <AudioAdapter.h>
#include <HandAdapter.h>
#include <LocoAdapter.h>
#include <SdkTypes.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

namespace unitree_sdk = humanoid::plugins::unitree::sdk;

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] bool TestLocoValidationWithoutHardware() {
  constexpr std::string_view kTestName{"Unitree loco adapter validation"};

  unitree_sdk::LocoAdapter adapter;
  const unitree_sdk::SdkResult invalid_velocity = adapter.SetVelocity(
      unitree_sdk::SdkVelocityCommand{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F});
  if (invalid_velocity.Succeeded()) {
    return Fail(kTestName, "non-finite velocity accepted");
  }

  const unitree_sdk::SdkResult stop = adapter.Stop();
  if (stop.Succeeded() || stop.code != unitree_sdk::SdkErrorCode::kConnectionFailed) {
    return Fail(kTestName, "stop without initialization did not fail safely");
  }

  return true;
}

[[nodiscard]] bool TestHandValidationWithoutHardware() {
  constexpr std::string_view kTestName{"Unitree hand adapter validation"};

  unitree_sdk::HandAdapter adapter;
  const unitree_sdk::SdkResult open = adapter.Open();
  if (open.Succeeded()) {
    return Fail(kTestName, "unsupported open command accepted");
  }

  const unitree_sdk::SdkResult gesture = adapter.Gesture(unitree_sdk::SdkHandGesture::kWave);
  if (gesture.Succeeded() || gesture.code != unitree_sdk::SdkErrorCode::kConnectionFailed) {
    return Fail(kTestName, "gesture without connection did not fail safely");
  }

  return true;
}

[[nodiscard]] bool TestAudioValidationWithoutHardware() {
  constexpr std::string_view kTestName{"Unitree audio adapter validation"};

  unitree_sdk::AudioAdapter adapter;
  const unitree_sdk::SdkAudioPlayback empty_playback;
  const unitree_sdk::SdkResult play = adapter.Play(empty_playback);
  if (play.Succeeded()) {
    return Fail(kTestName, "empty playback accepted");
  }

  const unitree_sdk::SdkResult volume = adapter.SetVolume(101U);
  if (volume.Succeeded()) {
    return Fail(kTestName, "out-of-range volume accepted");
  }

  const unitree_sdk::SdkResult stop = adapter.Stop("");
  if (stop.Succeeded()) {
    return Fail(kTestName, "empty app name accepted");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestLocoValidationWithoutHardware,
      TestHandValidationWithoutHardware,
      TestAudioValidationWithoutHardware,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
