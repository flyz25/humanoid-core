#include <humanoid/network/Endpoint.hpp>

#include <sstream>
#include <utility>

namespace humanoid::network {

Endpoint::Endpoint(std::string host, std::uint16_t port, TransportProtocol protocol)
    : host_(std::move(host)), port_(port), protocol_(protocol) {}

const std::string& Endpoint::host() const noexcept { return host_; }

std::uint16_t Endpoint::port() const noexcept { return port_; }

TransportProtocol Endpoint::protocol() const noexcept { return protocol_; }

std::string Endpoint::toString() const {
  std::ostringstream stream;
  stream << humanoid::network::toString(protocol_) << "://" << host_ << ':' << port_;
  return stream.str();
}

} // namespace humanoid::network
