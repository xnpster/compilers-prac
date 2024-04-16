#pragma once

#include "../qbelib.h"

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