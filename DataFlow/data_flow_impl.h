#pragma once
#include "data_flow.h"

#include<vector>
#include<set>

#include "../qbelib.h"

/* Note: this file should be included in data_flow.h to provide template definitions */

/* definitions for DataFlowAnalysisNode class */

template <class DataType>
const int DataFlowAnalysisNode<DataType>::CNT_NOTINIT;

template <class DataType>
DataFlowAnalysisNode<DataType>::DataFlowAnalysisNode()
{
    getRollback() = {};
    getIgnoreRollback() = {};
    prev_blocks = {};
}

template <class DataType>
void DataFlowAnalysisNode<DataType>::doStep()
{
    int in_max_cnt = CNT_NOTINIT, out_max_cnt = CNT_NOTINIT;
    for (auto next : prev_blocks)
    {
        if (in_cnt < next->out_cnt)
        {
            for (const auto& byOut : next->getOut())
            {
                bool inserted = addIn({ byOut });

                if (inserted)
                {
                    if (next->out_cnt > in_max_cnt)
                        in_max_cnt = next->out_cnt;

                    inserted = forwardData({ byOut });

                    if (inserted && next->out_cnt > out_max_cnt)
                        out_max_cnt = next->out_cnt;
                }
            }
        }
    }

    bool was_notinit = (out_cnt == CNT_NOTINIT);

    if (out_max_cnt != CNT_NOTINIT)
        out_cnt = out_max_cnt;

    if (in_max_cnt != CNT_NOTINIT)
        in_cnt = in_max_cnt;

    // TODO: fix counter overflow
    if (was_notinit)
    {
        out_cnt++;
        in_cnt++;
    }
}

/* definitions for DataFlowAnalysis class */

template <class NodeType>
void DataFlowAnalysis<NodeType>::analyze() {
    auto curr = 0;
    auto stop = nodes.size();

    while (curr != stop)
    {
        auto curr_bb = nodes[curr];
        curr_bb->doStep();

        const auto& jumps = curr_bb->getRollback();
        if (jumps.size() != 0)
        {
            auto& ignored_jumps = curr_bb->getIgnoreRollback();

            bool jump = false;
            for (const auto& jump_to : jumps)
            {
                if (ignored_jumps.insert(jump_to).second && curr_bb->newerThan(nodes[jump_to]))
                {
                    jump = true;
                    curr = jump_to;
                    break;
                }
            }

            if (!jump)
                curr_bb->getIgnoreRollback().clear();
            else
                continue;
        }

        curr++;
    }
}
    
template <class NodeType>
DataFlowAnalysis<NodeType>::~DataFlowAnalysis()
{
    nodes.clear();
}