#pragma once
#include "forward_data_flow.h"

#include <iostream>

#include <vector>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <list>
#include <map>

#include "../qbelib.h"

/* Note: this file should be included in forward_data_flow.h to provide template definitions */

template <class NodeType>
std::vector<Blk*> 
ForwardDataFlowAnalysis<NodeType>::sortCfgNodes(const std::vector<Blk*>& blocks, 
                                                std::set<std::pair<Blk*, Blk*>>* ignored_edges)
{
    std::set<Blk*> all_blocks(blocks.begin(), blocks.end());
    Blk* start = blocks[0];

    if (blocks.size() == 0)
        return { };

    // STEP 1: marks all back edges
    std::set<std::pair<Blk*, Blk*>> to_ignore;

    std::unordered_set<Blk*> in_process = { };
    std::unordered_set<Blk*> visited = { }, all_visited = { };
    std::unordered_set<Blk*> at_stack = {start};
    std::list<Blk*> processing_stack, processing_starts = {start};

    while (processing_starts.size() != 0)
    {
        auto start = processing_starts.front();
        processing_starts.pop_front();

        processing_stack = { start };
        at_stack = { start };

        while (processing_stack.size() != 0)
        {
            auto curr_it = processing_stack.end();
            curr_it--;
            Blk* curr = *curr_it;
            in_process.insert(curr);
            bool next_added = false;

            for (auto next_node : {curr->s1, curr->s2})
            {
                if (next_node && all_blocks.find(next_node) != all_blocks.end() && !to_ignore.count({ curr, next_node }))
                {
                    if (in_process.count(next_node))
                    {
                        // this is back edge
                        to_ignore.insert({ curr, next_node });
                    }
                    else if (!visited.count(next_node) && at_stack.insert(next_node).second)
                    {
                        processing_stack.push_back(next_node);
                        next_added = true;
                    }
                    // if next_node has status visited then do nothing
                }
            }

            if (!next_added) {
                // mark as visited
                in_process.erase(curr);
                visited.insert(curr);
                processing_stack.erase(curr_it);
                at_stack.erase(curr);
            }
        }

        in_process.clear();

        all_visited.insert(visited.begin(), visited.end());
        visited.clear();
        processing_stack.clear();
        at_stack.clear();
    }

    all_visited.clear();

    // STEP 2: visit nodes and build sorted array
    std::vector<Blk*> result;

    processing_stack.push_back(start);

    in_process = { start };

    while (processing_stack.size() != 0)
    {
        bool nodes_added = false;
        for (auto block_it = processing_stack.begin(); block_it != processing_stack.end();)
        {
            // check if block could be added to result
            bool add = true;
            auto block = *block_it;
            
            Blk** end_p = block->pred + block->npred;
            for (Blk** prev_p = block->pred; prev_p != end_p; prev_p++)
            {
                Blk* prev = *prev_p;

                if (!visited.count(prev) && !to_ignore.count({ prev, block }) && all_blocks.find(prev) != all_blocks.end())
                    add = false;
            }
            if (add)
            {
                nodes_added = true;
                if (visited.insert(block).second)
                    result.push_back(block);

                in_process.erase(block);
                block_it = processing_stack.erase(block_it);
                bool stack_empty = block_it == processing_stack.end();


                for (auto next : {block->s1, block->s2})
                    if (next && !visited.count(next) && in_process.insert(next).second && 
                        all_blocks.find(next) != all_blocks.end())
                        processing_stack.push_back(next);
                
                if (stack_empty)
                    block_it = processing_stack.begin();
            }
            else
            {
                block_it++;
            }
        }

        if (!nodes_added && processing_stack.size() != 0) {
            //there is some blocks in the stack but no one can be processed
            //this code should be unreachable
            auto block = processing_stack.back();
            processing_stack.pop_back();
            in_process.erase(block);

            result.push_back(block);
            visited.insert(block);
            nodes_added = true;

            for (auto next : { block->s1, block->s2 })
                if(next && all_blocks.find(next) != all_blocks.end())
                    if (!visited.count(next) && in_process.insert(next).second)
                        processing_stack.push_back(next);
        }
    }

    if (ignored_edges)
        ignored_edges->insert(to_ignore.begin(), to_ignore.end()); // return back edges if needed

    return result;
}

// minimizes the number of blocks beween the ends of back edges
template <class NodeType>
std::vector<Blk*>
ForwardDataFlowAnalysis<NodeType>::reorderSequence(const std::vector<Blk*>& blocks,
                                                    const std::set<Blk*> back_edge_sources)
{
    std::vector<Blk*> res = { };

    auto blocks_end = blocks.rend();
    for (auto it = blocks.rbegin(); it < blocks_end; it++)
    {
        Blk* curr = *it;
        auto res_end = res.end();
        auto inserter = res.begin();
        if (back_edge_sources.count(curr) == 0)
            while (inserter < res_end && (*inserter != curr->s1 && *inserter != curr->s2))
                inserter++;

        res.insert(inserter, curr);
    }

    return res;
}

template <class NodeType>
void ForwardDataFlowAnalysis<NodeType>::fit(const std::vector<Blk*>& blocks)
{
    std::set<std::pair<Blk*, Blk*>> back_edges = {};

    bool returned = false;
    std::map<Blk*, std::set<Blk*>> back_edges_by_dst;

    auto blocks_sorted = this->sortCfgNodes(blocks, &back_edges);

    std::set<Blk*> back_edge_sources;

    for (auto& edge : back_edges)
    {
        back_edges_by_dst[edge.second].insert(edge.first);
        back_edge_sources.insert(edge.first);
    }

    back_edges.clear();

    blocks_sorted = reorderSequence(blocks_sorted, back_edge_sources);
    back_edge_sources.clear();

    this->nodes.clear();
    std::map<Blk*, std::shared_ptr<NodeType>> node_by_block;

    for (auto block : blocks_sorted)
    {
        std::shared_ptr<NodeType> node = this->createNode(block);
        this->nodes.push_back(node);
        node_by_block[block] = node;
    }

    int nodes_size = this->nodes.size();

    for (int i = 0; i < nodes_size; i++)
    {
        std::shared_ptr<NodeType> node = this->nodes[i];

        auto back_edges_by_dst_it = back_edges_by_dst.find(node->getBlock());
        if (back_edges_by_dst_it != back_edges_by_dst.end())
        {
            // This node is a dest for back edge
            for (auto source : back_edges_by_dst_it->second)
            {
                auto node_by_block_it = node_by_block.find(source);
                if (node_by_block_it != node_by_block.end())
                    node_by_block_it->second->getRollback().insert(i);
            }
        }
        
        Blk** end_p = node->getBlock()->pred + node->getBlock()->npred;
        for (Blk** prev_p = node->getBlock()->pred; prev_p != end_p; prev_p++)
        {
            Blk* prev = *prev_p;
            auto node_by_block_it = node_by_block.find(prev);
            if (node_by_block_it != node_by_block.end())
                node->getPrevBlocks().insert(node_by_block_it->second);
        }
    }
}