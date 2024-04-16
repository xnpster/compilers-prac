#include <algorithm>
#include <cstdio>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef __cplusplus
    #define export exports
    extern "C" {
        #include "qbe/all.h"
    }
    #undef export
#else
    #include <qbe/all.h>
#endif
#define DEBUG 0

#include<set>
#include<memory>

std::set<Blk*> getPrevBlks(Blk* b);
std::set<Blk*> getNextBlks(Blk* b);

class ComparableRef {
public:
    Ref real;

    ComparableRef(Ref real) : real(real) {};

    friend bool operator<(const ComparableRef& a, const ComparableRef& b) {
        return std::make_pair(a.real.type, a.real.val) < std::make_pair(b.real.type, b.real.val); 
    }
};

std::shared_ptr<ComparableRef> makeComparable(const Ref r);

class LoopNode {
public:
    std::shared_ptr<LoopNode> parent = NULL;
    std::set<std::shared_ptr<LoopNode>> nested;
    std::set<Blk*> blocks;
    Blk* header;
    std::set<Blk*> footers;

    Blk* preheader = NULL;
    std::set<std::shared_ptr<ComparableRef>> invariants;

    LoopNode(
        const std::set<Blk*>& blocks,
        Blk* header,
        const std::set<Blk*>& footers
    ):
        blocks(blocks),
        header(header),
        footers(footers)
    {}

    ~LoopNode() {}
};

std::set<std::shared_ptr<LoopNode>> getLoops(Fn* fn);


#include<vector>
#include<set>
#include<vector>
#include<set>
#include<memory>


template <class DataType>
class DataFlowAnalysisNode {
    static const int CNT_NOTINIT = 0;

    int in_cnt = CNT_NOTINIT, out_cnt = CNT_NOTINIT;

    std::set<int> rollback;
    std::set<int> ignore_rollback;

    std::set<std::shared_ptr<DataFlowAnalysisNode<DataType>>> prev_blocks;

    Blk* bb;
public:
    DataFlowAnalysisNode();

    void doStep();

    virtual DataType getIn() = 0;
    virtual DataType getOut() = 0;

    virtual bool addIn(const DataType& data) = 0;
    virtual bool addOut(const DataType& data) = 0;

    virtual bool forwardData(const DataType& data) = 0;

    bool newerThan(const std::shared_ptr<DataFlowAnalysisNode<DataType>> block) const { return out_cnt > block->in_cnt; }

    int getInCnt() { return in_cnt; }
    int getOutCnt() { return out_cnt; }

    void setInCnt(int cnt) { in_cnt = cnt; }
    void setOutCnt(int cnt) { out_cnt = cnt; }
    static int getStartCounter() { return CNT_NOTINIT; }

    std::set<int>& getRollback() { return rollback; }
    std::set<int>& getIgnoreRollback() { return ignore_rollback; }

    std::set<std::shared_ptr<DataFlowAnalysisNode<DataType>>>& getPrevBlocks() { return prev_blocks; }

    Blk* getBlock() { return bb; }
    void setBlock(Blk* b) { bb = b; }
};

template <class NodeType>
class DataFlowAnalysis {
protected:
    std::vector<std::shared_ptr<NodeType>> nodes;

    virtual std::shared_ptr<NodeType> createNode(Blk* block) = 0;
public:
    virtual void fit(const std::vector<Blk*>& blocks) = 0;
    void analyze();

    const std::vector<std::shared_ptr<NodeType>>& getNodes() { return nodes; }

    ~DataFlowAnalysis();
};


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



template <class NodeType>
class ForwardDataFlowAnalysis : public DataFlowAnalysis<NodeType> {
    std::vector<Blk*> reorderSequence(const std::vector<Blk*>& blocks,
                                      const std::set<Blk*> back_edge_sources);
    std::vector<Blk*> sortCfgNodes(const std::vector<Blk*>& blocks, 
                                   std::set<std::pair<Blk*, Blk*>>* ignored_edges);
public:
    void fit(const std::vector<Blk*>& blocks);
};



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






void runLoopInvariantCodeMotion(const std::set<std::shared_ptr<LoopNode>>& loops, Fn* func);

#include<iostream>

