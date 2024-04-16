#pragma once 

#include "../Utils/qbeutils.h"
#include "../LoopNode/loop_node.h"
#include <memory>
#include <set>
#include <map>

void add_preheaders(std::set<std::shared_ptr<LoopNode>> loops, Fn* func, std::map<Blk*, int>& prehead_idx);
void fill_preheaders(std::set<std::shared_ptr<LoopNode>> loops, Fn* func, std::map<Blk*, int>& prehead_idx);
