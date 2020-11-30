// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef BARK_MODELS_BEHAVIOR_RSS_BEHAVIOR_BEHAVIOR_RSS_HPP_
#define BARK_MODELS_BEHAVIOR_RSS_BEHAVIOR_BEHAVIOR_RSS_HPP_

#include <memory>
#include <utility>

#include "bark/models/behavior/behavior_model.hpp"
#include "bark/models/behavior/behavior_safety/behavior_safety.hpp"
#include "bark/models/behavior/idm/idm_classic.hpp"
#ifdef RSS
#include "bark/world/evaluation/rss/evaluator_rss.hpp"
#endif
#include "bark/world/evaluation/rss/safety_polygon.hpp"
#include "bark/world/world.hpp"

namespace bark {
namespace models {
namespace behavior {

using bark::models::behavior::BehaviorIDMClassic;
using bark::models::behavior::BehaviorSafety;
using bark::world::map::LaneCorridor;
using bark::world::map::LaneCorridorPtr;
using dynamic::Trajectory;
using world::ObservedWorld;
using world::evaluation::BaseEvaluator;
using world::objects::AgentId;
using world::evaluation::SafetyPolygon;
#ifdef RSS
using bark::world::evaluation::EvaluatorRSS;
#endif
enum class BehaviorRSSConformantStatus { SAFETY_BEHAVIOR, NOMINAL_BEHAVIOR };

template <typename Enumeration>
auto as_integer(Enumeration const value) {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

class BehaviorRSSConformant : public BehaviorModel {
 public:
  explicit BehaviorRSSConformant(const commons::ParamsPtr& params)
      : BehaviorModel(params),
        nominal_behavior_model_(
            std::make_shared<BehaviorIDMLaneTracking>(
                GetParams()->AddChild("NominalBehavior"))),
        behavior_safety_model_(std::make_shared<BehaviorSafety>(
              GetParams())),
        rss_evaluator_(),
        behavior_rss_status_(BehaviorRSSConformantStatus::NOMINAL_BEHAVIOR),
        world_time_of_last_rss_violation_(-1),
        initial_lane_corr_(nullptr),
        minimum_safety_corridor_length_(GetParams()->GetReal(
            "MinimumSafetyCorridorLength",
            "Minimal lenght a safety corridor should have that a lateral "
            "safety maneuver is performed.",
            0.f)) {
    try {
#ifdef RSS
      rss_evaluator_ = std::make_shared<EvaluatorRSS>(GetParams());
#endif
    } catch (...) {
      VLOG(4) << "Could not load RSSEvaluator." << std::endl;
    }
  }

  virtual ~BehaviorRSSConformant() {}

  Trajectory Plan(double min_planning_time, const ObservedWorld& observed_world);

  virtual std::shared_ptr<BehaviorModel> Clone() const;

  // setter and getter
  std::shared_ptr<BehaviorModel> GetNominalBehaviorModel() const {
    return nominal_behavior_model_;
  }
  
  BehaviorRSSConformantStatus GetBehaviorRssStatus() const { 
    return behavior_rss_status_; 
  }

  void SetNominalBehaviorModel(const std::shared_ptr<BehaviorModel>& model){
    nominal_behavior_model_ = model;
  }
  std::shared_ptr<BehaviorSafety> GetBehaviorSafetyModel() const {
    return behavior_safety_model_;
  }
  void SetSafetyBehaviorModel(const std::shared_ptr<BehaviorSafety>& model) {
    behavior_safety_model_ = model;
  }

  void SetEvaluator(const std::shared_ptr<BaseEvaluator>& evaluator) {
    rss_evaluator_ = evaluator;
  }

  #ifdef RSS
  void ApplyRestrictionsToNominalModel(const ::ad::rss::state::AccelerationRestriction& acc_restrictions);

  int32_t GetLongitudinalResponse() const { return as_integer(lon_response_); }
  int32_t GetLateralLeftResponse() const {
    return as_integer(lat_left_response_);
  }
  int32_t GetLateralRightResponse() const {
    return as_integer(lat_right_response_);
  }
  void SetLongitudinalResponse(int32_t lon) {
    lon_response_ = static_cast<::ad::rss::state::LongitudinalResponse>(lon);
  }
  void SetLateralLeftResponse(int32_t lat_left) {
    lat_left_response_ =
        static_cast<::ad::rss::state::LateralResponse>(lat_left);
  }
  void SetLateralRightResponse(int32_t lat_right) {
    lat_right_response_ =
        static_cast<::ad::rss::state::LateralResponse>(lat_right);
  }
#endif
  std::vector<SafetyPolygon> GetSafetyPolygons() const {
    return safety_polygons_;
  }
  void SetSafetyPolygons(const std::vector<SafetyPolygon>& sp) {
    safety_polygons_ = sp;
  }
 private:
  std::shared_ptr<BehaviorModel> nominal_behavior_model_;
  std::shared_ptr<BehaviorSafety> behavior_safety_model_;
  std::shared_ptr<BaseEvaluator> rss_evaluator_;
  BehaviorRSSConformantStatus behavior_rss_status_;
  double world_time_of_last_rss_violation_;
  LaneCorridorPtr initial_lane_corr_;
  float minimum_safety_corridor_length_;
#ifdef RSS
  ::ad::rss::state::LongitudinalResponse lon_response_;
  ::ad::rss::state::LateralResponse lat_left_response_;
  ::ad::rss::state::LateralResponse lat_right_response_;
  ::ad::rss::state::AccelerationRestriction acc_restrictions_;
  std::vector<SafetyPolygon> safety_polygons_;
  #endif
};

inline std::shared_ptr<BehaviorModel> BehaviorRSSConformant::Clone() const {
  std::shared_ptr<BehaviorRSSConformant> model_ptr =
      std::make_shared<BehaviorRSSConformant>(*this);
  return model_ptr;
}

}  // namespace behavior
}  // namespace models
}  // namespace bark

#endif  // BARK_MODELS_BEHAVIOR_RSS_BEHAVIOR_BEHAVIOR_RSS_HPP_
