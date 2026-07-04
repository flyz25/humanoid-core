#include <humanoid/common/Version.hpp>

#include <sstream>

namespace humanoid::common {

std::string SemanticVersion::toString() const {
  std::ostringstream stream;
  stream << major_version_ << '.' << minor_version_ << '.' << patch_version_;
  return stream.str();
}

} // namespace humanoid::common
