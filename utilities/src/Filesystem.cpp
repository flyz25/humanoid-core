#include <humanoid/utilities/Filesystem.hpp>

#include <system_error>

namespace humanoid::utilities {

common::Status ensureDirectoryExists(const std::filesystem::path& path) {
  if (path.empty()) {
    return common::Status::error(common::StatusCode::kInvalidArgument, "directory path is empty");
  }

  std::error_code error;
  if (std::filesystem::exists(path, error)) {
    if (std::filesystem::is_directory(path, error)) {
      return common::Status::ok();
    }

    return common::Status::error(common::StatusCode::kFailedPrecondition,
                                 "path exists and is not a directory: " + path.string());
  }

  if (error) {
    return common::Status::error(common::StatusCode::kUnavailable,
                                 "failed to inspect directory path: " + path.string());
  }

  if (std::filesystem::create_directories(path, error)) {
    return common::Status::ok();
  }

  if (error) {
    return common::Status::error(common::StatusCode::kUnavailable,
                                 "failed to create directory: " + path.string());
  }

  return common::Status::ok();
}

common::Status requireRegularFile(const std::filesystem::path& path) {
  if (path.empty()) {
    return common::Status::error(common::StatusCode::kInvalidArgument, "file path is empty");
  }

  std::error_code error;
  if (!std::filesystem::exists(path, error)) {
    return common::Status::error(common::StatusCode::kUnavailable,
                                 "file does not exist: " + path.string());
  }

  if (error) {
    return common::Status::error(common::StatusCode::kUnavailable,
                                 "failed to inspect file path: " + path.string());
  }

  if (!std::filesystem::is_regular_file(path, error)) {
    return common::Status::error(common::StatusCode::kFailedPrecondition,
                                 "path is not a regular file: " + path.string());
  }

  if (error) {
    return common::Status::error(common::StatusCode::kUnavailable,
                                 "failed to inspect file type: " + path.string());
  }

  return common::Status::ok();
}

} // namespace humanoid::utilities
