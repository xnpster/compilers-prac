#include "loop_node.h"

#include <iostream>

#include <vector>
#include <set>
#include <list>

#include <memory>


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
    
    shared_ptr<LoopNode> created = shared_ptr<LoopNode>(new LoopNode(reachable, header, footer));

    cout << "new loop node:" << endl;
    for(Blk* b : created->blocks) {
        cout << b->name << endl;
    }
    cout << endl;

    return created;
}

static void insertNodeToTree(set<shared_ptr<LoopNode>>& tree, shared_ptr<LoopNode> node) {
    bool inserted = false;

    for(auto e : tree) {
        if(e->blocks.find(node->header) != e->blocks.end()) {
            node->parent = e;
            insertNodeToTree(e->nested, node);
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
                    cout << "Loop " << next->name << " -> " << top->name << endl;

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