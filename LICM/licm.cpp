#include "licm.h"
#include "preheader.h"
#include "../DataFlow/forward_data_flow.h"
#include "../Utils/qbeutils.h"
#include "../Utils/macro.h"

#include <map>
#include <set>
#include <memory>

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
                if(!req(arg->real, R) && arg->real.type != RInt && 
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
