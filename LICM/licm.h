#pragma once

#include "../qbelib.h"
#include "../LoopNode/loop_node.h"
#include "../DataFlow/forward_data_flow.h"

#include <set>
#include <memory>

void runLoopInvariantCodeMotion(const std::set<std::shared_ptr<LoopNode>>& loops, Fn* func);