
// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef BARK_WORLD_EVALUATION_SAFE_DISTANCES_EVALUATOR_SAFE_DIST_BASE_
#define BARK_WORLD_EVALUATION_SAFE_DISTANCES_EVALUATOR_SAFE_DIST_BASE_

#include "bark/world/evaluation/base_evaluator.hpp"

namespace bark {
namespace world {
class World;
namespace evaluation {

class EvaluatorSafeDistBase : public BaseEvaluator {
 public:
  EvaluatorSafeDistBase() : violation_count_(0) {};
  EvaluationReturn Evaluate(const world::ObservedWorld& observed_world) override {
    const auto safe_dist_check = CheckSafeDistance(observed_world);
    violation_count_ += int(safe_dist_check);
    return violation_count_;
  };

  virtual bool CheckSafeDistance(const world::ObservedWorld& observed_world) const = 0;
 private:
  int violation_count_;
};

}  // namespace evaluation
}  // namespace world
}  // namespace bark

#endif  // BARK_WORLD_EVALUATION_SAFE_DISTANCES_EVALUATOR_SAFE_DIST_BASE_