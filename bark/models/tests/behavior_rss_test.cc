// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "boost/variant.hpp"
#include "gtest/gtest.h"

#include "bark/commons/params/setter_params.hpp"
#include "bark/models/behavior/behavior_rss/behavior_rss.hpp"
#include "bark/models/behavior/behavior_safety/behavior_safety.hpp"
#include "bark/models/behavior/idm/idm_classic.hpp"
#include "bark/models/behavior/idm/idm_lane_tracking.hpp"
#include "bark/world/evaluation/base_evaluator.hpp"
#include "bark/world/evaluation/rss/evaluator_rss.hpp"
#include "bark/world/observed_world.hpp"
#include "bark/world/tests/make_test_world.hpp"

using bark::commons::SetterParams;
using bark::models::behavior::BehaviorIDMClassic;
using bark::models::behavior::BehaviorIDMLaneTracking;
using bark::models::behavior::BehaviorModelPtr;
using bark::models::behavior::BehaviorRSSConformant;
using bark::models::behavior::BehaviorRSSConformantStatus;
using bark::models::behavior::BehaviorSafety;
using bark::models::behavior::BehaviorStatus;
using bark::world::Agent;
using bark::world::ObservedWorld;
using bark::world::World;
using bark::world::WorldPtr;
using bark::world::evaluation::BaseEvaluator;
using bark::world::evaluation::EvaluationReturn;
using bark::world::evaluation::EvaluatorRSS;
using bark::world::tests::make_test_world;

class DummyRSSEvaluator : public BaseEvaluator {
  // return false if unsafe
 public:
  DummyRSSEvaluator(int step_trigger)
      : step_count_(0), step_trigger_(step_trigger) {}
  virtual EvaluationReturn Evaluate(const World& world) {
    step_count_++;
    return std::optional<bool>{step_count_ >= step_trigger_};
  }
  virtual EvaluationReturn Evaluate(const ObservedWorld& observed_world) {
    step_count_++;
    return std::optional<bool>{step_count_ >= step_trigger_};
  }

 private:
  int step_count_;
  int step_trigger_;
};

TEST(safe_behavior, init) {
  auto params = std::make_shared<SetterParams>();
  auto behavior_lane_tracking =
      std::make_shared<BehaviorIDMLaneTracking>(params);
  auto behavior_safety = BehaviorSafety(params);
  behavior_safety.SetBehaviorModel(behavior_lane_tracking);
}

TEST(behavior_rss, init) {
  // safety behavior
  auto params = std::make_shared<SetterParams>();
  auto behavior_lane_tracking =
      std::make_shared<BehaviorIDMLaneTracking>(params);
  std::shared_ptr<BehaviorSafety> behavior_safety =
      std::make_shared<BehaviorSafety>(params);
  behavior_safety->SetBehaviorModel(behavior_lane_tracking);

  // rss behavior
  auto behavior_rss = BehaviorRSSConformant(params);
  auto behavior_idm_classic = std::make_shared<BehaviorIDMClassic>(params);
  behavior_rss.SetNominalBehaviorModel(behavior_idm_classic);
  behavior_rss.SetSafetyBehaviorModel(behavior_safety);

  // set (dummy) RSS evaluator
  std::shared_ptr<BaseEvaluator> rss_eval =
      std::make_shared<DummyRSSEvaluator>(4);
  behavior_rss.SetEvaluator(rss_eval);

  auto behavior_safety_model = behavior_rss.GetBehaviorSafetyModel();
  auto safety_params = behavior_safety_model->GetBehaviorSafetyParams();
}

void FwSim(int steps, const WorldPtr& world, double dt = 0.2) {
  for (int i = 0; i < steps; i++) {
    world->Step(dt);
    auto ego_agent = world->GetAgents().begin()->second;
    VLOG(4) << "i= " << i << ", " << ego_agent->GetCurrentState();
  }
}