using namespace std;

Target T;

char debug['Z'+1];

static void init() {
    debug['A'] = 0; /* abi lowering */
    debug['C'] = 0; /* copy elimination */
    debug['F'] = 0; /* constant folding */
    debug['I'] = 0; /* instruction selection */
    debug['M'] = 0; /* memory optimization */
    debug['N'] = 0; /* ssa construction */
    debug['L'] = 0; /* liveness */
    debug['P'] = 0; /* parsing */
    debug['R'] = 0; /* reg. allocation */
    debug['S'] = 0; /* spilling */
}

static void dataHandler(Dat* dat) {
    (void) dat;
}

static void funHandler(Fn* fn) {
    fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
    fillpreds(fn);
    filluse(fn);
    ssa(fn);
    #if DEBUG
        printfn(fn, stdout);
    #endif
    auto loops = getLoops(fn);
    runLoopInvariantCodeMotion(loops, fn);

    #if DEBUG
        for(int i = 0; i < fn->nblk; i++) {
            Blk* b = fn->rpo[i];
            cout << b->id << " " << b->name << endl;
        }
    #endif

    printfn(fn, stdout);
}

static void dbgfile(char* str) {
    cout << str << endl;
}

int main(int argc, char** argv) {
#if DEBUG
    cout << "Running..." << endl;
#endif

    init();

    parse(stdin, "f", dataHandler, funHandler);
    freeall();

    return 0;
}

#define DEBUG 0

#if DEBUG
#   include <iostream>
#endif
using namespace std;

static shared_ptr<LoopNode> constructLoopNode(Blk* header, Blk* footer) {
    set<Blk*> reachable = { footer, header };
    set<Blk*> worklist = { footer };

    if(reachable.size() == 1)
        worklist = { }; // handle case with single-block loop

    while (!worklist.empty()) {
        set<Blk*> worklist_next = {};

        for(Blk* b : worklist) {
            Blk** pred_end = b->pred + b->npred;
            
            for(Blk** pred_p = b->pred; pred_p != pred_end; pred_p++) {
                Blk* pred = *pred_p;

                if(reachable.insert(pred).second) {
                    worklist_next.insert(pred);
                }
            }
        }

        worklist = worklist_next;
    }

    shared_ptr<LoopNode> created = shared_ptr<LoopNode>(new LoopNode(reachable, header, { footer} ));
#if DEBUG
    cout << "new loop node:" << endl;
    for(Blk* b : created->blocks) {
        cout << b->name << endl;
    }
    cout << endl;
#endif

    return created;
}

static void insertNodeToTree(set<shared_ptr<LoopNode>>& tree, shared_ptr<LoopNode> node) {
    bool inserted = false;

    for(auto e : tree) {
        if(e->blocks.find(node->header) != e->blocks.end()) {
            if(e->header == node->header) {
                e->blocks.insert(node->blocks.begin(), node->blocks.end());
                e->footers.insert(node->footers.begin(), node->footers.end());
            } else {
                node->parent = e;
                insertNodeToTree(e->nested, node);
            }

            inserted = true;
            break;
        }
    }

    if(!inserted) {
        for(auto it = tree.begin(); it != tree.end();) {
            if(node->blocks.find((*it)->header) != node->blocks.end()) {
                node->nested.insert(*it);
                (*it)->parent = node;
                it = tree.erase(it);
            } else {
                it++;
            }
        }

        tree.insert(node);
    }
}

/* fillpreds needed */
set<shared_ptr<LoopNode>> getLoops(Fn* fn) {
    set<shared_ptr<LoopNode>> res;

    Blk** end = fn->rpo + fn->nblk;

    //DFS
    set<Blk*> visited = { fn->start };
    set<Blk*> at_stack = { fn->start };
    list<pair<Blk*, list<Blk*>>> stack = { { fn->start , { fn->start->s1, fn->start->s2 } } };

    while(!stack.empty()) {
        auto& top_pair = stack.back();
        Blk* top = top_pair.first;

        auto& next_set = top_pair.second;

        bool step_done = false;

        while (!next_set.empty()) {
            Blk* next = next_set.back();
            next_set.pop_back();

            if(next) {
                if(at_stack.find(next) != at_stack.end()) {
                    // back edge
                    #if DEBUG
                        cout << "Loop " << next->name << " -> " << top->name << endl;
                    #endif

                    auto created = constructLoopNode(next, top);
                    insertNodeToTree(res, created);
                } else if (visited.find(next) != visited.end()) {
                    // cross edge, do nothing
                } else {
                    // go deeper

                    visited.insert(next);
                    at_stack.insert(next);
                    stack.push_back({next , { next->s1, next->s2 } });

                    step_done = true;
                    break;
                }
            }
        }

        if(!step_done) {
            at_stack.erase(top);
            stack.pop_back();
        }
    }

    return res;
}

