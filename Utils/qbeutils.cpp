#include "qbeutils.h"
#include "../qbelib.h"

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