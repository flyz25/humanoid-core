#include <humanoid/common/Status.hpp>

#include <utility>

namespace humanoid::common {

std::string_view toString(StatusCode code) noexcept {
  switch (code) {
  case StatusCode::kOk:
    return "ok";
  case StatusCode::kCancelled:
    return "cancelled";
  case StatusCode::kInvalidArgument:
    return "invalid_argument";
  case StatusCode::kUnavailable:
    return "unavailable";
  case StatusCode::kFailedPrecondition:
    return "failed_precondition";
  case StatusCode::kInternalError:
    return "internal_error";
  }

  return "unknown";
}

Status Status::ok() { return Status{}; }

Status Status::error(StatusCode code, std::string message) {
  if (code == StatusCode::kOk) {
    return Status::ok();
  }

  return Status{code, std::move(message)};
}

Status::Status(StatusCode code, std::string message)
    : code_(code), message_(code == StatusCode::kOk ? std::string{} : std::move(message)) {}

StatusCode Status::code() const noexcept { return code_; }

bool Status::isOk() const noexcept { return code_ == StatusCode::kOk; }

const std::string& Status::message() const noexcept { return message_; }

} // namespace humanoid::common