using namespace std;

set<Blk*> getPrevBlks(Blk* b) {
    return set<Blk*> (b->pred, b->pred + b->npred);
}

set<Blk*> getNextBlks(Blk* b) {
    set<Blk*> next;
    for(auto e : {b->s1, b->s2}) {
        if(e)
            next.insert(e);
    }

    return next;
}

shared_ptr<ComparableRef> makeComparable(const Ref r) {
    static map<pair<int, int>, shared_ptr<ComparableRef>> refs;

    auto converted = make_pair(r.type, r.val);
    auto it = refs.find(converted);
    if(it != refs.end()) {
        return it->second;
    } else {
        return refs[converted] = shared_ptr<ComparableRef>(new ComparableRef(r));
    }
}

void add_preheaders(std::set<std::shared_ptr<LoopNode>> loops, Fn* func, std::map<Blk*, int>& prehead_idx);
void fill_preheaders(std::set<std::shared_ptr<LoopNode>> loops, Fn* func, std::map<Blk*, int>& prehead_idx);

#define DEBUG 0

#if DEBUG
#   include <iostream>
#endif

using namespace std;

class LoopInvariantAnalysisNode : public DataFlowAnalysisNode<set<shared_ptr<ComparableRef>>> {
    Fn* func;
    set<shared_ptr<ComparableRef>> in, out;

    map<shared_ptr<ComparableRef>, set<shared_ptr<ComparableRef>>> local_deps;
    set<shared_ptr<ComparableRef>>& invariants;
public:
    set<shared_ptr<ComparableRef>> getIn() {
        return in;
    }

    set<shared_ptr<ComparableRef>> getOut() {
        return out;
    }

    bool addIn(const set<shared_ptr<ComparableRef>>& data) {
        bool inserted = false;

        for(auto e : data)
            inserted |= in.insert(e).second;

        return inserted;
    }

    bool addOut(const set<shared_ptr<ComparableRef>>& data) {
        bool inserted = false;

        for(auto e : data)
            inserted |= out.insert(e).second;

        return inserted;
    }

    bool forwardData(const set<shared_ptr<ComparableRef>>& data) {
        set<shared_ptr<ComparableRef>> new_invs = data;
        
        for(auto it = local_deps.begin(); it != local_deps.end();) {
            for(shared_ptr<ComparableRef> e : new_invs)
                it->second.erase(e);

            if(it->second.empty()) {
                new_invs.insert(it->first);
                invariants.insert(it->first);
                it = local_deps.erase(it);
            } else {
                it++;
            }
        }

        return addOut(new_invs);
    }

    LoopInvariantAnalysisNode(Blk* block, Fn* func, set<shared_ptr<ComparableRef>>& invariants, const set<shared_ptr<ComparableRef>>& defined)
    :
        func(func),
        invariants(invariants)
    {
        setBlock(block);

        for(int i = 0; i < block->nins; i++) {
            Ins& instr = block->ins[i];
            
            auto res = makeComparable(instr.to);

            // TODO: func calls

            for(auto arg: { makeComparable(instr.arg[0]), makeComparable(instr.arg[1]) }) {
                if(!req(arg->real, R) && 
                   defined.find(arg) != defined.end() &&
                   invariants.find(arg) == invariants.end()
                )
                {
                    local_deps[res].insert(arg);
                }
            }

            if(local_deps.find(res) == local_deps.end()) {
                out.insert(res);
                invariants.insert(res);
            }
        }
    }
};

