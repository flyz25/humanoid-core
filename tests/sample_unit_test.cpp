#include <gtest/gtest.h>

#include <humanoid/common/Status.hpp>
#include <humanoid/logging/LoggerManager.hpp>

namespace {

TEST(StatusTest, DefaultStatusIsSuccessful) {
  const humanoid::common::Status status;

  EXPECT_TRUE(status.isOk());
  EXPECT_EQ(humanoid::common::StatusCode::kOk, status.code());
}

TEST(LoggerManagerTest, StartsWithoutSinks) {
  const humanoid::logging::LoggerManager logger;

  EXPECT_EQ(0U, logger.sinkCount());
  EXPECT_FALSE(logger.isEnabled(humanoid::logging::LogLevel::kInfo));
}

} // namespace
