#pragma once

/**
 * @file RestApiCatalog.h
 * @brief Defines versioned REST API endpoint metadata.
 */

#include <string>
#include <string_view>
#include <vector>

namespace humanoid::cloud::api {

/**
 * @brief Supported REST methods in the platform catalog.
 */
enum class HttpMethod : unsigned char {
  Get,   ///< HTTP GET.
  Post,  ///< HTTP POST.
  Put,   ///< HTTP PUT.
  Patch, ///< HTTP PATCH.
  Delete ///< HTTP DELETE.
};

/**
 * @brief REST endpoint descriptor.
 */
struct RestEndpoint final {
  /** @brief Stable endpoint identifier. */
  std::string id;

  /** @brief API version, for example `v1`. */
  std::string version;

  /** @brief HTTP method. */
  HttpMethod method{HttpMethod::Get};

  /** @brief Versioned path template. */
  std::string path;

  /** @brief Framework subsystem exposed by the endpoint. */
  std::string subsystem;

  /** @brief True when authentication is required. */
  bool requiresAuthentication{true};
};

/**
 * @brief Provides the static REST endpoint catalog for downstream HTTP adapters.
 */
class RestApiCatalog final {
public:
  /**
   * @brief Constructs the default versioned endpoint catalog.
   */
  RestApiCatalog();

  /**
   * @brief Returns the configured REST endpoints.
   *
   * @return Endpoint list.
   */
  [[nodiscard]] const std::vector<RestEndpoint>& Endpoints() const noexcept;

  /**
   * @brief Finds endpoints for one framework subsystem.
   *
   * @param subsystem Subsystem name.
   * @return Matching endpoints.
   */
  [[nodiscard]] std::vector<RestEndpoint> FindBySubsystem(std::string_view subsystem) const;

private:
  std::vector<RestEndpoint> endpoints_;
};

} // namespace humanoid::cloud::api
