// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef BARK_WORLD_EVALUATION_SAFE_DISTANCES_EVALUATOR_STATIC_SAFE_DIST_
#define BARK_WORLD_EVALUATION_SAFE_DISTANCES_EVALUATOR_STATIC_SAFE_DIST_

#include "bark/world/evaluation/base_evaluator.hpp"

namespace bark {
namespace world {
class World;
namespace evaluation {

class EvaluatorStaticSafeDist : public BaseEvaluator {
 public:
  EvaluatorStaticSafeDist(const bark::commons::ParamsPtr& params);
  EvaluationReturn Evaluate(const world::ObservedWorld& observed_world) override;
 private:
  float lateral_safety_dist_;
  float longitudinal_safety_dist_;
};

}  // namespace evaluation
}  // namespace world
}  // namespace bark

#endif  // BARK_WORLD_EVALUATION_SAFE_DISTANCES_EVALUATOR_SAFE_DIST_LONG_
