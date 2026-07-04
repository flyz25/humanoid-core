#include <humanoid/logging/LogMessage.hpp>

#include <utility>

namespace humanoid::logging {

LogMessage::LogMessage(LogLevel level, std::string component, std::string text,
                       std::chrono::system_clock::time_point timestamp)
    : level_(level), component_(std::move(component)), text_(std::move(text)),
      timestamp_(timestamp) {}

LogLevel LogMessage::level() const noexcept { return level_; }

const std::string& LogMessage::component() const noexcept { return component_; }

const std::string& LogMessage::text() const noexcept { return text_; }

std::chrono::system_clock::time_point LogMessage::timestamp() const noexcept { return timestamp_; }

} // namespace humanoid::logging