TEST(behavior_rss, behavior_rss_system_test) {
  // Test-strategy: both time the ego agent is controlled by the
  // BehaviorIDMLaneTracking and attempts to change from the right to the
  // left lane (by setting the target corridor). One time the dummy rss
  // evaluator intervenes and the lane-change is aborted.
  auto params = std::make_shared<SetterParams>();

  // First case, we start with the desired velocity. After num steps, we should
  // advance
  float ego_velocity = 0.0, rel_distance = 7.0, velocity_difference = 0.0;
  float time_step = 0.2;

  // should place an agent on the left lane (-1.75)
  WorldPtr world =
      make_test_world(0, rel_distance, ego_velocity, velocity_difference);

  // two worlds
  auto world_nominal = world;
  auto world_rss_triggered = world->Clone();

  // nominal
  auto ego_agent = world->GetAgents().begin()->second;
  std::cout << ego_agent->GetCurrentState() << std::endl;
  std::shared_ptr<BehaviorRSSConformant> behavior_rss =
      std::make_shared<BehaviorRSSConformant>(params);
  ego_agent->SetBehaviorModel(behavior_rss);
  std::shared_ptr<BaseEvaluator> rss_eval_do_not_trigger =
      std::make_shared<DummyRSSEvaluator>(0);
  behavior_rss->SetEvaluator(rss_eval_do_not_trigger);
  FwSim(20, world_nominal);
  ASSERT_TRUE(behavior_rss->GetBehaviorRssStatus() ==
              BehaviorRSSConformantStatus::NOMINAL_BEHAVIOR);
  // if we perform the lane change we switch lanes to y=-5.25
  std::cout << ego_agent->GetCurrentState() << std::endl;
  ASSERT_TRUE(ego_agent->GetCurrentState()[2] < -3.5);

  // triggered
  auto ego_agent_triggered = world_rss_triggered->GetAgents().begin()->second;
  std::cout << ego_agent_triggered->GetCurrentState() << std::endl;
  std::shared_ptr<BehaviorRSSConformant> rss_triggered_behavior =
      std::make_shared<BehaviorRSSConformant>(params);
  std::shared_ptr<BaseEvaluator> rss_eval_trigger =
      std::make_shared<DummyRSSEvaluator>(21);
  rss_triggered_behavior->SetEvaluator(rss_eval_trigger);
  ego_agent_triggered->SetBehaviorModel(rss_triggered_behavior);
  FwSim(20, world_rss_triggered);
  ASSERT_TRUE(rss_triggered_behavior->GetBehaviorRssStatus() ==
              BehaviorRSSConformantStatus::SAFETY_BEHAVIOR);
  // if we do not perform the lane change we stay on the lane y=-1.75
  std::cout << ego_agent_triggered->GetCurrentState() << std::endl;
  ASSERT_TRUE(ego_agent_triggered->GetCurrentState()[2] > -3.5);

  // assert the velocity has been set to zero
  auto behavior_safety_model = rss_triggered_behavior->GetBehaviorSafetyModel();
  auto safety_params = behavior_safety_model->GetBehaviorSafetyParams();
  // EXPECT_NEAR(
  //   safety_params->GetReal("BehaviorIDMClassic::DesiredVelocity", "", -1.),
  //   1, 0.1);

  // assert that the velocity of the triggered agent is lower
  // ASSERT_TRUE(ego_agent_triggered->GetCurrentState()[4] <
  // ego_agent->GetCurrentState()[4]);
}

TEST(behavior_rss, safety_corridor_length_test) {
  auto params = std::make_shared<SetterParams>();
  params->SetReal("BehaviorRSSConformant::MinimumSafetyCorridorLength", 180.0f);

  // First case, we start with the desired velocity. After num steps, we should
  // advance
  float ego_velocity, rel_distance = 7.0, velocity_difference = 0.0;
  float time_step = 0.2f;

  // should place an agent on the left lane (-1.75)
  WorldPtr world =
      make_test_world(0, rel_distance, ego_velocity, velocity_difference);
  auto ego_agent = world->GetAgents().begin()->second;
  auto world_nominal = world;

  // model
  std::shared_ptr<BehaviorRSSConformant> behavior_rss =
      std::make_shared<BehaviorRSSConformant>(params);
  ego_agent->SetBehaviorModel(behavior_rss);
  std::shared_ptr<BaseEvaluator> rss_eval_do_not_trigger =
      std::make_shared<DummyRSSEvaluator>(0);
  behavior_rss->SetEvaluator(rss_eval_do_not_trigger);

  // simulate
  FwSim(1, world_nominal);
  auto safety_corr1 =
      behavior_rss->GetBehaviorSafetyModel()->GetInitialLaneCorridor();
  FwSim(1, world_nominal);
  auto safety_corr2 =
      behavior_rss->GetBehaviorSafetyModel()->GetInitialLaneCorridor();
  EXPECT_EQ(safety_corr1, safety_corr2);  // not triggered -> equal

  FwSim(10, world_nominal);
  auto lcp = std::make_shared<bark::world::map::LaneCorridor>(
      bark::world::map::LaneCorridor());
  behavior_rss->GetBehaviorSafetyModel()->SetInitialLaneCorridor(lcp);
  // trigger violation:
  auto safety_corr3 =
      behavior_rss->GetBehaviorSafetyModel()->GetInitialLaneCorridor();
  EXPECT_NE(safety_corr1, safety_corr3);

  // we are now below MinimumSafetyCorridorLength, the lane corridor is re-set
  // in model:
  FwSim(10, world_nominal);
  auto safety_corr4 =
      behavior_rss->GetBehaviorSafetyModel()->GetInitialLaneCorridor();
  EXPECT_EQ(safety_corr1, safety_corr4);
}

TEST(behavior_rss, real_rss_evaluator) {
  // safety behavior
  auto params = std::make_shared<SetterParams>();
  auto behavior_lane_tracking =
      std::make_shared<BehaviorIDMLaneTracking>(params);
  std::shared_ptr<BehaviorSafety> behavior_safety =
      std::make_shared<BehaviorSafety>(params);
  behavior_safety->SetBehaviorModel(behavior_lane_tracking);

  // rss behavior
  auto behavior_rss = BehaviorRSSConformant(params);
  auto behavior_idm_classic = std::make_shared<BehaviorIDMClassic>(params);
  behavior_rss.SetNominalBehaviorModel(behavior_idm_classic);
  behavior_rss.SetSafetyBehaviorModel(behavior_safety);

  // set real RSS evaluator
  // note: if the RSS should be fully built the flag --define rss=true has to be
  // sets
  auto eval_rss = std::make_shared<EvaluatorRSS>();
  behavior_rss.SetEvaluator(eval_rss);

  auto behavior_safety_model = behavior_rss.GetBehaviorSafetyModel();
  auto safety_params = behavior_safety_model->GetBehaviorSafetyParams();
}

int main(int argc, char** argv) {
  // FLAGS_v = 5;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}