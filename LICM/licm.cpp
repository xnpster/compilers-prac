#include "licm.h"
#include "../DataFlow/forward_data_flow.h"

#include <iostream>
#include <map>
#include <set>
#include <memory>

using namespace std;

class LoopInvariantAnalysisNode : public DataFlowAnalysisNode<set<Ref*>> {
    Fn* func;
    set<Ref*> in, out;

    map<Ref*, set<Ref*>> local_deps;
    set<Ref*>& invariants;
public:
    set<Ref*> getIn() {
        return in;
    }

    set<Ref*> getOut() {
        return out;
    }

    bool addIn(const set<Ref*>& data) {
        bool inserted = false;

        for(auto e : data)
            inserted |= in.insert(e).second;

        return inserted;
    }

    bool addOut(const set<Ref*>& data) {
        bool inserted = false;

        for(auto e : data)
            inserted |= out.insert(e).second;

        return inserted;
    }

    bool forwardData(const set<Ref*>& data) {
        set<Ref*> new_invs = data;
        
        for(auto it = local_deps.begin(); it != local_deps.end();) {
            for(Ref* e : new_invs)
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

    LoopInvariantAnalysisNode(Blk* block, Fn* func, set<Ref*>& invariants, const set<Ref*>& defined)
    :
        func(func),
        invariants(invariants)
    {
        setBlock(block);

        for(int i = 0; i < block->nins; i++) {
            Ins& instr = block->ins[i];

            // TODO: func calls

            for(Ref* arg: {&instr.arg[0], &instr.arg[1]}) {
                if(arg && !req(*arg, R) && arg->type != RInt && 
                   defined.find(arg) != defined.end() &&
                   invariants.find(arg) == invariants.end()
                )
                {
                    local_deps[&instr.to].insert(arg);
                }
            }

            if(local_deps.find(&instr.to) == local_deps.end()) {
                out.insert(&instr.to);
                invariants.insert(&instr.to);
            }
        }
    }
};

class LoopInvariantAnalysis : public ForwardDataFlowAnalysis<LoopInvariantAnalysisNode> {
protected:
    Fn* func;
    set<Ref*> invariants = {}, defined;

    shared_ptr<LoopInvariantAnalysisNode> createNode(Blk* block) override {
        return shared_ptr<LoopInvariantAnalysisNode>(
            new LoopInvariantAnalysisNode(block, func, invariants, defined)
        );
    }
public:
    LoopInvariantAnalysis(Fn* func, const set<Ref*>& defined)
    : 
        func(func), 
        defined(defined)
    { }
    set<Ref*> getResult() { return invariants; }
};

static set<Ref*> getDefined(const std::set<Blk *>& b) {
    set<Ref*> res = {};

    for(Blk* block: b) {
        for(int i = 0; i < block->nins; i++) {
            Ins& instr = block->ins[i];
            res.insert(&instr.to);
        }
    }

    return res;
}

static void runLoopInvariantCodeMotionForSingleNode(shared_ptr<LoopNode> loop, Fn* func) {
    vector<Blk*> b(loop->blocks.begin(), loop->blocks.end());
    swap(*b.begin(), *find(b.begin(), b.end(), loop->header));

    cout << "LICM for loop " << loop->header->name << " -> " << loop->footer->name << endl;

    auto defined_vars = getDefined(loop->blocks);
    LoopInvariantAnalysis analysis_obj(func, defined_vars);
    analysis_obj.fit(b);
    analysis_obj.analyze();

    for(Ref* inv : analysis_obj.getResult()) {
        printref(*inv, func, stdout);
        fflush(stdout);
        cout << endl;
    }

    cout << endl;
}

static void runLoopInvariantCodeMotionRec(const set<shared_ptr<LoopNode>>& loops, Fn* func) {
    for(auto e : loops) {
        runLoopInvariantCodeMotionRec(e->nested, func);
        runLoopInvariantCodeMotionForSingleNode(e, func);
    }
}

void runLoopInvariantCodeMotion(const set<shared_ptr<LoopNode>>& loops, Fn* func) {
    cout << "Running LICM..." << endl;
    runLoopInvariantCodeMotionRec(loops, func);
}