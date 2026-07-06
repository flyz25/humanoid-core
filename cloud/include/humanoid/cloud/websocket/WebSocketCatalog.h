#pragma once

/**
 * @file WebSocketCatalog.h
 * @brief Defines WebSocket stream metadata without depending on a WebSocket library.
 */

#include <string>
#include <vector>

namespace humanoid::cloud::websocket {

/**
 * @brief WebSocket stream descriptor.
 */
struct WebSocketStream final {
  /** @brief Stable stream identifier. */
  std::string id;

  /** @brief Stream path. */
  std::string path;

  /** @brief Stream payload family. */
  std::string payload;
};

/**
 * @brief Returns all default real-time streams.
 *
 * @return Stream descriptors.
 */
[[nodiscard]] std::vector<WebSocketStream> DefaultWebSocketStreams();

} // namespace humanoid::cloud::websocket
