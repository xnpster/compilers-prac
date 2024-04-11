#pragma once

#include "../qbelib.h"

#include<set>

std::set<Blk*> getPrevBlks(Blk* b);
std::set<Blk*> getNextBlks(Blk* b);