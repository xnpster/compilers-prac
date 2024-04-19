#pragma once

#include "../qbelib.h"
#include "../Utils/qbeutils.h"

#include <vector>
#include <set>
#include <memory>


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

#if DEBUG
void printLoopNodes(const std::set<std::shared_ptr<LoopNode>> nodes, int lvl);
#endif