class LoopInvariantAnalysis : public ForwardDataFlowAnalysis<LoopInvariantAnalysisNode> {
protected:
    Fn* func;
    set<shared_ptr<ComparableRef>> invariants = {}, defined;

    shared_ptr<LoopInvariantAnalysisNode> createNode(Blk* block) override {
        return shared_ptr<LoopInvariantAnalysisNode>(
            new LoopInvariantAnalysisNode(block, func, invariants, defined)
        );
    }
public:
    LoopInvariantAnalysis(Fn* func, const set<shared_ptr<ComparableRef>>& defined)
    : 
        func(func), 
        defined(defined)
    { }
    set<shared_ptr<ComparableRef>> getResult() { return invariants; }
};

static set<shared_ptr<ComparableRef>> getDefined(const std::set<Blk *>& b) {
    set<shared_ptr<ComparableRef>> res = {};

    for(Blk* block: b) {
        Phi* p = block->phi;

        while (p) {
            res.insert(makeComparable(p->to));
            p = p->link;
        }

        for(int i = 0; i < block->nins; i++) {
            Ins& instr = block->ins[i];
            res.insert(makeComparable(instr.to));
        }
    }

    return res;
}

static void runLoopInvariantCodeMotionForSingleNode(shared_ptr<LoopNode> loop, Fn* func) {
    vector<Blk*> b(loop->blocks.begin(), loop->blocks.end());
    swap(*b.begin(), *find(b.begin(), b.end(), loop->header));
    #if DEBUG
        cout << "LICM for loop " << loop->header->name << " -> ";
        for(auto ft : loop->footers)
            cout << ft->name << " ";
        cout << endl;
    #endif

    auto defined_vars = getDefined(loop->blocks);

    LoopInvariantAnalysis analysis_obj(func, defined_vars);
    analysis_obj.fit(b);
    analysis_obj.analyze();

    loop->invariants = analysis_obj.getResult();

    #if DEBUG
        for(shared_ptr<ComparableRef> inv : loop->invariants) {
            printref(inv->real, func, stdout);
            fflush(stdout);
            cout << endl;
        }

        cout << endl;
    #endif
}

static void runLoopInvariantCodeMotionRec(const set<shared_ptr<LoopNode>>& loops, Fn* func) {
    for(auto e : loops) {
        runLoopInvariantCodeMotionRec(e->nested, func);
        runLoopInvariantCodeMotionForSingleNode(e, func);
    }
}

static void cleanInvariantsRec(const set<shared_ptr<LoopNode>>& loops, const set<shared_ptr<ComparableRef>>& invs) {
    for(auto e : loops) {
        set<shared_ptr<ComparableRef>> next = invs;
        next.insert(e->invariants.begin(), e->invariants.end()); 
        cleanInvariantsRec(e->nested, next);

        for(auto inv : invs)
            e->invariants.erase(inv);
    }
}

void runLoopInvariantCodeMotion(const set<shared_ptr<LoopNode>>& loops, Fn* func) {
    #if DEBUG
        cout << "Running LICM..." << endl;
    #endif
    runLoopInvariantCodeMotionRec(loops, func);
    cleanInvariantsRec(loops, {});
    map<Blk*, int> prehead_idx;

    add_preheaders(loops, func, prehead_idx);
    fill_preheaders(loops, func, prehead_idx);
}

#define DEBUG 0

#if DEBUG
#   include <iostream>
#endif

using namespace std;

static const string prefix = "prehead@";

static bool insert_block_to_func(Fn* fn, Blk* before, Blk* curr) {
    for (auto b=fn->start; b; b=b->link) {
        if(b->link == before) {
            b->link = curr;
            curr->link = before;

            return true;
        }
    }

    return false;
}

