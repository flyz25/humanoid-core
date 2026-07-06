#include <humanoid/cloud/websocket/WebSocketCatalog.h>

#include <cstdlib>
#include <iostream>

int main() {
  const auto streams = humanoid::cloud::websocket::DefaultWebSocketStreams();
  if (streams.empty()) {
    return EXIT_FAILURE;
  }

  std::cout << "WebSocket streams: " << streams.size() << '\n';
  return EXIT_SUCCESS;
}
