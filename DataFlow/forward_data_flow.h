#pragma once
#include "data_flow.h"

#include <vector>
#include <set>
#include <algorithm>

#include "../qbelib.h"

template <class NodeType>
class ForwardDataFlowAnalysis : public DataFlowAnalysis<NodeType> {
    std::vector<Blk*> reorderSequence(const std::vector<Blk*>& blocks,
                                      const std::set<Blk*> back_edge_sources);
    std::vector<Blk*> sortCfgNodes(const std::vector<Blk*>& blocks, 
                                   std::set<std::pair<Blk*, Blk*>>* ignored_edges);
public:
    void fit(const std::vector<Blk*>& blocks);
};

#include "forward_data_flow_impl.h"