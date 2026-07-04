#include <humanoid/network/TransportProtocol.hpp>

namespace humanoid::network {

std::string_view toString(TransportProtocol protocol) noexcept {
  switch (protocol) {
  case TransportProtocol::kTcp:
    return "tcp";
  case TransportProtocol::kUdp:
    return "udp";
  case TransportProtocol::kDds:
    return "dds";
  }

  return "unknown";
}

} // namespace humanoid::network
