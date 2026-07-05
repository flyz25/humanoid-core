/**
 * @file perception_framework_unit_test.cpp
 * @brief Validates the completed perception framework abstractions.
 */

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/DetectionResult.h>
#include <humanoid/perception/IInferenceEngine.h>
#include <humanoid/perception/InferenceRequest.h>
#include <humanoid/perception/InferenceResult.h>
#include <humanoid/perception/ModelManager.h>
#include <humanoid/perception/PerceptionPipeline.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorFusion.h>
#include <humanoid/perception/SensorType.h>

namespace {

#define HUMANOID_REQUIRE(condition) Require((condition), #condition, __LINE__)

void Require(bool condition, const char* expression, int line) {
  if (!condition) {
    std::cerr << "Requirement failed at line " << line << ": " << expression << '\n';
    std::abort();
  }
}

[[nodiscard]] humanoid::common::Status Ok() { return humanoid::common::Status::ok(); }

[[nodiscard]] humanoid::perception::SensorFrame MakeFrame(
    humanoid::perception::SensorType type,
    humanoid::perception::SensorFrameTimestamp timestamp =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now())) {
  humanoid::perception::SensorFrame frame;
  frame.id = 1U;
  frame.sensorType = type;
  frame.timestamp = timestamp;
  frame.width = 2U;
  frame.height = 2U;
  frame.channels = 1U;
  frame.bytesPerElement = 1U;
  frame.encoding = "bytes";
  frame.data = std::vector<std::byte>{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
  return frame;
}

class DeterministicEngine final : public humanoid::perception::IInferenceEngine {
public:
  [[nodiscard]] humanoid::common::Status Initialize() override { return Ok(); }

  [[nodiscard]] humanoid::common::Status Shutdown() override { return Ok(); }

  [[nodiscard]] humanoid::perception::InferenceResult
  Run(const humanoid::perception::InferenceRequest& request) override {
    humanoid::perception::InferenceResult result;
    result.modelId = request.modelId;
    result.outputs.emplace("score", 0.95);

    humanoid::perception::DetectionResult detection;
    detection.id = "det-1";
    detection.type = humanoid::perception::DetectionType::Object;
    detection.confidence = 0.95;
    detection.label = "target";
    detection.boundingBox = humanoid::perception::BoundingBox2D{1.0, 2.0, 3.0, 4.0};
    detection.timestamp = std::chrono::steady_clock::now();
    detection.trackingId = "track-1";
    result.detections.push_back(std::move(detection));
    result.timestamp = std::chrono::steady_clock::now();
    return result;
  }

  [[nodiscard]] humanoid::perception::InferenceEngineCapabilities Capabilities() const override {
    humanoid::perception::InferenceEngineCapabilities capabilities;
    capabilities.engineId = "deterministic";
    capabilities.name = "Deterministic Engine";
    capabilities.supportedBackends = {"Custom"};
    capabilities.supportedModelFormats = {"memory"};
    return capabilities;
  }
};

class RecordingStage final : public humanoid::perception::IPerceptionStage {
public:
  explicit RecordingStage(std::string label, std::atomic<std::uint64_t>& calls)
      : label_(std::move(label)), calls_(calls) {}

  [[nodiscard]] humanoid::common::Status
  Process(humanoid::perception::PerceptionContext& context) override {
    context.metadata.emplace(label_, std::string{"executed"});
    ++calls_;
    return Ok();
  }

private:
  std::string label_;
  std::atomic<std::uint64_t>& calls_;
};

class DetectionStage final : public humanoid::perception::IPerceptionStage {
public:
  [[nodiscard]] humanoid::common::Status
  Process(humanoid::perception::PerceptionContext& context) override {
    humanoid::perception::DetectionResult detection;
    detection.id = "pipeline-detection";
    detection.type = humanoid::perception::DetectionType::Marker;
    detection.confidence = 1.0;
    detection.label = "marker";
    detection.boundingBox = humanoid::perception::BoundingBox2D{0.0, 0.0, 1.0, 1.0};
    detection.timestamp = std::chrono::steady_clock::now();
    context.detections.push_back(std::move(detection));
    return Ok();
  }
};

void VerifyModelManagerAndInference() {
  humanoid::perception::ModelManager manager;
  auto engine = std::make_shared<DeterministicEngine>();
  HUMANOID_REQUIRE(manager.RegisterEngine(engine).isOk());

  humanoid::perception::ModelDescriptor descriptor;
  descriptor.modelId = "object-detector";
  descriptor.name = "Object Detector";
  descriptor.format = "memory";
  descriptor.engineId = "deterministic";
  descriptor.version = "1";
  HUMANOID_REQUIRE(manager.RegisterModel(descriptor).isOk());
  HUMANOID_REQUIRE(manager.EnumerateEngines().size() == 1U);
  HUMANOID_REQUIRE(manager.EnumerateModels().size() == 1U);
  HUMANOID_REQUIRE(manager.FindModel("object-detector").succeeded());

  humanoid::perception::InferenceRequest request;
  request.modelId = "object-detector";
  request.inputFrame = MakeFrame(humanoid::perception::SensorType::Camera);

  humanoid::perception::InferenceResult result = manager.RunInference(request);
  HUMANOID_REQUIRE(result.succeeded());
  HUMANOID_REQUIRE(result.modelId == "object-detector");
  HUMANOID_REQUIRE(result.outputs.find("score") != result.outputs.end());
  HUMANOID_REQUIRE(result.detections.size() == 1U);
  HUMANOID_REQUIRE(result.detections.front().hasIdentity());
  HUMANOID_REQUIRE(result.detections.front().boundingBox.isValid());
  HUMANOID_REQUIRE(result.detections.front().position == std::nullopt);
}

void VerifyPipelineGraphAndConcurrentExecution() {
  humanoid::perception::PerceptionPipeline pipeline;
  std::atomic<std::uint64_t> calls{0U};

  humanoid::perception::PerceptionStageDescriptor frame_stage;
  frame_stage.stageId = "frame";
  frame_stage.name = "Frame";
  frame_stage.type = humanoid::perception::PerceptionStageType::Frame;

  humanoid::perception::PerceptionStageDescriptor filter_stage;
  filter_stage.stageId = "filter";
  filter_stage.name = "Filter";
  filter_stage.type = humanoid::perception::PerceptionStageType::Filter;
  filter_stage.dependencies = {"frame"};

  humanoid::perception::PerceptionStageDescriptor detection_stage;
  detection_stage.stageId = "detection";
  detection_stage.name = "Detection";
  detection_stage.type = humanoid::perception::PerceptionStageType::Detection;
  detection_stage.dependencies = {"filter"};

  HUMANOID_REQUIRE(
      pipeline.RegisterStage(frame_stage, std::make_shared<RecordingStage>("frame", calls)).isOk());
  HUMANOID_REQUIRE(
      pipeline.RegisterStage(filter_stage, std::make_shared<RecordingStage>("filter", calls))
          .isOk());
  HUMANOID_REQUIRE(
      pipeline.RegisterStage(detection_stage, std::make_shared<DetectionStage>()).isOk());
  HUMANOID_REQUIRE(pipeline.Size() == 3U);
  HUMANOID_REQUIRE(pipeline.DescribeGraph().size() == 3U);

  humanoid::perception::PerceptionPipelineResult result =
      pipeline.Execute(MakeFrame(humanoid::perception::SensorType::Camera));
  HUMANOID_REQUIRE(result.succeeded());
  HUMANOID_REQUIRE(result.executedStages.size() == 3U);
  HUMANOID_REQUIRE(result.executedStages[0] == "frame");
  HUMANOID_REQUIRE(result.executedStages[1] == "filter");
  HUMANOID_REQUIRE(result.executedStages[2] == "detection");
  HUMANOID_REQUIRE(result.context.detections.size() == 1U);

  constexpr std::size_t kThreadCount = 4U;
  std::vector<std::jthread> threads;
  threads.reserve(kThreadCount);
  for (std::size_t index = 0U; index < kThreadCount; ++index) {
    threads.emplace_back([&pipeline]() {
      const humanoid::perception::PerceptionPipelineResult threaded_result =
          pipeline.Execute(MakeFrame(humanoid::perception::SensorType::Camera));
      HUMANOID_REQUIRE(threaded_result.succeeded());
    });
  }
  threads.clear();
  HUMANOID_REQUIRE(calls.load() == 2U * (kThreadCount + 1U));
}

void VerifySensorFusionSynchronization() {
  const auto timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());

  humanoid::perception::FrameSynchronizationPolicy policy;
  policy.maximumSkew = std::chrono::milliseconds{5};
  policy.requiredSensorTypes = {humanoid::perception::SensorType::Camera,
                                humanoid::perception::SensorType::Imu};

  humanoid::perception::FrameSynchronizer synchronizer{policy};
  std::vector<humanoid::perception::SensorFrame> frames;
  frames.push_back(MakeFrame(humanoid::perception::SensorType::Camera, timestamp));
  frames.push_back(
      MakeFrame(humanoid::perception::SensorType::Imu, timestamp + std::chrono::milliseconds{1}));

  humanoid::perception::SynchronizedFrameSet synchronized_frames =
      synchronizer.Synchronize(std::move(frames));
  HUMANOID_REQUIRE(synchronized_frames.succeeded());
  HUMANOID_REQUIRE(synchronized_frames.frames.size() == 2U);
  HUMANOID_REQUIRE(synchronized_frames.latestTimestamp - synchronized_frames.earliestTimestamp <=
                   policy.maximumSkew);

  std::vector<humanoid::perception::SensorFrame> late_frames;
  late_frames.push_back(MakeFrame(humanoid::perception::SensorType::Camera, timestamp));
  late_frames.push_back(
      MakeFrame(humanoid::perception::SensorType::Imu, timestamp + std::chrono::milliseconds{50}));
  HUMANOID_REQUIRE(!synchronizer.Synchronize(std::move(late_frames)).succeeded());

  humanoid::perception::CoordinateFrame base_frame;
  base_frame.frameId = "base";
  humanoid::perception::CoordinateFrame camera_frame;
  camera_frame.frameId = "camera";
  camera_frame.parentFrameId = "base";
  humanoid::perception::CoordinateTransform transform;
  transform.source = camera_frame;
  transform.target = base_frame;
  transform.translationZ = 1.0;
  HUMANOID_REQUIRE(transform.source.isValid());
  HUMANOID_REQUIRE(transform.target.isValid());
}

} // namespace

int main() {
  VerifyModelManagerAndInference();
  VerifyPipelineGraphAndConcurrentExecution();
  VerifySensorFusionSynchronization();
  return EXIT_SUCCESS;
}
