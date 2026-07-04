#include <humanoid/diagnostics/DiagnosticRecord.hpp>

#include <utility>

namespace humanoid::diagnostics {

DiagnosticRecord::DiagnosticRecord(std::string name, DiagnosticStatus status, std::string message,
                                   std::chrono::system_clock::time_point timestamp)
    : name_(std::move(name)), status_(status), message_(std::move(message)), timestamp_(timestamp) {
}

const std::string& DiagnosticRecord::name() const noexcept { return name_; }

DiagnosticStatus DiagnosticRecord::status() const noexcept { return status_; }

const std::string& DiagnosticRecord::message() const noexcept { return message_; }

std::chrono::system_clock::time_point DiagnosticRecord::timestamp() const noexcept {
  return timestamp_;
}

} // namespace humanoid::diagnostics
