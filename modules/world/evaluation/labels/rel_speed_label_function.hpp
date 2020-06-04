// Copyright (c) 2019 fortiss GmbH
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef MODULES_WORLD_EVALUATION_LABELS_REL_SPEED_LABEL_FUNCTION_HPP_
#define MODULES_WORLD_EVALUATION_LABELS_REL_SPEED_LABEL_FUNCTION_HPP_

#include <string>

#include "modules/world/evaluation/labels/multi_agent_label_function.hpp"
#include "modules/world/objects/object.hpp"

namespace modules {
namespace world {
namespace evaluation {

class RelSpeedLabelFunction : public MultiAgentLabelFunction {
 public:
  RelSpeedLabelFunction(const std::string& string, double rel_speed_thres);
  bool EvaluateAgent(const world::ObservedWorld& observed_world,
                     const AgentPtr& other_agent) const override;

 private:
  const double rel_speed_thres_;
};

}  // namespace evaluation
}  // namespace world
}  // namespace modules

#endif  // MODULES_WORLD_EVALUATION_LABELS_REL_SPEED_LABEL_FUNCTION_HPP_
