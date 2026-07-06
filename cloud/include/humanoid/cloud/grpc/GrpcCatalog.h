#pragma once

/**
 * @file GrpcCatalog.h
 * @brief Defines optional gRPC service metadata without depending on gRPC.
 */

#include <string>
#include <vector>

namespace humanoid::cloud::grpc {

/**
 * @brief gRPC method descriptor used by downstream gRPC adapters.
 */
struct GrpcMethod final {
  /** @brief Service name. */
  std::string service;

  /** @brief Method name. */
  std::string method;

  /** @brief True when the method is a streaming method. */
  bool streaming{false};
};

/**
 * @brief Returns the optional gRPC method catalog.
 *
 * @return gRPC method descriptors.
 */
[[nodiscard]] std::vector<GrpcMethod> DefaultGrpcCatalog();

} // namespace humanoid::cloud::grpc
