/**
 * @file main.cpp
 * @brief Demonstrates a configurable perception pipeline.
 */

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <humanoid/perception/DetectionResult.h>
#include <humanoid/perception/PerceptionPipeline.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorType.h>

namespace {

class MetadataStage final : public humanoid::perception::IPerceptionStage {
public:
  explicit MetadataStage(std::string key) : key_(std::move(key)) {}

  [[nodiscard]] humanoid::common::Status
  Process(humanoid::perception::PerceptionContext& context) override {
    context.metadata.emplace(key_, std::string{"ok"});
    return humanoid::common::Status::ok();
  }

private:
  std::string key_;
};

class DetectionStage final : public humanoid::perception::IPerceptionStage {
public:
  [[nodiscard]] humanoid::common::Status
  Process(humanoid::perception::PerceptionContext& context) override {
    humanoid::perception::DetectionResult detection;
    detection.id = "marker-1";
    detection.type = humanoid::perception::DetectionType::Marker;
    detection.confidence = 1.0;
    detection.label = "example-marker";
    detection.boundingBox = humanoid::perception::BoundingBox2D{0.0, 0.0, 1.0, 1.0};
    detection.timestamp = std::chrono::steady_clock::now();
    context.detections.push_back(std::move(detection));
    return humanoid::common::Status::ok();
  }
};

} // namespace

int main() {
  humanoid::perception::PerceptionPipeline pipeline;

  humanoid::perception::PerceptionStageDescriptor frame_stage;
  frame_stage.stageId = "frame";
  frame_stage.name = "Frame";
  frame_stage.type = humanoid::perception::PerceptionStageType::Frame;

  humanoid::perception::PerceptionStageDescriptor detection_stage;
  detection_stage.stageId = "detection";
  detection_stage.name = "Detection";
  detection_stage.type = humanoid::perception::PerceptionStageType::Detection;
  detection_stage.dependencies = {"frame"};

  if (!pipeline.RegisterStage(frame_stage, std::make_shared<MetadataStage>("frame")).isOk() ||
      !pipeline.RegisterStage(detection_stage, std::make_shared<DetectionStage>()).isOk()) {
    return 1;
  }

  humanoid::perception::SensorFrame frame;
  frame.id = 1U;
  frame.sensorType = humanoid::perception::SensorType::Camera;
  frame.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  frame.data = std::vector<std::byte>{std::byte{1}};

  const humanoid::perception::PerceptionPipelineResult result = pipeline.Execute(std::move(frame));
  if (!result.succeeded()) {
    return 1;
  }

  std::cout << "Pipeline stages=" << result.executedStages.size()
            << " detections=" << result.context.detections.size() << '\n';
  return 0;
}
