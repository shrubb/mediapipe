#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/port/status.h"

namespace mediapipe {

namespace {

constexpr char kDetectionsTag[] = "DETECTIONS";
constexpr char kDetectionListTag[] = "DETECTION_LIST";

}  // namespace

// Remove all detections but (the first) one, and hard-assign that the id of 1.
//
// Note that the calculator will consume the input vector of Detection or
// DetectionList. So the input stream can not be connected to other calculators.
//
// Example config:
// node {
//   calculator: "SingleDetectionCalculator"
//   input_stream: "DETECTIONS:detections"
//   output_stream: "DETECTIONS:output_detections"
// }
class SingleDetectionCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(cc->Inputs().HasTag(kDetectionListTag) ||
              cc->Inputs().HasTag(kDetectionsTag))
        << "None of the input streams are provided.";

    if (cc->Inputs().HasTag(kDetectionListTag)) {
      RET_CHECK(cc->Outputs().HasTag(kDetectionListTag));
      cc->Inputs().Tag(kDetectionListTag).Set<DetectionList>();
      cc->Outputs().Tag(kDetectionListTag).Set<DetectionList>();
    }
    if (cc->Inputs().HasTag(kDetectionsTag)) {
      RET_CHECK(cc->Outputs().HasTag(kDetectionsTag));
      cc->Inputs().Tag(kDetectionsTag).Set<std::vector<Detection>>();
      cc->Outputs().Tag(kDetectionsTag).Set<std::vector<Detection>>();
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) override {
    cc->SetOffset(::mediapipe::TimestampDiff(0));
    return ::mediapipe::OkStatus();
  }
  ::mediapipe::Status Process(CalculatorContext* cc) override;
};
REGISTER_CALCULATOR(SingleDetectionCalculator);

::mediapipe::Status SingleDetectionCalculator::Process(
    CalculatorContext* cc) {
  if (cc->Inputs().HasTag(kDetectionListTag) &&
      !cc->Inputs().Tag(kDetectionListTag).IsEmpty()) {
    auto result =
        cc->Inputs().Tag(kDetectionListTag).Value().Consume<DetectionList>();
    if (result.ok()) {
      auto detection_list = std::move(result).ValueOrDie();

      // TODO a more elegant solution
      // Remove all detections but the first one, and pretend it's the object id 1
      while (detection_list->detection().size() > 1) {
        detection_list->mutable_detection()->RemoveLast();
      }
      if (!detection_list->detection().empty()) {
        detection_list->mutable_detection()->at(0).set_detection_id(1);
      }

      cc->Outputs()
          .Tag(kDetectionListTag)
          .Add(detection_list.release(), cc->InputTimestamp());
    }
  }

  if (cc->Inputs().HasTag(kDetectionsTag) &&
      !cc->Inputs().Tag(kDetectionsTag).IsEmpty()) {
    auto result = cc->Inputs()
                      .Tag(kDetectionsTag)
                      .Value()
                      .Consume<std::vector<Detection>>();
    if (result.ok()) {
      auto detections = std::move(result).ValueOrDie();

      // TODO a more elegant solution
      // Remove all detections but the first one, and pretend it's the object id 1
      while (detections->size() > 1) {
        detections->pop_back();
      }
      if (!detections->empty()) {
        (*detections)[0].set_detection_id(1);
      }

      cc->Outputs()
          .Tag(kDetectionsTag)
          .Add(detections.release(), cc->InputTimestamp());
    }
  }
  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe
