#pragma once 

#include "../Utils/qbeutils.h"
#include "../LoopNode/loop_node.h"
#include <memory>
#include <set>

void add_preheaders(std::set<std::shared_ptr<LoopNode>> loops, Fn* func);