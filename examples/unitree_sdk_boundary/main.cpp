#include <AudioAdapter.h>
#include <HandAdapter.h>
#include <LocoAdapter.h>
#include <SdkTypes.h>
#include <SdkWrapper.h>

#include <cstdlib>
#include <iostream>
#include <limits>

int main() {
  namespace unitree_sdk = humanoid::plugins::unitree::sdk;

  unitree_sdk::SdkWrapper wrapper;
  const unitree_sdk::SdkRobotState initial_state = wrapper.ReadRobotState();

  unitree_sdk::LocoAdapter loco_adapter;
  const unitree_sdk::SdkResult invalid_velocity = loco_adapter.SetVelocity(
      unitree_sdk::SdkVelocityCommand{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F});

  unitree_sdk::HandAdapter hand_adapter;
  const unitree_sdk::SdkResult unsupported_open = hand_adapter.Open();

  unitree_sdk::AudioAdapter audio_adapter;
  const unitree_sdk::SdkResult invalid_volume = audio_adapter.SetVolume(101U);

  if (invalid_velocity.Succeeded() || unsupported_open.Succeeded() || invalid_volume.Succeeded()) {
    std::cerr << "SDK boundary validation unexpectedly accepted an invalid command" << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "Unitree SDK boundary example validated command adapters; initial_connected="
            << (initial_state.connection_state == unitree_sdk::SdkConnectionState::kConnected
                    ? "yes"
                    : "no")
            << '\n';
  return EXIT_SUCCESS;
}
