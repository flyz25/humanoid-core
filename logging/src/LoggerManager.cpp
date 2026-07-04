#include <humanoid/logging/LoggerManager.hpp>

#include <algorithm>
#include <utility>

namespace humanoid::logging {
namespace {

/**
 * @brief Returns a comparable numeric rank for a log level.
 *
 * @param level Log level to rank.
 * @return Severity rank.
 */
int severityRank(LogLevel level) noexcept {
  switch (level) {
  case LogLevel::kTrace:
    return 0;
  case LogLevel::kDebug:
    return 1;
  case LogLevel::kInfo:
    return 2;
  case LogLevel::kWarning:
    return 3;
  case LogLevel::kError:
    return 4;
  case LogLevel::kCritical:
    return 5;
  }

  return 0;
}

} // namespace

LoggerManager::LoggerManager(LogLevel minimum_level) : minimum_level_(minimum_level) {}

common::Status LoggerManager::addSink(std::shared_ptr<LogSink> sink) {
  if (!sink) {
    return common::Status::error(common::StatusCode::kInvalidArgument, "log sink is null");
  }

  sinks_.push_back(std::move(sink));
  return common::Status::ok();
}

void LoggerManager::clearSinks() noexcept { sinks_.clear(); }

std::size_t LoggerManager::sinkCount() const noexcept { return sinks_.size(); }

void LoggerManager::setMinimumLevel(LogLevel level) noexcept { minimum_level_ = level; }

LogLevel LoggerManager::minimumLevel() const noexcept { return minimum_level_; }

common::Status LoggerManager::log(const LogMessage& message) {
  if (!passesMinimumLevel(message.level())) {
    return common::Status::ok();
  }

  common::Status first_error = common::Status::ok();
  for (const auto& sink : sinks_) {
    if (sink && sink->accepts(message.level())) {
      const common::Status status = sink->write(message);
      if (!status.isOk() && first_error.isOk()) {
        first_error = status;
      }
    }
  }

  return first_error;
}

bool LoggerManager::isEnabled(LogLevel level) const noexcept {
  if (!passesMinimumLevel(level)) {
    return false;
  }

  return std::any_of(sinks_.begin(), sinks_.end(), [level](const std::shared_ptr<LogSink>& sink) {
    return sink && sink->accepts(level);
  });
}

bool LoggerManager::passesMinimumLevel(LogLevel level) const noexcept {
  return severityRank(level) >= severityRank(minimum_level_);
}

} // namespace humanoid::logging