static void create_preheader(shared_ptr<LoopNode> loop, Fn* fn, 
                             map<Blk*, Blk*>& created_headers, 
                             map<Blk*, int>& prehead_idx)
{
    #if DEBUG
        cout << "add preheader to " <<  loop->header->name << endl;
    #endif

    set<Blk*> outer_preds;
    Blk** preds = loop->header->pred; 

    for(int i = 0; i < (loop->header->npred); i++)
        if(loop->blocks.find(preds[i]) == loop->blocks.end())
            outer_preds.insert(preds[i]);

    if(!loop->invariants.empty() && !outer_preds.empty()) {
        if (outer_preds.size() == 1 && false) { // TODO: remove
            // Если у хедера только один предок - он становится прехедером
            loop->preheader = *(outer_preds.begin());
            prehead_idx[loop->preheader] = loop->preheader->nins;
        } else if(created_headers.find(loop->header) != created_headers.end()) {
            loop->preheader = created_headers[loop->header];
            prehead_idx[loop->preheader] = 0;
        } else {
            // Новый прехеддер
            Blk* prehead = blknew();
            insert_block_to_func(fn, loop->header, prehead);

            loop->preheader = prehead;
            created_headers[loop->header] = prehead;
            prehead_idx[loop->preheader] = 0;
            // Прехеддер - новый предок хедера
            string newname = string(prefix + string(loop->header->name));
            strncpy(prehead->name, newname.c_str(), newname.size() + 1);
            prehead->nins = 0;
            prehead->s1 = loop->header;
            prehead->s2 = NULL;

            prehead->jmp.type = Jjmp;

            // В цикле у всех предков хедера потомком делаем прехедер вместо хедера
            for(auto pred : outer_preds) {
                if(pred->s1 == loop->header)
                    pred->s1 = prehead;

                if(pred->s2 == loop->header)
                    pred->s2 = prehead;
            }
        }
        // Alloc ins

        int result_size = loop->preheader->nins + loop->invariants.size();
        Ins* ins_mem = (Ins*)malloc(result_size * sizeof(Ins));

        if(loop->preheader->nins != 0) {
            memcpy(ins_mem, loop->preheader->ins, loop->preheader->nins * sizeof(Ins));
            //free(loop->preheader->ins);
        }

        loop->preheader->ins = ins_mem;
        loop->preheader->nins = result_size;
    }

    for(auto descend: loop->nested) {
        // Создаём прехедер для вложенных циклов
        create_preheader(descend, fn, created_headers, prehead_idx);
        
        // Добавляем прехедеры вложенных циклов в множество блоков текущего цикла
        if(descend->preheader)
            loop->blocks.insert(descend->preheader);
    }
}

void add_preheaders(set<shared_ptr<LoopNode>> loops, Fn* func, map<Blk*, int>& prehead_idx) {
    map<Blk*, Blk*> created_headers;

    for(auto loop: loops) {
        create_preheader(loop, func, created_headers, prehead_idx);
    }

    fillpreds(func);
}

static void fill_preheader(shared_ptr<LoopNode> loop, Fn* func, map<Blk*, int>& prehead_idx) {
    // Added ins
    
    if(loop->preheader) {
        int& counter = prehead_idx[loop->preheader];
        
        #if DEBUG
            cout << "fill preheader " <<  loop->preheader->name << endl;
        #endif

        for (const auto& invariant: loop->invariants) {
            #if DEBUG
                cout << "inv ";
                printref(invariant->real, func, stdout);
                cout << endl;
            #endif

            for (auto& block: loop->blocks) {
                bool added = false;
                for (int i = 0; i < block->nins; i++) {
                    if (invariant != makeComparable(block->ins[i].to)) {
                        continue;
                    }
                    
                    // Move ins from block to preheader
                    loop->preheader->ins[counter++] = block->ins[i];

                    #if DEBUG
                        cout << loop->preheader->name << "[" << counter - 1 << "/" << loop->preheader->nins - 1 << "] = ";
                        printref(block->ins[i].to, func, stdout);
                        cout << endl;
                    #endif

                    for(i++; i < block->nins; i++) {
                        block->ins[i - 1] = block->ins[i];
                    }
                    block->nins--;
                    added = true;
                    break;
                }
                if (added) {
                    break;
                }
            }
        }

        loop->preheader->nins = counter;
    }

    for (auto& nst: loop->nested) {
        fill_preheader(nst, func, prehead_idx);
    }
    
    return;
}

void fill_preheaders(set<shared_ptr<LoopNode>> loops, Fn* func, map<Blk*, int>& prehead_idx) {
    for (auto& loop: loops) {
        fill_preheader(loop, func, prehead_idx);
    }
    return;
}
