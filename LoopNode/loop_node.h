#pragma once

#include "../qbelib.h"

#include <vector>
#include <set>
#include <memory>


class LoopNode {
public:
    std::shared_ptr<LoopNode> parent = NULL;
    std::set<std::shared_ptr<LoopNode>> nested;
    std::set<Blk*> blocks;
    Blk* header;
    Blk* footer;

    LoopNode(
        const std::set<Blk*>& blocks,
        Blk* header,
        Blk* footer
    ):
        blocks(blocks),
        header(header),
        footer(footer)
    {}

    ~LoopNode() {}
};

std::set<std::shared_ptr<LoopNode>> getLoops(Fn* fn);