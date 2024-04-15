#include "qbeutils.h"
#include "../qbelib.h"
#include <map>
#include <memory>

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