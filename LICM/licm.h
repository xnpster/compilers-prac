#include "../qbelib.h"
#include "../LoopNode/loop_node.h"

#include <set>
#include <memory>

void runLoopInvariantCodeMotion(const std::set<std::shared_ptr<LoopNode>>& loops, Fn* func);